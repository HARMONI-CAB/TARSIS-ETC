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

#ifndef _ETC_INSTRUMENT_H
#define _ETC_INSTRUMENT_H

#include "ConfigManager.h"
#include <cmath>

class Curve;
class Spectrum;

#define CAHA_APERTURE_DIAMETER 3.5    // m
#define CAHA_FOCAL_LENGTH      12.195 // m
#define CAHA_APERTURE_AREA     (.25 * M_PI * CAHA_APERTURE_DIAMETER * CAHA_APERTURE_DIAMETER)
#define CAHA_EFFECTIVE_AREA    9.093  // m^2

#define TARSIS_SLICES          40     // Number of TARSIS slices per FOV
#define SPECTRAL_PIXEL_LENGTH  2048   // Guessed from the resolution elements and ranges

struct InstrumentProperties : public Config {
  using Config::Config;

  double fNum          = CAHA_FOCAL_LENGTH / CAHA_APERTURE_DIAMETER;
  double apEfficiency  = CAHA_EFFECTIVE_AREA / CAHA_APERTURE_AREA;
  std::string detector = "ML15";

  virtual bool serialize() override;
  virtual bool deserialize() override;
};

enum InstrumentArm {
  BlueArm,
  RedArm
};

//
// Instrument model. Unless specified, all units are SI. This is: meters,
// seconds, Joules, Hertzs and so on
//

class InstrumentModel {
    InstrumentProperties *m_properties = nullptr; // Borrowed
    Curve    *m_blueML15      = nullptr;          // Owned, blue + ML15
    Curve    *m_blueNBB       = nullptr;          // Owned, blue + NBB
    Curve    *m_redML15       = nullptr;          // Owned, red + ML15

    ///////////////////////////// Blue arm ///////////////////////////
    Curve    *m_blueDisp[TARSIS_SLICES];          // Owned, spectral dispersion
    Curve    *m_blueREPx[TARSIS_SLICES];          // Resolution element (in pixels)
    Curve    *m_blueW2Px[TARSIS_SLICES];          // Owned, int of inv of sd
    Curve    *m_bluePx2W[TARSIS_SLICES];          // Owned, inverse of above

    ////////////////////////////// Red arm ///////////////////////////
    Curve    *m_redDisp[TARSIS_SLICES];           // Owned, spectral dispersion
    Curve    *m_redREPx[TARSIS_SLICES];           // Resolution element (in pixels)
    Curve    *m_redW2Px[TARSIS_SLICES];           // Owned, int of inv of sd
    Curve    *m_redPx2W[TARSIS_SLICES];           // Owned, inverse of above

    Spectrum *m_attenSpectrum   = nullptr;        // Owned, attenuated before dispersor
    InstrumentArm m_currentPath = BlueArm;        // Path of the attenuated spectrum

  public:
    InstrumentModel();
    ~InstrumentModel();

    // Returns instrument properties (required for simulation)
    InstrumentProperties *properties() const;

    // Turns a pixel into lambda
    double pxToWavelength(InstrumentArm arm, unsigned slice, unsigned pixel) const;
    Curve *pxToWavelength(unsigned slice) const;

    int    wavelengthToPx(InstrumentArm arm, unsigned slice, double lambda) const;


    // Set input spectrum. The input spectrum must be in radiance units,
    // with a *wavelength* spectral axis* i.e. J / (s * m^2 * sr * m)
    void   setInput(InstrumentArm arm, Spectrum const &);

    // Returns the per-pixel photon flux,in units of in ph / (s m^2)
    Spectrum *makePixelPhotonFlux(unsigned int slice) const;
};

#endif // _ETC_INSTRUMENT_H
