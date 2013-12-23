#ifndef ECWIN7_H
#define ECWIN7_H

#include <QtGlobal>
#include <QWidget>

// Windows only data definitions
#ifdef Q_OS_WIN

#include <QtWinExtras/QWinTaskbarButton>
#include <QtWinExtras/QWinTaskbarProgress>
#include <QtWinExtras/QWinJumpList>
#include <QtWinExtras/QWinJumpListCategory>

#endif

class EcWin7
{
public:

	// Initialization methods
    EcWin7();
    void init(QWidget* wid);
    void addRecentFile(const QString& filename);

    void setProgressValue(int value, int max);

private:
    void associateFileTypes(const QStringList &fileTypes);

    HWND mWindowId;

    QWinTaskbarButton* taskbarButton;
    QWinTaskbarProgress* taskbarProgress;
    QWinJumpList* jumplist;
};

#endif // ECWIN7_H
