#include "MainWindow.h"

#include <QApplication>
#include <DataFileManager.h>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);

  DataFileManager::instance()->addSearchPath("../data");

  MainWindow w;

  w.show();
  return a.exec();
}
