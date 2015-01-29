#include <QApplication>
//#include "mainwindow.h"
#include "OpenCVWidget.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    OpenCVWidget w;
    w.show();

    return a.exec();
}
