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

#ifndef _ETC_DETECTOR_H
#define _ETC_DETECTOR_H

#include "ConfigManager.h"

#define DETECTOR_PIXELS 2048

class Spectrum;
class Curve;

struct DetectorSpec : public Config {
  using Config::Config;

  double pixelSide    = 15e-6; // m
  double readOutNoise = 0;     // e-
  double gain         = 1.;    // e-/count
  double darkCurrent  = 0;     // e-/s
  double qE           = 1;     // 1

  virtual bool serialize() override;
  virtual bool deserialize() override;
};

struct DetectorProperties : public Config {
  using Config::Config;

  std::map<std::string, DetectorSpec *> detectors;

  virtual bool serialize() override;
  virtual bool deserialize() override;

  void clearDetectors();
  virtual ~DetectorProperties();
};

class Detector {
    DetectorProperties *m_properties = nullptr;
    DetectorSpec       *m_detector = nullptr;

    double m_expostureTime = 1.;

    Spectrum *m_photonFluxPerPixel; // ph / (px m^2 s)
    Spectrum *m_photonsPerPixel;    // ph/px
    Spectrum *m_electronsPerPixel;  // e/px
    Spectrum *m_signal;             // counts
    Spectrum *m_noise;              // noise

  public:
    Detector();
    ~Detector();

    DetectorProperties *properties() const;
    unsigned signal(unsigned px) const;
    unsigned noise(unsigned px) const;
    const Spectrum *signal() const;
    
    bool setDetector(std::string const &);
    void setPixelPhotonFlux(Spectrum const &);
    void setExposureTime(double);
    void recalculate();
};

#endif //_ETC_DETECTOR_H
