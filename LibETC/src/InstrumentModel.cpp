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

#include <InstrumentModel.h>
#include <DataFileManager.h>
#include <Curve.h>
#include <Spectrum.h>
#include <cmath>
#include <Helpers.h>

//
// Turn FWHM to the inverse of sigma. This speeds up the calculation of
// gaussian functions with varying variance.
//

static inline void
fwhm2invStd(Curve *curve)
{
  // Invert Y axis and scale by VAR2FWHMSQ. Now this is a 1/sigma curve
  curve->invertAxis(YAxis, STD2FWHM);
}

//////////////////////////// InstrumentProperties //////////////////////////////
bool
InstrumentProperties::serialize()
{
  auto &self = *this;

  self["fNum"]         = fNum;
  self["apEfficiency"] = apEfficiency;
  self["sdetector"]    = detector;

  return true;
}

bool
InstrumentProperties::deserialize()
{
  auto &self = *this;

  deserializeField(fNum, "fNum");
  deserializeField(apEfficiency, "apEfficiency");
  deserializeField(detector, "detector");

  return true;
}

///////////////////////////// InstrumentModel //////////////////////////////////
InstrumentModel::InstrumentModel()
{
  for (auto i = 0; i < TARSIS_SLICES; ++i) {
    m_blueDisp[i] = m_blueW2Px[i] = m_bluePx2W[i] = m_blueREPx[i] = nullptr;
    m_redDisp[i]  = m_redW2Px[i]  = m_redPx2W[i]  = m_redREPx[i]  = nullptr;
  }

  m_properties    = &ConfigManager::get<InstrumentProperties>("tarsis");

  m_attenSpectrum = new Spectrum();

  m_blueML15      = new Curve();
  m_blueNBB       = new Curve();
  m_redML15       = new Curve();

  m_blueML15->load(dataFile("blueTransmission.csv"), true, 0, 1);
  m_blueML15->scaleAxis(XAxis, 1e-9);

  m_blueNBB->load(dataFile("blueTransmission.csv"), true, 0, 2);
  m_blueNBB->scaleAxis(XAxis, 1e-9);

  m_redML15->load(dataFile("redTransmission.csv"), true);
  m_redML15->scaleAxis(XAxis, 1e-9);

  for (auto i = 0; i < TARSIS_SLICES; ++i) {
    // Units of this datafile are nm -> nm/px
    // We invert the Y axis of the curve to have (m -> px / m)
    m_blueDisp[i] = new Curve();
    m_blueDisp[i]->load(dataFile("dispersionBlue.csv"), true, 0, i + 1);
    m_blueDisp[i]->extendRight();
    m_blueDisp[i]->extendLeft();
    m_blueDisp[i]->scaleAxis(XAxis, 1e-9);
    m_blueDisp[i]->scaleAxis(YAxis, 1e-9);
    m_blueDisp[i]->invertAxis(YAxis);

    // Units of this datafile are nm -> px
    m_blueREPx[i] = new Curve();
    m_blueREPx[i]->load(dataFile("pxResolutionBlue.csv"), true, 0, i + 1);
    m_blueREPx[i]->extendRight();
    m_blueREPx[i]->extendLeft();
    m_blueREPx[i]->scaleAxis(XAxis, 1e-9);
    
    fwhm2invStd(m_blueREPx[i]);

    // We want to have a curve that connects wavelengths to pixels, therefore:
    // 1. We assign the m_blueDisp[i] to it (m -> px/m)
    // 2. We integrate the curve. Now we have (m -> px)
    m_blueW2Px[i] = new Curve();
    m_blueW2Px[i]->assign(*m_blueDisp[i]);
    m_blueW2Px[i]->integrate();

    // We now want the pixel-to-wavelength relationship. Easy. Just flip X and Y
    m_bluePx2W[i] = new Curve();
    m_bluePx2W[i]->assign(*m_blueW2Px[i]);
    m_bluePx2W[i]->flip();

    ///////////////////////////// Repeat for red ///////////////////////////////
    // Units of this datafile are nm -> nm/px
    // We invert the Y axis of the curve to have (m -> px / m)
    m_redDisp[i] = new Curve();
    m_redDisp[i]->load(dataFile("dispersionRed.csv"), true, 0, i + 1);
    m_redDisp[i]->extendRight();
    m_redDisp[i]->extendLeft();
    m_redDisp[i]->scaleAxis(XAxis, 1e-9);
    m_redDisp[i]->scaleAxis(YAxis, 1e-9);
    m_redDisp[i]->invertAxis(YAxis);

    // Units of this datafile are nm -> px
    m_redREPx[i] = new Curve();
    m_redREPx[i]->load(dataFile("pxResolutionRed.csv"), true, 0, i + 1);
    m_redREPx[i]->extendRight();
    m_redREPx[i]->extendLeft();
    m_redREPx[i]->scaleAxis(XAxis, 1e-9);

    fwhm2invStd(m_redREPx[i]);

    // We want to have a curve that connects wavelengths to pixels, therefore:
    // 1. We assign the m_redDisp[i] to it (m -> px/m)
    // 3. We integrate the curve. Now we have (m -> px)
    m_redW2Px[i] = new Curve();
    m_redW2Px[i]->assign(*m_redDisp[i]);
    m_redW2Px[i]->integrate();

    // We now want the pixel-to-wavelength relationship. Easy. Just flip X and Y
    m_redPx2W[i] = new Curve();
    m_redPx2W[i]->assign(*m_redW2Px[i]);
    m_redPx2W[i]->flip();
  }
}

