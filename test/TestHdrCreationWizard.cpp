#include <QApplication>
#include <QScopedPointer>

#include "HdrWizard/HdrWizard.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    QScopedPointer<HdrWizard> wizard( new HdrWizard(0, QStringList(), QStringList(), QVector<float>()) );
    /* if ( */
    wizard->exec();
    /* == QDialog::Accepted)
    {
        // emit load_success(wizard->getPfsFrameHDR(), wizard->getCaptionTEXT(), wizard->getInputFilesNames(), true);
    }
    */

    app.exec();
}
