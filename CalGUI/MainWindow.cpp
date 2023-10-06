#include "MainWindow.h"
#include "CalculationWorker.h"
#include "./ui_MainWindow.h"
#include <QThread>
#include <QMessageBox>
#include "GUIHelpers.h"
#include <QFileInfo>
#include <QFileDialog>
#include <ZoomableChartWidget.h>
#include <RangeLimitedValueAxis.h>
#include <QLineSeries>
#include <QValueAxis>

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  m_workerThread = new QThread();
  m_calcWorker   = new CalculationWorker();

  m_calcWorker->moveToThread(m_workerThread);
  m_workerThread->start();

  m_openFileDialog = new QFileDialog(this);

  m_openFileDialog->setWindowTitle("Load input spectrum");
  m_openFileDialog->setFileMode(QFileDialog::ExistingFile);
  m_openFileDialog->setAcceptMode(QFileDialog::AcceptOpen);
  m_openFileDialog->setNameFilter(
        "2-column comma-separated values (*.csv);;All files (*)");

  m_blueSNRWidget = new ZoomableChartWidget();

  m_blueX         = new RangeLimitedValueAxis;
  m_blueX->setTitleText("Pixel wavelength [nm]");
  m_blueX->setRange(320, 520);
  m_blueSNRWidget->chart()->addAxis(m_blueX, Qt::AlignBottom);

  m_blueY         = new RangeLimitedValueAxis;
  m_blueY->setRange(0, 32768);
  m_blueY->setTitleText("Counts");
  m_blueSNRWidget->chart()->addAxis(m_blueY, Qt::AlignLeft);

  m_redSNRWidget  = new ZoomableChartWidget();
  m_redX          = new RangeLimitedValueAxis;
  m_redX->setRange(520, 820);
  m_redX->setTitleText("Pixel wavelength [nm]");
  m_redSNRWidget->chart()->addAxis(m_redX, Qt::AlignBottom);

  m_redY          = new RangeLimitedValueAxis;
  m_redY->setRange(0, 32768);
  m_redY->setTitleText("Counts");
  m_redSNRWidget->chart()->addAxis(m_redY, Qt::AlignLeft);

  ui->tabWidget->addTab(
        m_blueSNRWidget,
        "Blue arm");

  ui->tabWidget->addTab(
        m_redSNRWidget,
        "Red arm");

  connectAll();

  refreshUi();

  emit initWorker();
}

MainWindow::~MainWindow()
{
  m_workerThread->quit();
  m_workerThread->wait();
  delete m_workerThread;

  delete ui;
}

void
MainWindow::refreshUi()
{
  double airmass   = m_simParams.airmass;
  double elRad     = .5 * M_PI - acos(1 / airmass);
  double elevation = elRad * 180. / M_PI;

  BLOCKSIG(ui->airmassSpinBox, setValue(airmass));
  BLOCKSIG(ui->elevationSpinBox, setValue(elevation));
  BLOCKSIG(ui->magSpinBox, setValue(m_simParams.rABmag));
  BLOCKSIG(ui->moonSlider, setValue(static_cast<int>(m_simParams.moon)));
  BLOCKSIG(ui->moonSpinBox, setValue(m_simParams.moon));
  BLOCKSIG(ui->sliceSpinBox, setValue(m_simParams.slice));
  BLOCKSIG(ui->exposureTimeSpinBox, setValue(m_simParams.exposure));
}

bool
MainWindow::parse()
{
  QString css, msg;
  bool ok = true;

  m_filePath = ui->inputFileEdit->text();
  QFileInfo info(m_filePath);

  if (!info.exists() || !info.permission(QFile::Permission::ReadUser)) {
    if (m_filePath.isEmpty())
      msg = "No input file specified";
    else
      msg = "Input file \"" + info.fileName() + "\" is not accessible.";

    ui->statusbar->showMessage(msg, 5000);

    css = "background-color: #ff7f7f;";
    ok = false;
  }

  ui->inputFileEdit->setStyleSheet(css);

  m_simParams.airmass  = ui->airmassSpinBox->value();
  m_simParams.detector = "ML15"; // Leave this by default
  m_simParams.exposure = ui->exposureTimeSpinBox->value();
  m_simParams.moon     = ui->moonSpinBox->value();
  m_simParams.progName = "CalGUI";
  m_simParams.rABmag   = ui->magSpinBox->value();
  m_simParams.slice    = ui->sliceSpinBox->value();

  return ok;
}

void
MainWindow::refreshUiState()
{
  ui->sliceSpinBox->setEnabled(!ui->averageSpinBox->isChecked());
}

