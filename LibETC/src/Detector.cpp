//
// InstrumentModel.h: Instrument model
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

// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include <Detector.h>
#include <Spectrum.h>
#include <stdexcept>

//////////////////////////// DetectorSpec //////////////////////////////
bool
DetectorSpec::serialize()
{
  auto &self = *this;

  self["pixelSide"]    = pixelSide;
  self["readOutNoise"] = readOutNoise;
  self["gain"]         = gain;
  self["darkCurrent"]  = darkCurrent;

  return true;
}

bool
DetectorSpec::deserialize()
{
  auto &self = *this;

  deserializeField(pixelSide,    "pixelSide");
  deserializeField(readOutNoise, "readOutNoise");
  deserializeField(gain,         "gain");
  deserializeField(darkCurrent,  "darkCurrent");

  return true;
}


//////////////////////////// DetectorProperties //////////////////////////////
bool
DetectorProperties::serialize()
{
  auto &self = *this;

  std::map<std::string, YAML::Node> yamlDetectors;

  for (auto &p : detectors) {
    p.second->serialize();
    yamlDetectors[p.first] = p.second->yamlNode();
  }

  self["detectors"] = yamlDetectors;

  return true;
}

bool
DetectorProperties::deserialize()
{
  auto &self = *this;
  std::map<std::string, YAML::Node> yamlDetectors;

  clearDetectors();

  if (deserializeField(yamlDetectors, "detectors")) {
    for (auto p : yamlDetectors) {
      DetectorSpec *spec = new DetectorSpec("detectors." + p.first);
      if (!spec->deserializeYamlNode(p.second)) {
        fprintf(stderr, "%s: failed to deserialize detector\n", p.first);
        delete spec;
      }
      detectors[p.first] = spec;
    }
  }

  return true;
}

void
DetectorProperties::clearDetectors()
{
  for (auto &p : detectors)
    delete p.second;

  detectors.clear();
}

DetectorProperties::~DetectorProperties()
{
  clearDetectors();
}

///////////////////////////// InstrumentModel //////////////////////////////////


Detector::Detector()
{
  m_properties         = &ConfigManager::get<DetectorProperties>("detectors");

  m_photonFluxPerPixel = new Spectrum();
  m_photonsPerPixel    = new Spectrum();
  m_electronsPerPixel  = new Spectrum();
  m_signal             = new Spectrum();
  m_noise              = new Spectrum();
}

Detector::~Detector()
{
  if (m_photonFluxPerPixel != nullptr)
    delete m_photonFluxPerPixel;

  if (m_photonsPerPixel != nullptr)
    delete m_photonsPerPixel;

  if (m_electronsPerPixel != nullptr)
    delete m_electronsPerPixel;

  if (m_signal != nullptr)
    delete m_signal;

  if (m_noise != nullptr)
    delete m_noise;
}

DetectorProperties *
Detector::properties() const
{
  return m_properties;
}

unsigned
Detector::signal(unsigned px) const
{
  return (*m_signal)(px);
}

const Spectrum *
Detector::signal() const
{
  return m_signal;
}

unsigned
Detector::noise(unsigned px) const
{
  return (*m_noise)(px);
}
    
void
Detector::setPixelPhotonFlux(Spectrum const &flux)
{
  m_photonFluxPerPixel->assign(flux);
  recalculate();
}

void
Detector::setExposureTime(double t)
{
  m_expostureTime = t;
}

bool
Detector::setDetector(std::string const &det)
{
  auto it = m_properties->detectors.find(det);
  if (it == m_properties->detectors.end())
    return false;

  m_detector = it->second;
  return true;
}

void
Detector::recalculate()
{
  if (m_detector == nullptr)
    throw std::runtime_error("No detector selected");

  double invGain = 1. / m_detector->gain;

  // Compute electrons in each pixel by means of the exposure time
  m_photonsPerPixel->fromExisting(
    *m_photonFluxPerPixel,
    m_expostureTime * m_detector->pixelSide * m_detector->pixelSide);
  
  // Compute number of electrons by means of the quantum efficiency
  m_electronsPerPixel->fromExisting(*m_photonsPerPixel, m_detector->qE);
  
  // Turn this into counts
  m_signal->fromExisting(*m_electronsPerPixel, invGain);

  // Compute noise floor
  double noiseFloor = 
    m_detector->readOutNoise + m_detector->darkCurrent * m_expostureTime;
  
  m_noise->clear();
  (*m_noise)[m_signal->xMin()] = invGain * noiseFloor;
  (*m_noise)[m_signal->xMax()] = invGain * noiseFloor;
}