//
// CalculationWorker.h: Run calculation code
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

#ifndef CALCULATIONWORKER_H
#define CALCULATIONWORKER_H

#include <QObject>
#include <QMetaType>
#include <Simulation.h>
#include <vector>

Q_DECLARE_METATYPE(SimulationParams)

struct SNRCurve {
  bool                initialized = false;
  std::vector<double> wavelength;
  std::vector<double> signal;
  std::vector<double> noise;

  SNRCurve();
};

struct CalculationProduct {
  SNRCurve redArm;
  SNRCurve blueArm;
};

Q_DECLARE_METATYPE(CalculationProduct);

class CalculationWorker : public QObject
{
  Q_OBJECT

  Simulation *m_simulation = nullptr;
  SimulationParams m_simParams;

  Spectrum m_inputSpectrum;

public:
  CalculationWorker(QObject *parent = nullptr);
  virtual ~CalculationWorker() override;

public slots:
  void init();
  void setInputSpectrum(QString);
  void setParams(SimulationParams);
  void simulate();

signals:
  void done(QString);
  void dataProduct(CalculationProduct);
  void exception(QString);
};

#endif // CALCULATIONWORKER_H
