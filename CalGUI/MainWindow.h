#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <Simulation.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class CalculationWorker;
class QFileDialog;

class MainWindow : public QMainWindow
{
  Q_OBJECT

  CalculationWorker *m_calcWorker = nullptr;
  QThread           *m_workerThread = nullptr;
  SimulationParams   m_simParams;
  QString            m_filePath;
  QFileDialog       *m_openFileDialog = nullptr;
  void               refreshUiState();
  void               connectAll();
  bool               parse();
  void               refreshUi();

public:
  MainWindow(QWidget *parent = nullptr);
  virtual ~MainWindow() override;

private:
  Ui::MainWindow *ui;

signals:
  void initWorker();
  void setSimulationParams(SimulationParams);
  void setInputSpectrum(QString);
  void runSimulation();

public slots:
  void onTaskDone(QString);
  void onTaskException(QString);

  void onBrowseFile();
  void onSimulate();

  void onElevationChanged();
  void onAirMassChanged();

  void onMoonSpinChanged();
  void onMoonSliderChanged();

  void onFileTextEdited();
};
#endif // MAINWINDOW_H
