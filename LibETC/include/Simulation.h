//
// Simulation.h: Run instrument simulations
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

#ifndef _ETC_SIMULATION_H
#define _ETC_SIMULATION_H

#include <Curve.h>
#include <Spectrum.h>
#include <string>
#include <SkyModel.h>
#include <InstrumentModel.h>
#include <Detector.h>

struct SimulationParams {
  const char *progName     = nullptr;
  std::string blueDetector = "CCD231-84-0-S77";
  std::string redDetector  = "CCD231-84-0-H69";
  double airmass           = 1.;
  double moon              = 0.;
  double exposure          = 3600;
  double rABmag            = 18.;
  int    slice             = 20;
};

class Simulation {
    Spectrum  m_input;
    Curve     m_cousinsR;
    double    m_cousinsREquivBw;
    Spectrum *m_sky = nullptr;
    SkyModel *m_skyModel = nullptr;
    InstrumentModel *m_tarsisModel = nullptr;
    Detector *m_det = nullptr;
    SimulationParams m_params;


  public:
    Simulation();
    ~Simulation();

    void setInput(Spectrum const &spec);
    void normalizeToRMag(double mag);
    void setParams(SimulationParams const &params);

    void simulateArm(InstrumentArm arm);
    double signal(unsigned px) const;
    double noise(unsigned px) const;
    double electrons(unsigned px) const;
    double readOutNoise() const;
    double gain() const;
    double pxToWavelength(unsigned px) const;
    Curve const &wlToPixelCurve() const;
};

#endif // _ETC_SIMULATION_H

