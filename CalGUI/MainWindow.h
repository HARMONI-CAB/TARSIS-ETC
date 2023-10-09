#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <Simulation.h>
#include <CalculationWorker.h>
#include <list>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QFileDialog;
class ZoomableChartWidget;
class QValueAxis;

class MainWindow : public QMainWindow
{
  Q_OBJECT

  CalculationWorker   *m_calcWorker = nullptr;
  QThread             *m_workerThread = nullptr;
  bool                 m_haveClickedPoint = false;
  QPointF              m_clickedPoint;

  std::list<CalculationProduct>
                       m_lastProducts;

  ZoomableChartWidget *m_blueSNRWidget = nullptr;
  QValueAxis          *m_blueX = nullptr;
  QValueAxis          *m_blueY = nullptr;

  ZoomableChartWidget *m_redSNRWidget = nullptr;
  QValueAxis          *m_redX = nullptr;
  QValueAxis          *m_redY = nullptr;

  SimulationParams    m_simParams;
  QString             m_filePath;
  QFileDialog        *m_openFileDialog = nullptr;
  QFileDialog        *m_saveFileDialog = nullptr;
  void                refreshUiState();
  void                connectAll();
  bool                parse();
  void                refreshUi();
  void                resetMeasurements();
  void                refreshMeasurements();
  bool                saveDataProduct(std::string const &);

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
  void onDataProduct(CalculationProduct);

  void onBrowseFile();
  void onSimulate();
  void onSaveProduct();
  void onClearPlots();
  void onPlotPointClicked(QPointF);

  void onElevationChanged();
  void onAirMassChanged();

  void onMoonSpinChanged();
  void onMoonSliderChanged();

  void onFileTextEdited();

};
#endif // MAINWINDOW_H
