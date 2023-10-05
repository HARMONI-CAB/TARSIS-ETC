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
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include <SkyModel.h>
#include <DataFileManager.h>
#include <Curve.h>
#include <Spectrum.h>
#include <Helpers.h>
#include <cmath>

//////////////////////////////// SkyProperties /////////////////////////////////
bool
SkyProperties::serialize()
{
  auto &self = *this;

  self["skyEmission"]           = skyEmission;
  self["skyEmissionRefAirmass"] = skyEmissionRefAirmass;
  self["skyExtinction"]         = skyExtinction;

  return true;
}

bool
SkyProperties::deserialize()
{
  auto &self = *this;

  deserializeField(skyEmission,           "skyEmission");
  deserializeField(skyEmissionRefAirmass, "skyEmissionRefAirmass");
  deserializeField(skyExtinction,         "skyExtinction");

  return true;
}

///////////////////////////////// SkyModel /////////////////////////////////////
SkyModel::SkyModel()
{
  m_properties    = &ConfigManager::get<SkyProperties>("sky");

  m_skySpectrum   = new Spectrum();
  m_skyExt        = new Curve();
  m_moonToMag     = new Curve();

  //
  // http://www.caha.es/sanchez/sky/
  // X axis is Angstrom
  // Y Axis is 1e-16 Erg / (s cm^2 A) per 2.7 arcsec diam fiber. I.e.,
  //            1.7466e-17 erg / (s cm^2 A arcsec^2), i.e.
  //             7.4309394e-10 W / (m^2 A sr)

  m_skySpectrum->load(dataFile("CAHASky.csv"), false, 1, 2);
  m_skySpectrum->scaleAxis(YAxis, 7.4309394e-10); // To SI units
  m_skySpectrum->scaleAxis(XAxis, 1e-10);         // Convert angstrom to meters

  m_skyExt->load(dataFile("CAHASkyExt.csv"));
  m_moonToMag->load(dataFile("moonBrightness.csv"));
}

SkyModel::~SkyModel()
{
  if (m_skySpectrum)
    delete m_skySpectrum;
  
  if (m_skyExt)
    delete m_skyExt;

  if (m_moonToMag)
    delete m_moonToMag;
}

void
SkyModel::setMoon(double moon)
{
  if (moon < 0 || moon > 100)
    throw std::runtime_error("Moon percent out of bounds");
  
  m_moonFraction = moon;
}

void
SkyModel::setAirmass(double airmass)
{
  if (airmass < 1)
    throw std::runtime_error("Airmass out of bounds");

  m_airmass = airmass;
}

void
SkyModel::setZenithDistance(double z)
{
  if (z < 0 || z >= 90)
    throw std::runtime_error("Zenith distance out of bounds");

  setAirmass(1. / cos(z / 180. * M_PI));
}

Spectrum *
SkyModel::makeSkySpectrum(Spectrum const &object) const
{
  Spectrum *spectPtr     = new Spectrum();
  Spectrum &spectrum     = *spectPtr;
  Curve const &skyExt    = *m_skyExt;
  Spectrum const &skyBg  = *m_skySpectrum;
  Curve const &moon      = *m_moonToMag;


  spectrum.fromExisting(skyBg);
  spectrum.scaleAxis(YAxis, m_airmass);
  spectrum.add(object);
  auto xp = spectrum.xPoints();

  for (auto p : xp) {
    double extFrac = mag2frac(skyExt(p) * m_airmass);
    // Model: I_sky = extinction(airmass) * (object + moon + background * airmass)

    spectrum[p] = extFrac * (
      spectrum(p) 
      + surfaceBrightnessAB2radiance(moon(m_moonFraction), p));
  }

  return spectPtr;
}
