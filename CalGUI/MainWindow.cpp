#include "MainWindow.h"
#include "CalculationWorker.h"
#include "./ui_MainWindow.h"
#include <QThread>
#include <QMessageBox>
#include "GUIHelpers.h"
#include <QFileInfo>
#include <QFileDialog>

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
