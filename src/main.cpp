#include "mainwindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("imageviewer"));
    QApplication::setOrganizationName(QStringLiteral("imageviewer"));

    const QString initial = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QString();
    imageviewer::MainWindow window(initial);
    return app.exec();
}
