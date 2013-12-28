#include "TiffModeDialog.h"
#include "ui_TiffModeDialog.h"

TiffModeDialog::TiffModeDialog(bool hdrMode, QWidget *parent)
    : QDialog(parent)
    , m_hdrMode(hdrMode)
    , m_ui(new Ui::TiffModeDialog)
{
    m_ui->setupUi(this);

    if ( m_hdrMode ) {
        m_ui->comboBox->insertItem(0, "TIFF 32 bit/channel floating point");
        m_ui->comboBox->insertItem(1, "TIFF LogLuv");
    } else {
        m_ui->comboBox->insertItem(0, "TIFF 8 bit/channel");
        m_ui->comboBox->insertItem(1, "TIFF 16 bit/channel");
        m_ui->comboBox->insertItem(2, "TIFF 32 bit/channel floating point");
    }

#ifdef Q_OS_MAC
    this->setWindowModality(Qt::WindowModal); // In OS X, the QMessageBox is modal to the window
#endif
}

TiffModeDialog::~TiffModeDialog()
{
    delete m_ui;
}

int TiffModeDialog::getTiffWriterMode()
{
    if ( m_hdrMode ) {
        return m_ui->comboBox->currentIndex() + 2;
    } else {
        return m_ui->comboBox->currentIndex();
    }
}