InstrumentModel::~InstrumentModel()
{
  if (m_blueML15 != nullptr)
    delete m_blueML15;

  if (m_blueNBB != nullptr)
    delete m_blueNBB;

  if (m_redML15 != nullptr)
    delete m_redML15;

  if (m_attenSpectrum != nullptr)
    delete m_attenSpectrum;

  for (auto i = 0; i < TARSIS_SLICES; ++i) {
    // Delete blue curves
    if (m_blueDisp[i] != nullptr)
      delete m_blueDisp[i];

    if (m_blueREPx[i] != nullptr)
      delete m_blueREPx[i];

    if (m_blueW2Px[i] != nullptr)
      delete m_blueW2Px[i];

    if (m_bluePx2W[i] != nullptr)
      delete m_bluePx2W[i];

    // Delete red curves
    if (m_redDisp[i] != nullptr)
      delete m_redDisp[i];

    if (m_redREPx[i] != nullptr)
      delete m_redREPx[i];

    if (m_redW2Px[i] != nullptr)
      delete m_redW2Px[i];

    if (m_redPx2W[i] != nullptr)
      delete m_redPx2W[i];
  }
}

// Returns instrument properties (required for simulation)
InstrumentProperties *
InstrumentModel::properties() const
{
  return m_properties;
}

// Turns a pixel into lambda
double
InstrumentModel::pxToWavelength(
  InstrumentArm arm,
  unsigned slice,
  unsigned pixel) const
{
  if (slice >= TARSIS_SLICES)
    throw std::runtime_error("Slice " + std::to_string(slice + 1) + " out of bounds");

  switch (arm) {
    case BlueArm:
      return (*m_bluePx2W[slice])(pixel);

    case RedArm:
      return (*m_redPx2W[slice])(pixel);
  }

  return std::numeric_limits<double>::quiet_NaN();
}

Curve *
InstrumentModel::pxToWavelength(unsigned slice) const
{
  if (slice >= TARSIS_SLICES)
    throw std::runtime_error("Slice " + std::to_string(slice + 1) + " out of bounds");

  switch (m_currentPath) {
    case BlueArm:
      return m_bluePx2W[slice];

    case RedArm:
      return m_redPx2W[slice];
  }

  return nullptr;
}

int
InstrumentModel::wavelengthToPx(
  InstrumentArm arm,
  unsigned slice,
  double lambda) const
{
  if (slice >= TARSIS_SLICES)
    throw std::runtime_error("Slice " + std::to_string(slice + 1) + " out of bounds");

  switch (arm) {
    case BlueArm:
      return (*m_blueW2Px[slice])(lambda);

    case RedArm:
      return (*m_redW2Px[slice])(lambda);
  }

  return -1;
}

Curve *
InstrumentModel::wavelengthToPx(unsigned slice) const
{
  if (slice >= TARSIS_SLICES)
    throw std::runtime_error("Slice " + std::to_string(slice + 1) + " out of bounds");

  switch (m_currentPath) {
    case BlueArm:
      return m_blueW2Px[slice];

    case RedArm:
      return m_redW2Px[slice];
  }

  return nullptr;
}

