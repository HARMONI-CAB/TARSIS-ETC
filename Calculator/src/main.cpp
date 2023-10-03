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
#include <getopt.h>

void
help(const char *progName)
{
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "\t%s [OPTIONS] SPECTRUM-FILE\n\n", progName);
  fprintf(stderr, "Calculates the SNR of a surface brightness spectrum\n");
  fprintf(stderr, "as seen by TARSIS. SPECTRUM-FILE must be a CSV-formatted surface\n");
  fprintf(stderr, "brightness (radiance) spectrum, specified in the wavelength\n");
  fprintf(stderr, "axis (nm). \n\nOPTIONS can be any of the following:\n");
  fprintf(stderr, "\t-a, --airmass [AIRMASS]    Set airmass (default is 1)\n");
  fprintf(stderr, "\t-d, --detector [DET]       Set detector to DET (ML15 or NBB, default is ML15)\n");
  fprintf(stderr, "\t-e, --elevation [ANGLE]    Set elevation angle (same as -z 90-ANGLE,\n");
  fprintf(stderr, "\t                           default is 90)\n");
  fprintf(stderr, "\t-m, --magnitude [MAGR_AB]  Normalize spectrum to the specified R(AB)\n");
  fprintf(stderr, "\t                           magnitude (default is 18 mag/arcsec^2)\n");
  fprintf(stderr, "\t-M, --moon [PERCENT]       Set moon illumination, being 0 new\n");
  fprintf(stderr, "\t                           moon and 100 full moon (default is 0)\n");
  fprintf(stderr, "\t-s, --slice [SLICE]        Slice at which calculations are to be\n");
  fprintf(stderr, "\t                           done (from 1 to 40, default is 20)\n");
  fprintf(stderr, "\t-t, --exposure [TIME]      Set exposure time, in seconds (default\n");
  fprintf(stderr, "\t                           is 3600 seconds)\n");
  fprintf(stderr, "\t-z, --zenith [ANGLE]       Specify airmass from the zenith angle\n\n");
  fprintf(stderr, "\t--help                     This help\n");
}

struct SimulationParams {
  const char *progName = nullptr;
  std::string detector = "ML15";
  double airmass       = 1.;
  double moon          = 0.;
  double exposure      = 3600;
  double rABmag        = 18.;
  int    slice         = 20;
};

class Simulation {
    Spectrum  m_input;
    Curve     m_cousinsR;
    double    m_cousinsREquivBw;
    double    m_cousinsREffLambda;
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
    double pxToWavelength(unsigned px) const;
};

Simulation::Simulation()
{
  // Init model
  m_skyModel    = new SkyModel();
  m_det         = new Detector();
  m_tarsisModel = new InstrumentModel();

  // 
  // http://svo2.cab.inta-csic.es/theory/fps/index.php?id=Generic/Cousins.R&&mode=browse&gname=Generic&gname2=Cousins
  //

  m_cousinsR.load(dataFile("Generic_Cousins.R.dat"));
  m_cousinsR.scaleAxis(XAxis, 1e-10); // X axis was in angstrom
  m_cousinsREquivBw   = m_cousinsR.integral();
  m_cousinsREffLambda = m_cousinsR.distMean();
}

Simulation::~Simulation()
{
  if (m_skyModel != nullptr)
    delete m_skyModel;

  if (m_tarsisModel != nullptr)
    delete m_tarsisModel;

  if (m_det != nullptr)
    delete m_det;

  if (m_sky != nullptr)
    delete m_sky;
}

void
Simulation::setInput(Spectrum const &spec)
{
  m_input = spec;
  m_input.save("asloaded.csv");
}

// Compute spectrum's R magnitude
void
Simulation::normalizeToRMag(double R)
{
  Spectrum filtered;
  double meanSB, desiredSB;

  //
  // The following calculation departs from the following assumption. If we
  // feed the R filter by a flat radiance of 0 magAB (/arcsec^2), the resulting
  // R mag must also be 0 magAB,R (/arcsec^2)
  //

  // This is the desired radiance, normalized by the R transfer
  desiredSB = surfaceBrightnessAB2radiance(R, m_cousinsREffLambda);

  // Input is assumed to be in W / (m^2 sr m)
  filtered.fromExisting(m_input);
  filtered.multiplyBy(m_cousinsR);

  // Again, W / (m^2 sr m). But this time, normalized by the R transfer.
  meanSB = filtered.integral() / m_cousinsREquivBw;
  // And normalize

  m_input.scaleAxis(YAxis, desiredSB / meanSB);
}

