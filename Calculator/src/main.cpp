#include <cstdio>
#include <Curve.h>
#include <Spectrum.h>
#include <ConfigManager.h>
#include <DataFileManager.h>
#include <InstrumentModel.h>
#include <stdexcept>
#include <Detector.h>

#define JANSKY 1e-26         // W / (m^2 Hz)
#define ARCSEC 4.8481368e-06 // radian

static inline double
surfaceBrightnessAB2radiance(double mag, double wl)
{
  double fnu     = pow(10, -.4 * mag) * 3631 * JANSKY / (ARCSEC * ARCSEC);
  double flambda = SPEED_OF_LIGHT / (wl * wl) * fnu;

  return flambda;
}

static inline double
surfaceBrightnessVega2radiance(double mag, double wl)
{
  double flambda     = pow(10, -.4 * mag) * 0.03631 / (ARCSEC * ARCSEC);
  return flambda;
}

int
main(void)
{
  Spectrum testSpectrum;
  DataFileManager::instance()->addSearchPath("../data");

  for (auto i = 320; i < 820; i += 10) {
    double wl = i * 1e-9;
    testSpectrum[wl] = surfaceBrightnessAB2radiance(17.8, wl);
  }
  
  testSpectrum.save("input.csv");

  try {
    Detector *detector = new Detector();
    InstrumentModel *model = new InstrumentModel();

    auto detName = model->properties()->detector;

    if (!detector->setDetector(detName))
      throw std::runtime_error("Detector `" + detName + "' not found\n");
    detector->setExposureTime(3600);

    InstrumentArm arm = BlueArm;
    std::string armDesc = arm == BlueArm ? "blue" : "red";

    model->setInput(arm, testSpectrum);

    for (auto i = 0; i < TARSIS_SLICES; ++i) {
      Spectrum *result = model->makePixelPhotonFlux(i);

      result->save("flux_" + std::to_string(i) + ".csv");

      detector->setPixelPhotonFlux(*result);
      detector->signal()->save("counts_" + armDesc + "_" + std::to_string(i) + ".csv");

      delete result;
    }

    delete model;
  } catch (std::runtime_error const &e) {
    fprintf(stderr, "Exception: %s\n", e.what());
  }
  return 0;
}
