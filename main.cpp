#include <QApplication>
#include "sis.h"

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  SIS w;
  w.show();
  
  return a.exec();
}
