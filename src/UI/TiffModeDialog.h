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
    explicit TiffModeDialog(bool hdrMode = false, QWidget *parent = 0);
    ~TiffModeDialog();
    
    int getTiffWriterMode();

private:
    bool m_hdrMode;
    Ui::TiffModeDialog *m_ui;
};

#endif // TIFFMODEDIALOG_H
