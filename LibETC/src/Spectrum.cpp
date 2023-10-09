//
// Spectrum.cpp: Generic spectrum curve
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

#include <Spectrum.h>
#include <cmath>
#include <stdexcept>

void
Spectrum::scaleAxis(CurveAxis axis, double factor)
{
  if (axis == XAxis) {
    std::list<std::map<double, double>::node_type> xp;

    auto it = m_curve.cbegin();
    auto next = std::next(it);

    while (it != m_curve.end()) {
      next = std::next(it);
      xp.push_back(std::move(m_curve.extract(it)));
      it = next;
    }
    
    for (auto &p : xp) {
      p.key()    *= factor;
      p.mapped() /= factor;
      m_curve.insert(std::move(p));
    }

    m_oobLeft  /= factor;
    m_oobRight /= factor;
  } else {
    Curve::scaleAxis(axis, factor);
  }
}

void
Spectrum::scaleAxis(CurveAxis axis, Curve const &curve)
{
  if (axis == XAxis) {
    std::list<std::map<double, double>::node_type> xp;
    
    auto it = m_curve.cbegin();
    auto next = std::next(it);

    while (it != m_curve.end()) {
      next = std::next(it);
      xp.push_back(std::move(m_curve.extract(it)));
      it = next;
    }

    //
    // fz(g(x)) = fx(x) / g'(x), this is:
    // p.key()     = curve(x)
    // p.mapped() /= curve_diff(x)
    //
    // We would like the differentiation to be (almost) the inverse of
    // integrate()

    for (auto &p : xp) {
      double x    = p.key();
      double diff = fabs(curve.getdiff(x));
      p.key()     = curve(x);

      if (diff != 0.0) {
        p.mapped() /= diff;
        m_curve.insert(std::move(p));
      }
    }
    
    if (m_oobLeft != 0.0)
      m_oobLeft  /= fabs(curve.getdiff(m_curve.begin()->second));
    if (m_oobRight != 0.0)
      m_oobRight /= fabs(curve.getdiff(std::prev(m_curve.end())->second));
  } else {
    Curve::scaleAxis(axis, curve);
  }
}

void
Spectrum::scaleAxis(CurveAxis axis, Curve const &curve, Curve const &diff)
{
  if (axis == XAxis) {
    std::list<std::map<double, double>::node_type> xp;
    
    auto it = m_curve.cbegin();
    auto next = std::next(it);

    while (it != m_curve.end()) {
      next = std::next(it);
      xp.push_back(std::move(m_curve.extract(it)));
      it = next;
    }
    
    //
    // fz(g(x)) = fx(x) / g'(x), this is:
    // p.key()     = curve(x)
    // p.mapped() /= curve_diff(x)
    //
    // We would like the differentiation to be (almost) the inverse of
    // integrate()

    for (auto &p : xp) {
      double x    = p.key();
      double dfdx = fabs(diff(x));
      p.key()     = curve(x);

      if (dfdx != 0.0) {
        p.mapped() /= dfdx;
        m_curve.insert(std::move(p));
      }
    }
    
    if (m_oobLeft != 0.0)
      m_oobLeft  /= fabs(diff(m_curve.begin()->second));
    if (m_oobRight != 0.0)
      m_oobRight /= fabs(diff(std::prev(m_curve.end())->second));
  } else {
    Curve::scaleAxis(axis, curve);
  }
}

void
Spectrum::invertAxis(CurveAxis axis, double factor)
{
  if (m_curve.empty())
      return;

  if (axis == XAxis) {
    std::list<std::map<double, double>::node_type> xp;

    auto it = m_curve.cbegin();
    auto next = std::next(it);

    while (it != m_curve.end()) {
      next = std::next(it);
      xp.push_back(std::move(m_curve.extract(it)));
      it = next;
    }

    if (xp.front().key() < 0)
      throw std::runtime_error("Inverting spectrums with negative values in the X axis not yet supported");

    //
    // g(x)    = factor / x
    // 1/g'(x) = x*x / factor
    //

    for (auto &p : xp) {
      double x    = p.key();
      p.key()     = factor / x;
      p.mapped() *= (x * x) / factor;
      m_curve.insert(std::move(p));
    }

    //
    // Everything to the right is going to be squashed in a point of 
    // infinite density towards 0.
    //

    if (m_oobRight != 0.)
      m_curve[0] = std::numeric_limits<double>::infinity();
    else
      m_curve[0] = 0.;
    
    m_oobLeft  = m_curve[0];
    m_oobRight = 0;
  } else {
    Curve::invertAxis(axis);
  }
}