void
MainWindow::connectAll()
{
  connect(
        this,
        SIGNAL(initWorker()),
        m_calcWorker,
        SLOT(init()));

  connect(
        this,
        SIGNAL(setSimulationParams(SimulationParams)),
        m_calcWorker,
        SLOT(setParams(SimulationParams)));

  connect(
        this,
        SIGNAL(setInputSpectrum(QString)),
        m_calcWorker,
        SLOT(setInputSpectrum(QString)));

  connect(
        this,
        SIGNAL(runSimulation()),
        m_calcWorker,
        SLOT(simulate()));

  connect(
        m_calcWorker,
        SIGNAL(dataProduct(CalculationProduct)),
        this,
        SLOT(onDataProduct(CalculationProduct)));

  connect(
        m_calcWorker,
        SIGNAL(done(QString)),
        this,
        SLOT(onTaskDone(QString)));

  connect(
        m_calcWorker,
        SIGNAL(exception(QString)),
        this,
        SLOT(onTaskException(QString)));

  connect(
        ui->browseButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onBrowseFile()));

  connect(
        ui->actionCalculate,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onSimulate()));

  connect(
        ui->elevationSpinBox,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onElevationChanged()));

  connect(
        ui->airmassSpinBox,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onAirMassChanged()));

  connect(
        ui->moonSpinBox,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onMoonSpinChanged()));

  connect(
        ui->moonSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onMoonSliderChanged()));

  connect(
        ui->inputFileEdit,
        SIGNAL(textEdited(QString)),
        this,
        SLOT(onFileTextEdited()));
}

void
MainWindow::onTaskDone(QString what)
{
  ui->statusbar->showMessage(
        "Background task \"" + what + "\" finished successfully.",
        5000);
}

void
MainWindow::onTaskException(QString what)
{
  QMessageBox::critical(
        this,
        "Task exception",
        "Calculator task failed: " + what);
}

void
MainWindow::onBrowseFile()
{
  if (m_openFileDialog->exec()
      && !m_openFileDialog->selectedFiles().empty()) {
    BLOCKSIG(ui->inputFileEdit, setText(m_openFileDialog->selectedFiles()[0]));
    parse();
  }
}

void
MainWindow::onSimulate()
{
  if (!parse()) {
    QMessageBox::warning(
          this,
          "Simulation parameters",
          "Some parameters are not properly set. Verify the current simulation "
          "parameters and try again.");
    return;
  }

  emit setSimulationParams(m_simParams);
  emit setInputSpectrum(m_filePath);
  emit runSimulation();
}

void
MainWindow::onElevationChanged()
{
  double elevation = ui->elevationSpinBox->value();
  double elRad     = elevation / 180. * M_PI;
  double airmass   = 1. / cos(.5 * M_PI - elRad);

  BLOCKSIG(ui->airmassSpinBox, setValue(airmass));
}

void
MainWindow::onAirMassChanged()
{
  double airmass   = ui->airmassSpinBox->value();
  double elRad     = .5 * M_PI - acos(1 / airmass);
  double elevation = elRad * 180. / M_PI;

  BLOCKSIG(ui->elevationSpinBox, setValue(elevation));
}

void
MainWindow::onMoonSpinChanged()
{
  BLOCKSIG(ui->moonSlider, setValue(static_cast<int>(ui->moonSpinBox->value())));
}

void
MainWindow::onMoonSliderChanged()
{
  BLOCKSIG(ui->moonSpinBox, setValue(ui->moonSlider->value()));
}

void
MainWindow::onFileTextEdited()
{
  if (parse()) {
    QFileInfo info(m_filePath);

    m_openFileDialog->setDirectory(info.dir());
  }
}

void
MainWindow::onDataProduct(CalculationProduct product)
{
  if (product.blueArm.initialized) {
    double max = 1;
    auto series = new QLineSeries;
    series->setName("Signal at pixel");
    series->setColor(QColor(0, 0, 255));

    for (unsigned i = 0; i < DETECTOR_PIXELS; ++i) {
      if (product.blueArm.signal[i] > max)
        max = product.blueArm.signal[i];

      series->append(
            1e9 * product.blueArm.wavelength[i],
            product.blueArm.signal[i]);
    }

    m_blueY->setRange(0, max);

    m_blueSNRWidget->chart()->removeAllSeries();
    m_blueSNRWidget->chart()->addSeries(series);
    series->attachAxis(m_blueX);
    series->attachAxis(m_blueY);
    m_blueSNRWidget->fitInView();
  }

  if (product.redArm.initialized) {
    double max = 1;
    auto series = new QLineSeries;
    series->setName("Signal at pixel");
    series->setColor(QColor(255, 0, 0));
    for (unsigned i = 0; i < DETECTOR_PIXELS; ++i) {
      if (product.redArm.signal[i] > max)
        max = product.redArm.signal[i];
      series->append(
            1e9 * product.redArm.wavelength[i],
            product.redArm.signal[i]);
    }

    m_redY->setRange(0, max);

    m_redSNRWidget->chart()->removeAllSeries();
    m_redSNRWidget->chart()->addSeries(series);
    series->attachAxis(m_redX);
    series->attachAxis(m_redY);
    m_redSNRWidget->fitInView();
  }
}
