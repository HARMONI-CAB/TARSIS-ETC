//
// Curve.h: Generic physical curve, with units
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

#ifndef _ETC_CURVE_H
#define _ETC_CURVE_H

#include <map>
#include <list>
#include <string>
#include <limits>

class Curve;

enum CurveAxis {
  XAxis,
  YAxis
};

struct CurveAssignProxy {
  Curve *parent = nullptr;
  double xPoint;

  void operator=(double);
  operator double() const;
};

class Curve {
  protected:
    double                   m_oobRight = 0;
    double                   m_oobLeft  = 0;
    std::string              m_unitsX;
    std::string              m_unitsY;
    std::map<double, double> m_curve;

  public:
    inline bool
    isOob(double x) const
    {
      if (m_curve.empty())
        return true;

      if (x < m_curve.cbegin()->first)
        return true;

      if (std::prev(m_curve.cend())->first < x)
        return true;

      return false;
    }

    inline double
    xMin() const
    {
      if (m_curve.empty())
        return std::numeric_limits<double>::quiet_NaN();
      return m_curve.begin()->first;
    }

    inline double
    xMax() const
    {
      if (m_curve.empty())
        return std::numeric_limits<double>::quiet_NaN();
      return std::prev(m_curve.end())->first;
    }

    double operator[](double) const;
    double operator()(double) const;
    
    CurveAssignProxy operator[](double);

    void setUnits(CurveAxis, std::string const &);
    
    double integral() const;
    double distMean() const;
    
    void integrate(double k = 0);
    void flip();
    void extendRight();
    void extendLeft();

    virtual void scaleAxis(CurveAxis, double);
    virtual void scaleAxis(CurveAxis, Curve const &);
    virtual void invertAxis(CurveAxis, double factor = 1);

    std::list<double> xPoints() const;

    void set(double, double);
    double get(double) const;
    double getdiff(double) const;

    void multiplyBy(Curve const &);
    void add(Curve const &);
    void add(double);
    void assign(Curve const &);
    void fromExisting(Curve const &, double yUnits = 1.);
    void clear();

    void load(std::string const &, bool transpose = false, unsigned xCol = 0, unsigned yCol = 1);
    void save(std::string const &) const;

    void debug();
};

#endif // _ETC_CURVE_H
