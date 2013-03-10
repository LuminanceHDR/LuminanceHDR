#include "TiffModeDialog.h"
#include "ui_TiffModeDialog.h"

TiffModeDialog::TiffModeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TiffModeDialog)
{
    ui->setupUi(this);
}

TiffModeDialog::~TiffModeDialog()
{
    delete ui;
}

int TiffModeDialog::getTiffWriterMode()
{
    return ui->comboBox->currentIndex();
}
