//
// Curve.cpp: Generic physical curve, with units
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

#include <Curve.h>
#include <cstdio>
#include <cerrno>
#include <stdexcept>
#include <cstring>
#include <fstream>
#include <set>

#include <rapidcsv.h>

void
CurveAssignProxy::operator=(double val)
{
  parent->set(xPoint, val);
}

CurveAssignProxy::operator double() const
{
  return parent->get(xPoint);
}

double
Curve::operator[](double x) const
{
  return get(x);
}

double
Curve::operator()(double x) const
{
  return get(x);
}

CurveAssignProxy
Curve::operator[](double x)
{
  CurveAssignProxy proxy;

  proxy.parent = this;
  proxy.xPoint = x;

  return proxy;
}

void
Curve::setUnits(CurveAxis axis, std::string const &units)
{
  switch (axis) {
    case XAxis:
      m_unitsX = units;
      break;

    case YAxis:
      m_unitsY = units;
      break;
  }
}

double
Curve::integral() const
{
  std::map<double, double>::const_iterator it, next;
  double accum = 0;
  double err   = 0;

  if (m_oobRight + m_oobLeft != 0)
    return std::numeric_limits<double>::infinity() * (m_oobLeft + m_oobRight);
  
  it = m_curve.begin();
  if (it == m_curve.end())
    return 0;

  while ((next = std::next(it)) != m_curve.end()) {
    double dx = next->first - it->first;
    double my = .5 * (next->second + it->second);

    // More Kahan
    double term_real         = my * dx - err;
    volatile double tmp      = accum + term_real;
    volatile double term_err = tmp - accum;
    err                      = term_err - term_real;
    accum                    = tmp;

    it = next;
  }

  return accum;
}

double
Curve::distMean() const
{
  std::map<double, double>::const_iterator it, next;
  double accum = 0;
  double err   = 0;

  if (m_oobRight + m_oobLeft != 0)
    return .5 * (m_oobLeft + m_oobRight);
  
  it = m_curve.begin();
  if (it == m_curve.end())
    return it->first;

  while ((next = std::next(it)) != m_curve.end()) {
    double dx = next->first - it->first;
    double x0 = .5 * (next->first + it->first);
    double my = .5 * (next->second + it->second);

    // More Kahan
    double term_real         = x0 * my * dx - err;
    volatile double tmp      = accum + term_real;
    volatile double term_err = tmp - accum;
    err                      = term_err - term_real;
    accum                    = tmp;

    it = next;
  }

  return accum / integral();
}

void
Curve::integrate(double K)
{
  double accum = K;
  double err   = 0;
  double x0, x_prev, y_prev, x, y;
  
  m_oobLeft = K;

  if (m_curve.empty()) {
    m_oobRight = K;
    return;
  }
  
  if (m_curve.size() == 1) {
    auto it    = m_curve.begin();
    it->second = K;
    m_oobRight = K;
    return;
  }

  // Get point at x0
  auto it       = m_curve.begin();
  auto next     = std::next(it);
  double dx     = next->first - it->first;
  x0            = it->first;

  //
  // At x_1 there must be the accumulated area from x_0 to x_1
  // At x_2 there must be the accumulared area from x_0 to x_2
  // And so on and so forth
  //

  x = it->first;
  y = it->second;

  it = next;

  while (it != m_curve.end()) {
    x_prev = x;
    y_prev = y;

    x  = it->first;
    y  = it->second;
    dx = x - x_prev;

    // Kahan summation to calculate Y
    double term_real         = .5 * (y + y_prev) * dx - err;
    volatile double tmp      = accum + term_real;
    volatile double term_err = tmp - accum;
    err                      = term_err - term_real;
    accum                    = tmp;
    it->second               = accum;

    // Advance
    ++it;
  }

  m_curve[x0] = K;
  m_curve[x + dx] = y * (x - x_prev + dx) + m_curve[x_prev];

  m_oobRight = accum;
}

void
Curve::flip()
{
  std::string tmp = m_unitsX;
  m_unitsX = m_unitsY;
  m_unitsY = tmp;

  std::list<std::map<double, double>::node_type> xp;

  for (auto it = m_curve.cbegin(); it != m_curve.cend(); ++it)
    xp.push_back(std::move(m_curve.extract(it)));
  
  for (auto &p : xp) {
    double tmp = p.key();
    p.key() = p.mapped();
    p.mapped() = tmp;
    m_curve.insert(std::move(p));
  }
}

void
Curve::extendLeft()
{
  if (m_curve.empty())
    return;
  
  m_oobLeft = m_curve.cbegin()->second;
}

void
Curve::extendRight()
{
  if (m_curve.empty())
    return;
  
  m_oobRight = std::prev(m_curve.end())->second;
}

