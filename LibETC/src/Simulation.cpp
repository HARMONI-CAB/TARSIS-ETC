//
// Simulation.cpp: Run instrument simulations
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

#include <Simulation.h>
#include <ConfigManager.h>
#include <DataFileManager.h>
#include <Helpers.h>

Simulation::Simulation()
{
  // Init model
  m_skyModel    = new SkyModel();
  m_det         = new Detector();
  m_tarsisModel = new InstrumentModel();

  // 
  // http://svo2.cab.inta-csic.es/theory/fps/index.php?id=Generic/Cousins.R&&mode=browse&gname=Generic&gname2=Cousins
  //

  m_cousinsR.load(dataFile("Generic_Cousins.R.dat"));
  m_cousinsR.scaleAxis(XAxis, 1e-10); // X axis was in angstrom
  m_cousinsR.invertAxis(XAxis, SPEED_OF_LIGHT); // To frequency

  m_cousinsREquivBw = m_cousinsR.integral();
}

Simulation::~Simulation()
{
  if (m_skyModel != nullptr)
    delete m_skyModel;

  if (m_tarsisModel != nullptr)
    delete m_tarsisModel;

  if (m_det != nullptr)
    delete m_det;

  if (m_sky != nullptr)
    delete m_sky;
}

void
Simulation::setInput(Spectrum const &spec)
{
  m_input = spec;
}

// Compute spectrum's R magnitude
void
Simulation::normalizeToRMag(double R)
{
  Spectrum filtered;
  double meanSB, desiredSB;

  //
  // The following calculation departs from the following assumption. If we
  // feed the R filter by a flat radiance of 0 magAB (/arcsec^2), the resulting
  // R mag must also be 0 magAB,R (/arcsec^2)
  //

  // This is the desired radiance, normalized by the R transfer
  desiredSB = surfaceBrightnessAB2FreqRadiance(R);

  // Input is assumed to be in W / (m^2 sr m)
  filtered.fromExisting(m_input);

  // Turn into W / (m^2 sr Hz)
  filtered.invertAxis(XAxis, SPEED_OF_LIGHT);

  // Filter (still in W / (m^2 sr Hz))
  filtered.multiplyBy(m_cousinsR);

  // Integrate the spectrum axis. Units are W / (m^2 sr). Dividing by 
  // the equivalent bandwidth gives a mean spectral surface brightness
  meanSB = filtered.integral() / m_cousinsREquivBw;

  // And normalize
  m_input.scaleAxis(YAxis, desiredSB / meanSB);
}

void
Simulation::setParams(SimulationParams const &params)
{
  m_params = params;

  m_skyModel->setAirmass(params.airmass);
  m_skyModel->setMoon(params.moon);
  
  if (m_sky != nullptr) {
    delete m_sky;
    m_sky = nullptr;
  }

  // Update sky spectrum
  m_sky = m_skyModel->makeSkySpectrum(m_input);

  // Update detector config
  m_det->setExposureTime(params.exposure);
}

void
Simulation::simulateArm(InstrumentArm arm)
{
  Spectrum *flux = nullptr;

  try {
    auto tarsisProp = m_tarsisModel->properties();
    std::string detName;

    switch (arm) {
      case BlueArm:
        detName = m_params.blueDetector;
        break;

      case RedArm:
        detName = m_params.redDetector;
        break;

      default:
        throw std::runtime_error("Unknown arm");
    }

    if (!m_det->setDetector(detName))
      throw std::runtime_error("Unknown detector `" + detName + "'");

    // Set coating
    tarsisProp->coating = m_det->getSpec()->coating;
    m_tarsisModel->setInput(arm, *m_sky);
    flux = m_tarsisModel->makePixelPhotonFlux(m_params.slice);
    m_det->setPixelPhotonFlux(*flux);
    delete flux;
    flux = nullptr;
  } catch (std::runtime_error const &e) {
    if (flux != nullptr)
      delete flux;
    throw;
  } 

  if (flux != nullptr)
    delete flux;
}

double
Simulation::signal(unsigned px) const
{
  return m_det->signal(px);
}

double
Simulation::noise(unsigned px) const
{
  return m_det->noise(px);
}

double
Simulation::electrons(unsigned px) const
{
  return m_det->electrons(px);
}

double
Simulation::readOutNoise() const
{
  return m_det->readOutNoise();
}

double
Simulation::gain() const
{
  auto spec = m_det->getSpec();

  if (spec == nullptr)
    throw std::runtime_error("No detector specified");
  
  return spec->gain;
}

double
Simulation::pxToWavelength(unsigned px) const
{
  return (*m_tarsisModel->pxToWavelength(m_params.slice))(px);
}

Curve const &
Simulation::wlToPixelCurve() const
{
  return (*m_tarsisModel->wavelengthToPx(m_params.slice));
}
