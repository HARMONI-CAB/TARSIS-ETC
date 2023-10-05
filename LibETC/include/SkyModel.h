//
// Spectrum.h: Generic spectrum curve
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

#ifndef _SKY_MODEL_H
#define _SKY_MODEL_H

#include "ConfigManager.h"
#include <cmath>

class Curve;
class Spectrum;

struct SkyProperties : public Config {
  using Config::Config;

  std::string skyEmission           = "CAHASKy.csv";
  double      skyEmissionRefAirmass = 1.;
  std::string skyExtinction         = "CAHASkyExt.csv";
  
  virtual bool serialize() override;
  virtual bool deserialize() override;
};

class SkyModel {
  SkyProperties *m_properties = nullptr; // Borrowed

  Curve    *m_skyExt       = nullptr;    // Owned
  Spectrum *m_skySpectrum  = nullptr;    // Owned
  Curve    *m_moonToMag    = nullptr;    // Owned
  double    m_airmass      = 1.;
  double    m_moonFraction = 0.;

  // TODO: Add moon spectrum

public:
  SkyModel();
  ~SkyModel();

  SkyProperties *properties() const;

  void setMoon(double);
  void setAirmass(double);
  void setZenithDistance(double);

  // Returns a radiance spectrum
  Spectrum *makeSkySpectrum(Spectrum const &) const;
};

#endif // _SKY_MODEL_H