void
Curve::scaleAxis(CurveAxis axis, double factor)
{
  if (axis == XAxis) {
    std::list<std::map<double, double>::node_type> xp;

    for (auto it = m_curve.cbegin(); it != m_curve.cend(); ++it)
      xp.push_back(std::move(m_curve.extract(it)));
    
    for (auto &p : xp) {
      p.key() *= factor;
      m_curve.insert(std::move(p));
    }

  } else {
    for (auto &p : m_curve)
      p.second *= factor;

    m_oobLeft  *= factor;
    m_oobRight *= factor;
  }
}

void
Curve::scaleAxis(CurveAxis axis, Curve const &curve)
{
  if (axis == XAxis) {
    std::list<std::map<double, double>::node_type> xp;

    for (auto it = m_curve.cbegin(); it != m_curve.cend(); ++it)
      xp.push_back(m_curve.extract(it));
    
    for (auto &p : xp) {
      p.key() = curve(p.key());
      m_curve.insert(std::move(p));
    }
  } else {
    for (auto &p : m_curve)
      p.second = curve(p.second);
    
    m_oobLeft  = curve(m_oobLeft);
    m_oobRight = curve(m_oobRight);
  }
}

void
Curve::invertAxis(CurveAxis axis, double factor)
{
  if (axis == XAxis) {
    std::list<std::map<double, double>::node_type> xp;

    for (auto it = m_curve.cbegin(); it != m_curve.cend(); ++it)
      xp.push_back(m_curve.extract(it));
    
    for (auto &p : xp) {
      p.key() = factor / p.key();
      m_curve.insert(std::move(p));
    }
  } else {
    for (auto &p : m_curve)
      p.second = factor / p.second;
    
    m_oobLeft  = 1 / m_oobLeft;
    m_oobRight = 1 / m_oobRight;
  }
}

void
Curve::set(double x, double y)
{
  m_curve[x] = y;
}

std::list<double>
Curve::xPoints() const
{
  std::list<double> xP;

  for (auto &p : m_curve)
    xP.push_back(p.first);

  return xP;
}

double
Curve::get(double x) const
{
  auto next = m_curve.lower_bound(x);
  if (next == m_curve.end())
    return m_oobRight;

  if (next == m_curve.begin()) {
    if (x < next->first)
      return m_oobLeft;
    return next->second;
  }
  
  auto prev = std::prev(next);
  
  double x0 = prev->first;
  double y0 = prev->second;
  double x1 = next->first;
  double y1 = next->second;

  return y0 + (x - x0) * (y1 - y0) / (x1 - x0);
}

double
Curve::getdiff(double x) const
{
  auto next = m_curve.lower_bound(x);
  if (next == m_curve.end())
    return 0;

  if (next == m_curve.begin())
    return 0;
  
  auto prev = std::prev(next);
  
  double x0 = prev->first;
  double y0 = prev->second;
  double x1 = next->first;
  double y1 = next->second;
  
  // Two cases:
  if (next->first != x) {
    // Middle of a segment. Easy
    return (y1 - y0) / (x1 - x0);
  } else {
    // Edge of a segment. We need to take the next one into account
    auto after = std::next(next);

    if (after == m_curve.end())
      return 0;

    double x2 = after->first;
    double y2 = after->second;

    return (y2 - y0) / (x2 - x0);
  }
}

void
Curve::multiplyBy(Curve const &curve)
{
  // Construct the union of both curves
  std::set<double> xp;
  std::list<std::pair<double, double>> newPairs;
  Curve &self = *this;

  for (auto &p : m_curve)
    xp.emplace(p.first);

  for (auto &p : curve.m_curve)
    xp.emplace(p.first);

  for (auto &x : xp) {
    std::pair<double, double> pair;
    pair.first  = x;
    pair.second = self(x) * curve(x);
    newPairs.push_back(pair);
  }
  
  // Recreate curve
  m_curve.clear();

  m_oobLeft  *= curve.m_oobLeft;
  m_oobRight *= curve.m_oobRight;
  for (auto &p : newPairs)
    m_curve.insert(std::move(p));
}

void
Curve::add(Curve const &curve)
{
  // Construct the union of both curves
  std::set<double> xp;
  std::list<std::pair<double, double>> newPairs;
  Curve &self = *this;

  for (auto &p : m_curve)
    xp.emplace(p.first);

  for (auto &p : curve.m_curve)
    xp.emplace(p.first);

  for (auto &x : xp) {
    std::pair<double, double> pair;
    pair.first  = x;
    pair.second = self(x) + curve(x);
    newPairs.push_back(pair);
  }
  
  // Recreate curve
  m_curve.clear();

  m_oobLeft  += curve.m_oobLeft;
  m_oobRight += curve.m_oobRight;
  for (auto &p : newPairs)
    m_curve.insert(std::move(p));
} 

