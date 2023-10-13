//
// CalculationWorker.cpp: Run calculation code
// Copyright (c) 2023 Gonzalo J. Carracedo <BatchDrake@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//


#include "CalculationWorker.h"
#include "GUIHelpers.h"
#include <random>
#include <sys/time.h>

static bool g_registered = false;

CalculationWorker::CalculationWorker(QObject *parent) : QObject(parent)
{
  if (!g_registered) {
    qRegisterMetaType<SimulationParams>();
    qRegisterMetaType<CalculationProduct>();
    g_registered = true;
  }
}

CalculationWorker::~CalculationWorker()
{
  if (m_simulation != nullptr)
    delete m_simulation;
}

void
CalculationWorker::init()
{
  if (m_simulation == nullptr) {
    try {
      m_simulation = new Simulation();
      emit done("init");
    } catch (std::runtime_error const &e) {
      emit exception(e.what());
    }
  }
}

void
CalculationWorker::setInputSpectrum(QString path)
{
  if (m_simulation == nullptr) {
    emit exception("Simulation not initialized");
    return;
  }

  try {
    Spectrum newSpectrum;

    newSpectrum.load(path.toStdString());
    newSpectrum.scaleAxis(XAxis, 1e-9);

    m_inputSpectrum = newSpectrum;
    m_newSpectrum   = true;

    emit done("setInputSpectrum");
  } catch (std::runtime_error const &e) {
    emit exception(e.what());
  }
}

void
CalculationWorker::setParams(SimulationParams params)
{
  if (m_simulation == nullptr) {
    emit exception("Simulation not initialized");
    return;
  }

  try {
    // AB magnitude changed, recalculate input spectrum
    if (m_newSpectrum ||
        fabs(m_simParams.rABmag - params.rABmag) > std::numeric_limits<double>::epsilon()) {
      m_simulation->setInput(m_inputSpectrum);
      m_simulation->normalizeToRMag(params.rABmag);
      m_newSpectrum = false;
    }

    m_simParams = params;
    m_simulation->setParams(params);
    emit done("setParams");
  } catch (std::runtime_error const &e) {
    emit exception(e.what());
  }
}

SNRCurve::SNRCurve()
{
  wavelength.resize(DETECTOR_PIXELS);
  signal.resize(DETECTOR_PIXELS);
  noise.resize(DETECTOR_PIXELS);
  counts.resize(DETECTOR_PIXELS);
}

void
CalculationWorker::simulateArm(InstrumentArm arm, SNRCurve &curve)
{
  std::default_random_engine generator;
  double ron, invGain;
  struct timeval tv;
  gettimeofday(&tv, nullptr);

  m_simulation->simulateArm(arm);

  generator.seed(
        static_cast<uint64_t>(tv.tv_usec)
        + static_cast<uint64_t>(tv.tv_sec) * 1000000ull);

  invGain = 1. / m_simulation->gain();
  ron = m_simulation->readOutNoise(); // In counts
  for (unsigned i = 0; i < DETECTOR_PIXELS; ++i) {
    double electrons = m_simulation->electrons(i);
    double signal = m_simulation->signal(i);
    double noise  = m_simulation->noise(i);
    std::poisson_distribution<int> shotElectrons(electrons);

    curve.wavelength[i] = m_simulation->pxToWavelength(i);
    curve.wlToPixel     = m_simulation->wlToPixelCurve();
    curve.signal[i]     = signal;
    curve.noise[i]      = noise;
    curve.counts[i]     =
        static_cast<int>(
            invGain * shotElectrons(generator)
          + ron * randNormal());
  }

  curve.initialized = true;
}

void
CalculationWorker::singleShot()
{
  CalculationProduct result;

  simulateArm(BlueArm, result.blueArm);
  simulateArm(RedArm,  result.redArm);

  emit dataProduct(result);
}

void
CalculationWorker::allSlices()
{
  CalculationProduct result;
  auto paramCopy = m_simParams;

  for (int i = 0; i < TARSIS_SLICES; ++i) {
    emit progress(100. * (i + 1.) / TARSIS_SLICES);

    paramCopy.slice = i;
    m_simulation->setParams(paramCopy);

    simulateArm(BlueArm, result.blueArm);
    simulateArm(RedArm,  result.redArm);

    emit dataProduct(result);
  }

  m_simulation->setParams(m_simParams);
}

void
CalculationWorker::simulate()
{
  if (m_simulation == nullptr) {
    emit exception("Simulation not initialized");
    return;
  }

  try {
    if (m_simParams.slice < 0)
      allSlices();
    else
      singleShot();

    emit done("simulate");

  } catch (std::runtime_error const &e) {
    emit exception(e.what());
  }
}
