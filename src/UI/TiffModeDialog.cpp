#include "TiffModeDialog.h"
#include "ui_TiffModeDialog.h"

namespace
{
const static QString TIFF_MODE_HDR_KEY = "tiffmodedialog/mode/hdr";
const static int TIFF_MODE_HDR_VALUE = 0;

const static QString TIFF_MODE_LDR_KEY = "tiffmodedialog/mode/ldr";
const static int TIFF_MODE_LDR_VALUE = 0;
}

TiffModeDialog::TiffModeDialog(bool hdrMode, QWidget *parent)
    : QDialog(parent)
    , m_hdrMode(hdrMode)
    , m_ui(new Ui::TiffModeDialog)
    , m_options(new LuminanceOptions())
{
    m_ui->setupUi(this);

    if (m_hdrMode)
    {
        m_ui->comboBox->insertItem(0, "TIFF 32 bit/channel floating point");
        m_ui->comboBox->insertItem(1, "TIFF LogLuv");

        m_ui->comboBox->setCurrentIndex(m_options->value(TIFF_MODE_HDR_KEY, TIFF_MODE_HDR_VALUE).toInt());
    }
    else
    {
        m_ui->comboBox->insertItem(0, "TIFF 8 bit/channel");
        m_ui->comboBox->insertItem(1, "TIFF 16 bit/channel");
        m_ui->comboBox->insertItem(2, "TIFF 32 bit/channel floating point");

        m_ui->comboBox->setCurrentIndex(m_options->value(TIFF_MODE_LDR_KEY, TIFF_MODE_LDR_VALUE).toInt());
    }

#ifdef Q_OS_MAC
    this->setWindowModality(Qt::WindowModal); // In OS X, the QMessageBox is modal to the window
#endif
}

TiffModeDialog::~TiffModeDialog()
{
    if (m_hdrMode)
    {
        m_options->setValue(TIFF_MODE_HDR_KEY, m_ui->comboBox->currentIndex());
    }
    else
    {
        m_options->setValue(TIFF_MODE_LDR_KEY, m_ui->comboBox->currentIndex());
    }
}

int TiffModeDialog::getTiffWriterMode()
{
    if (m_hdrMode)
    {
        return m_ui->comboBox->currentIndex() + 2;
    }
    else
    {
        return m_ui->comboBox->currentIndex();
    }
}