void
Simulation::setParams(SimulationParams const &params)
{
  m_params = params;

  m_skyModel->setAirmass(params.airmass);
  m_skyModel->setMoon(params.moon);
  
  if (m_sky != nullptr) {
    delete m_sky;
    m_sky = nullptr;
  }

  // Update sky spectrum
  m_sky = m_skyModel->makeSkySpectrum(m_input);
  
  m_det->setDetector(params.detector);
  m_det->setExposureTime(params.exposure);

  auto prop = m_tarsisModel->properties();
  prop->detector = params.detector;
}

void
Simulation::simulateArm(InstrumentArm arm)
{
  Spectrum *flux = nullptr;

  try {
    m_tarsisModel->setInput(arm, *m_sky);
    flux = m_tarsisModel->makePixelPhotonFlux(m_params.slice);
    m_det->setPixelPhotonFlux(*flux);
    delete flux;
    flux = nullptr;
  } catch (std::runtime_error const &e) {
    if (flux != nullptr)
      delete flux;
    throw;
  } 

  if (flux != nullptr)
    delete flux;
}

double
Simulation::signal(unsigned px) const
{
  return m_det->signal(px);
}

double
Simulation::noise(unsigned px) const
{
  return m_det->noise(px);
}

double
Simulation::pxToWavelength(unsigned px) const
{
  return (*m_tarsisModel->pxToWavelength(m_params.slice))(px);
}

bool
runSimulation(SimulationParams const &params, std::string const &path)
{
  bool ok = false;
  Spectrum input;
  Simulation *sim = nullptr;

  DataFileManager::instance()->addSearchPath("../data");

  try {
    sim = new Simulation();

    // Init data
    input.load(path);
    input.scaleAxis(XAxis, 1e-9);

    sim->setInput(input);
    sim->normalizeToRMag(params.rABmag);

    sim->setParams(params);
    sim->simulateArm(BlueArm);
    
    for (auto i = 0; i < 2048; ++i)
      printf("%s%g", i > 0 ? "," : "", sim->pxToWavelength(i));
    putchar('\n');

    for (auto i = 0; i < 2048; ++i)
      printf("%s%g", i > 0 ? "," : "", sim->signal(i));
    putchar('\n');

    for (auto i = 0; i < 2048; ++i)
      printf("%s%g", i > 0 ? "," : "", sim->noise(i));
    putchar('\n');

    if (params.detector == "ML15") {
      sim->simulateArm(RedArm);

      for (auto i = 0; i < 2048; ++i)
        printf("%s%g", i > 0 ? "," : "", sim->pxToWavelength(i));
      putchar('\n');

      for (auto i = 0; i < 2048; ++i)
        printf("%s%g", i > 0 ? "," : "", sim->signal(i));
      putchar('\n');


      for (auto i = 0; i < 2048; ++i)
        printf("%s%g", i > 0 ? "," : "", sim->noise(i));
      putchar('\n');
    }

    ok = true;
  } catch (std::runtime_error const &e) {
    fprintf(
      stderr,
      "%s: simulation exception: %s\n",
      params.progName,
      e.what());
  }

  if (sim != nullptr)
    delete sim;
  
  return ok;
}

