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

#ifndef _HELPERS_H
#define _HELPERS_H

#include <cmath>

#define SPEED_OF_LIGHT         299792458.       // m/s
#define PLANCK_CONSTANT        6.62607015e-34   // J s

#define JANSKY                 1e-26            // W / (m^2 Hz)
#define AB_ZEROPOINT           (3631 * JANSKY)  // W / (m^2 Hz)

#define ARCSEC                 4.8481368e-06    // radian

#define STD2FWHM               2.35482004503095 // sqrt(8 * ln(2))
#define INVSQRT2PI             0.39894228040143 // 1 / sqrt(2 * pi)

static inline double
mag2frac(double mag)
{
  return pow(10, -.4 * mag);
}

static inline double
surfaceBrightnessAB2FreqRadiance(double mag)
{
  double fnu     = mag2frac(mag) * AB_ZEROPOINT / (ARCSEC * ARCSEC);
  return fnu;
}

static inline double
surfaceBrightnessAB2radiance(double mag, double wl)
{
  double fnu     = surfaceBrightnessAB2FreqRadiance(mag);
  double flambda = SPEED_OF_LIGHT / (wl * wl) * fnu;

  return flambda;
}

static inline double
surfaceBrightnessVega2radiance(double mag, double wl)
{
  double flambda = mag2frac(mag) * 0.03631 / (ARCSEC * ARCSEC);
  return flambda;
}

#endif // _HELPERS_H
