#ifndef TIFFMODEDIALOG_H
#define TIFFMODEDIALOG_H

#include <QDialog>

namespace Ui {
class TiffModeDialog;
}

class TiffModeDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit TiffModeDialog(QWidget *parent = 0);
    ~TiffModeDialog();
    
    int getTiffWriterMode();

private:
    Ui::TiffModeDialog *ui;
};

#endif // TIFFMODEDIALOG_H