int
main(int argc, char **argv)
{
  SimulationParams params;
  const char* const short_opt = "a:e:M:m:s:t:z:h";
  double angle;
  int opt;
  const option long_opt[] = {
    {"airmass",         required_argument, nullptr, 'a'},
    {"elevation",       required_argument, nullptr, 'e'},
    {"magnitude",       required_argument, nullptr, 'm'},
    {"moon",            required_argument, nullptr, 'M'},
    {"slice",           required_argument, nullptr, 's'},
    {"exposure",        required_argument, nullptr, 't'},
    {"zenith-distance", required_argument, nullptr, 'z'},
    {"help",            no_argument,       nullptr, 'h'},
    {nullptr,           no_argument,       nullptr, 0}
  };

  params.progName = argv[0];

  while ((opt = getopt_long(argc, argv, short_opt, long_opt, nullptr)) != -1) {
    switch (opt) {
      case 'a':
        if (sscanf(optarg, "%lg", &params.airmass) < 1) {
          fprintf(stderr, "%s: invalid airmass `%s'\n", argv[0], optarg);
          goto bad_option;
        }
        
        if (params.airmass < 1) {
          fprintf(stderr, "%s: airmass `%s' out of bounds\n", argv[0], optarg);
          goto bad_option;
        }
        break;

      case 'd':
        params.detector = optarg;
        break;

      case 'e':
        if (sscanf(optarg, "%lg", &angle) < 1) {
          fprintf(stderr, "%s: invalid elevation angle `%s'\n", argv[0], optarg);
          goto bad_option;
        }

        if (angle < 0 || angle > 90) {
          fprintf(stderr, "%s: elevation angle `%s' out of bounds\n", argv[0], optarg);
          goto bad_option;
        }

        params.airmass = 1. / cos((90 - angle) * M_PI / 180.);
        break;
      
      case 'm':
        if (sscanf(optarg, "%lg", &params.rABmag) < 1) {
          fprintf(stderr, "%s: invalid r(AB) magnitude `%s'\n", argv[0], optarg);
          goto bad_option;
        }
        break;
      
      case 'M':
        if (sscanf(optarg, "%lg", &params.moon) < 1) {
          fprintf(stderr, "%s: invalid moon illumination `%s'\n", argv[0], optarg);
          goto bad_option;
        }

        if (params.moon < 0 || params.moon > 100) {
          fprintf(stderr, "%s: moon illumination `%s' out of bounds\n", argv[0], optarg);
          goto bad_option;
        }
        break;

      case 's':
        if (sscanf(optarg, "%u", &params.slice) < 1) {
          fprintf(stderr, "%s: invalid slice number `%s'\n", argv[0], optarg);
          goto bad_option;
        }

        if (params.slice < 1 || params.slice > 40) {
          fprintf(stderr, "%s: slice `%s' out of bounds\n", argv[0], optarg);
          goto bad_option;
        }
        break;

      case 't':
        if (sscanf(optarg, "%lg", &params.exposure) < 1) {
          fprintf(stderr, "%s: invalid exposure time `%s'\n", argv[0], optarg);
          goto bad_option;
        }

        if (params.exposure < 0) {
          fprintf(stderr, "%s: exposure time `%s' out of bounds\n", argv[0], optarg);
          goto bad_option;
        }
        break;

      case 'z':
        if (sscanf(optarg, "%lg", &angle) < 1) {
          fprintf(stderr, "%s: invalid zenith distance `%s'\n", argv[0], optarg);
          goto bad_option;
        }

        if (angle < 0 || angle > 90) {
          fprintf(stderr, "%s: zenith distance `%s' out of bounds\n", argv[0], optarg);
          goto bad_option;
        }

        params.airmass = 1. / cos(angle * M_PI / 180.);
        break;

      case 'h':
        help(argv[0]);
        exit(EXIT_SUCCESS);

      case '?':
        help(argv[0]);
        goto bad_option;
    }
  }

  if (optind == argc) {
    fprintf(stderr, "%s: no input spectrum specified\n", argv[0]);
    help(argv[0]);
    exit(EXIT_FAILURE);
  }

  if (optind + 1 < argc) {
    fprintf(stderr, "%s: too many input files\n", argv[0]);
    help(argv[0]);
    exit(EXIT_FAILURE);
  }

  if (!runSimulation(params, argv[optind]))
    exit(EXIT_FAILURE);
  
  exit(EXIT_SUCCESS);

bad_option:
  fprintf(stderr, "Type `%s --help` for help\n", argv[0]);
  exit(EXIT_FAILURE);
}