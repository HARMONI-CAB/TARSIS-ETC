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
CalculationWorker::simulate()
{
  if (m_simulation == nullptr) {
    emit exception("Simulation not initialized");
    return;
  }

  try {
    CalculationProduct result;
    m_simulation->simulateArm(BlueArm);

    for (unsigned i = 0; i < DETECTOR_PIXELS; ++i) {
      double signal = m_simulation->signal(i);
      double noise  = m_simulation->noise(i);

      result.blueArm.wavelength[i] = m_simulation->pxToWavelength(i);
      result.blueArm.wlToPixel     = m_simulation->wlToPixelCurve();
      result.blueArm.signal[i]     = signal;
      result.blueArm.noise[i]      = noise;
      result.blueArm.counts[i]     = static_cast<int>(signal + noise * randNormal());
    }
    result.blueArm.initialized = true;

    m_simulation->simulateArm(RedArm);

    for (unsigned i = 0; i < DETECTOR_PIXELS; ++i) {
      double signal = m_simulation->signal(i);
      double noise  = m_simulation->noise(i);

      result.redArm.wavelength[i] = m_simulation->pxToWavelength(i);
      result.redArm.wlToPixel     = m_simulation->wlToPixelCurve();
      result.redArm.signal[i]     = signal;
      result.redArm.noise[i]      = noise;
      result.redArm.counts[i]     = static_cast<int>(signal + noise * randNormal());
    }

    result.redArm.initialized = true;

    emit done("simulate");
    emit dataProduct(result);
  } catch (std::runtime_error const &e) {
    emit exception(e.what());
  }
}
