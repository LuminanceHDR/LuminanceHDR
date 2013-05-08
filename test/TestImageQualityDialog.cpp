#include <QtGui/QApplication>

#include <Libpfs/frame.h>
#include <UI/ImageQualityDialog.h>

using namespace pfs;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    pfs::Frame image(100, 100);

    Channel* R;
    Channel* G;
    Channel* B;
    image.createXYZChannels(R, G, B);

    ImageQualityDialog d(&image, "jpg");
    d.show();

    return a.exec();
}