void
Curve::assign(Curve const &curve)
{
  // Determine overlapping segments:
  auto ownFirst = m_curve.cbegin();
  auto crvFirst = curve.m_curve.cbegin();

  // Nothing to add
  if (crvFirst == curve.m_curve.end())
    return;

  // No curve
  if (ownFirst == m_curve.end()) {
    *this = curve;
    return;
  }

  // Assign middle part (this must go first!)
  for (auto &p : m_curve)
    if (!curve.isOob(p.first))
      p.second = curve(p.first);

  // Curve is longer to the left
  if (crvFirst->first < ownFirst->first)
    m_oobLeft = curve.m_oobLeft;

  auto ownLast = std::prev(m_curve.cend());
  auto crvLast = std::prev(curve.m_curve.cend());

  // Curve is longer to the right
  if (ownLast->first < crvLast->first)
    m_oobRight = curve.m_oobRight;

  // Assign the whole curve
  for (auto &p : curve.m_curve)
    set(p.first, p.second);
}

void
Curve::fromExisting(Curve const &curve, double yUnits)
{
  *this = curve;

  if (yUnits != 1.)
    for (auto &p : m_curve)
      p.second *= yUnits;
}

void
Curve::add(double val)
{
  for (auto &p : m_curve)
    p.second += val;

  m_oobLeft  += val;
  m_oobRight += val;
}

void
Curve::clear()
{
  m_curve.clear();
  m_oobLeft = m_oobRight = 0;
}

void
Curve::load(std::string const &path, bool transpose, unsigned xCol, unsigned yCol)
{
  rapidcsv::Document doc(
    path,
    rapidcsv::LabelParams(-1, -1),
    rapidcsv::SeparatorParams(','));

  auto rows = doc.GetRowCount();
  auto cols = doc.GetColumnCount();

  clear();

  if (transpose) {
    // Columns are CSV rows, rows are CSV cols
    if (xCol >= rows)
      throw std::runtime_error("Column for X is out of range (file has " + std::to_string(rows) + " data columns)");
    if (yCol >= rows)
      throw std::runtime_error("Column for Y is out of range (file has " + std::to_string(rows) + " data columns)");
    
    for (auto i = 0u; i < cols; ++i) {
      try {
        m_curve[doc.GetCell<double>(i, xCol)] = doc.GetCell<double>(i, yCol);
      } catch (std::invalid_argument const &e) {
        std::string asString = doc.GetCell<std::string>(i, yCol);
        fprintf(
          stderr,
          "warning: %s:row %d: col %d: invalid argument (\"%s\")\n",
          path.c_str(),
          yCol + 1,
          i + 1,
          asString.c_str());
      } catch (std::runtime_error const &e) {
        fprintf(
          stderr,
          "warning: %s:row %d: col %d: out of bounds! (blank line?)\n",
          path.c_str(),
          yCol + 1,
          i + 1);
      }
    }
  } else {
    if (xCol >= cols)
      throw std::runtime_error("Column for X is out of range (file has " + std::to_string(rows) + " data columns)");
    if (yCol >= cols)
      throw std::runtime_error("Column for Y is out of range (file has " + std::to_string(rows) + " data columns)");
    
    for (auto i = 0u; i < rows; ++i) {
      try {
        m_curve[doc.GetCell<double>(xCol, i)] = doc.GetCell<double>(yCol, i);
      } catch (std::invalid_argument const &e) {
        std::string asString = doc.GetCell<std::string>(yCol, i);
        fprintf(
          stderr,
          "warning: %s:row %d: col %d: invalid argument (\"%s\")\n",
          path.c_str(),
          i + 1,
          yCol + 1,
          asString.c_str());
      } catch (std::runtime_error const &e) {
        fprintf(
          stderr,
          "warning: %s:row %d: col %d: out of bounds! (blank line?)\n",
          path.c_str(),
          i + 1,
          yCol + 1);
      }
    }
  }
}

void
Curve::save(std::string const &path) const
{
  FILE *fp;

  fp = fopen(path.c_str(), "wb");
  if (fp == nullptr)
    throw std::runtime_error(
      "Cannot open `" + path + "' for writing: " + strerror(errno));

  for (auto &p : m_curve)
    fprintf(fp, "%.15e, %.15e\n", p.first, p.second);

  fclose(fp);
}

void
Curve::debug()
{
  for (auto p : m_curve)
    printf("%g=%g, ", p.first, p.second);
  putchar(10);
}