// Applies the spectrum. The input spectrum must be in radiance units,
// with a *frequency* spectral axis* i.e. J / (s * m^2 * sr * Hz)
void
InstrumentModel::setInput(InstrumentArm arm, Spectrum const &input)
{
  double lightConeSr;
  double apertureAngRadius;
  double totalScale;
  const Curve *transmission = nullptr;
  
  std::string const &det = m_properties->detector;

  switch (arm) {
    case BlueArm:
      if (det == "ML15")
        transmission = m_blueML15;
      else if (det == "NBB")
        transmission = m_blueNBB;
      else
        throw std::runtime_error("Unknown detector for blue arm: `" + det + "'");
      break;

    case RedArm:
      if (det == "ML15")
        transmission = m_redML15;
      else
        throw std::runtime_error("Unknown detector for red arm: `" + det + "'");
      break;

    default:
      throw std::runtime_error("Invalid arm configuration");
  }

  // 
  // Simulation is a multi-step process that involves:
  //
  // 1. Turn radiance spectrum into irradiance spectrum. We achieve this by
  //    multiplying the radiance by a light cone of f/#.
  // 2. Attenuate spectrum by the aperture efficiency
  // 3. Attenuate by the instrument curve
  // 4. Save to m_attenSpectrum
  //

  apertureAngRadius = atan(.5 / m_properties->fNum);
  lightConeSr       = M_PI * apertureAngRadius * apertureAngRadius;
  totalScale        = lightConeSr * m_properties->apEfficiency;

  m_currentPath     = arm;

  m_attenSpectrum->fromExisting(input);          // Set input radiance
  m_attenSpectrum->scaleAxis(YAxis, totalScale); // To irradiance
  m_attenSpectrum->multiplyBy(*transmission);    // Attenuate by transmission
}

static inline double
convolveAround(
  Curve const &curve,
  double x0,
  double invSigma,
  unsigned int oversample = 11)
{
  double scale    = 0;
  double halfPrec = .5 * invSigma * invSigma;
  double dx       = STD2FWHM / (invSigma * oversample);
  double y        = 0;

  int halfWidth = oversample / 2;

  for (int i = -halfWidth; i <= halfWidth; ++i) {
    double offset = i * dx;
    double x = x0 + offset;
    double weight = exp(-halfPrec * offset * offset);
    y     += curve(x) * weight;
    scale += weight;
  }

  y /= scale;

  return y;
}

// Returns the per-pixel photon flux,in units of in ph / (s m^2)
Spectrum *
InstrumentModel::makePixelPhotonFlux(unsigned int slice) const
{
  Spectrum dispSpectrum;

  Curve const *w2pxPtr  = nullptr;
  Curve const *px2wPtr  = nullptr;
  Curve const *resElPtr = nullptr;
  Curve const *dispPtr  = nullptr;

  switch (m_currentPath) {
    case BlueArm:
      dispPtr  = m_blueDisp[slice];
      w2pxPtr  = m_blueW2Px[slice];
      px2wPtr  = m_bluePx2W[slice];
      resElPtr = m_blueREPx[slice];
      break;

    case RedArm:
      dispPtr  = m_redDisp[slice];
      w2pxPtr  = m_redW2Px[slice];
      px2wPtr  = m_redPx2W[slice];
      resElPtr = m_redREPx[slice];
      break;
  }

  Curve const &w2px  = *w2pxPtr;
  Curve const &px2w  = *px2wPtr;
  Curve const &resEl = *resElPtr;
  Curve const &disp  = *dispPtr;

  //
  // This operates on the attenuated spectrum and involves:
  //
  // 1. Perform dispersion. This is basically scaling the X axis of the
  //    attenuated spectrum by the wavelengt-to-pixel curve.
  // 2. Create a pixel spectrum, ranging from 0 to SPECTRAL_PIXEL_LENGTH
  // 3. Apply a non-LTI convolution by a gaussian given by m_*REPx, from
  // 4. Convert power to photons by means of the planck constant. Note that
  //    ph = E / (hf) = E lambda / hc

  dispSpectrum.fromExisting(*m_attenSpectrum);
  dispSpectrum.scaleAxis(XAxis, w2px, disp);

  Spectrum *pixelFlux = new Spectrum();
  auto &pxFRef = *pixelFlux;

  for (auto i = 0; i < SPECTRAL_PIXEL_LENGTH; ++i) {
    double wl = px2w(i);
    double toPhotons = wl / (PLANCK_CONSTANT * SPEED_OF_LIGHT);

    if (!std::isnan(wl))
      pxFRef[i] = convolveAround(dispSpectrum, i, resEl(wl)) * toPhotons;
  }

  return pixelFlux;
}

