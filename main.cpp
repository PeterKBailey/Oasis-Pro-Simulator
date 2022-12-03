#include "mainwindow.h"
#include "device.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    auto d = new Device();
    MainWindow w(d);
    w.show();
    return a.exec();
}
