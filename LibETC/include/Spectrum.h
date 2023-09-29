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

// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef _ETC_SPECTRUM_H
#define _ETC_SPECTRUM_H

#include "Curve.h"

class Spectrum : public Curve {
  public:
    virtual void scaleAxis(CurveAxis, double) override;
    virtual void scaleAxis(CurveAxis, Curve const &) override;
    virtual void scaleAxis(CurveAxis, Curve const &, Curve const &diff);
    virtual void invertAxis(CurveAxis, double factor = 1) override;
};

#endif // _ETC_SPECTRUM_H
