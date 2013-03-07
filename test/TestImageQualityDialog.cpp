#include <QtGui/QApplication>

#include "UI/ImageQualityDialog.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QImage image(100, 100, QImage::Format_RGB32);

    ImageQualityDialog d(&image, "jpg");
    d.show();

    return a.exec();
}
