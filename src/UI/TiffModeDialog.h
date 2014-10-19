#ifndef TIFFMODEDIALOG_H
#define TIFFMODEDIALOG_H

#include <QDialog>
#include <QScopedPointer>

#include "Common/LuminanceOptions.h"

namespace Ui
{
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
    QScopedPointer<Ui::TiffModeDialog> m_ui;
    QScopedPointer<LuminanceOptions> m_options;
};

#endif // TIFFMODEDIALOG_H
