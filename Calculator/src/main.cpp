#include <cstdio>
#include <Curve.h>
#include <Spectrum.h>
#include <ConfigManager.h>
#include <DataFileManager.h>
#include <InstrumentModel.h>
#include <SkyModel.h>
#include <stdexcept>
#include <Detector.h>
#include <Helpers.h>

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
    SkyModel *skyModel = new SkyModel();
    InstrumentModel *model = new InstrumentModel();
    Detector *detector = new Detector();

    skyModel = new SkyModel();

    skyModel->setAirmass(1.5);
    skyModel->setMoon(70);

    Spectrum *spec = skyModel->makeSkySpectrum(testSpectrum);

    spec->save("noisyspec.csv");

    auto detName = model->properties()->detector;

    if (!detector->setDetector(detName))
      throw std::runtime_error("Detector `" + detName + "' not found\n");
    detector->setExposureTime(3600);

    InstrumentArm arm = BlueArm;
    std::string armDesc = arm == BlueArm ? "blue" : "red";

    model->setInput(arm, *spec);

    for (auto i = 0; i < TARSIS_SLICES; ++i) {
      Spectrum *result = model->makePixelPhotonFlux(i);

      result->save("flux_" + std::to_string(i) + ".csv");

      detector->setPixelPhotonFlux(*result);
      detector->signal()->save("counts_" + armDesc + "_" + std::to_string(i) + ".csv");

      delete result;
    }

    delete model;
    delete skyModel;
    delete detector;
    delete spec;

  } catch (std::runtime_error const &e) {
    fprintf(stderr, "Exception: %s\n", e.what());
  }
  return 0;
}
