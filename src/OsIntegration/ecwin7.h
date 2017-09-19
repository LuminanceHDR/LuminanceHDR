#ifndef ECWIN7_H
#define ECWIN7_H

#include <QWidget>
#include <QtGlobal>

#include <QtWinExtras/QWinJumpList>
#include <QtWinExtras/QWinJumpListCategory>
#include <QtWinExtras/QWinTaskbarButton>
#include <QtWinExtras/QWinTaskbarProgress>

class EcWin7 {
   public:
    // Initialization methods
    EcWin7();
    void init(QWidget *wid);
    void addRecentFile(const QString &filename);

    void setProgressValue(int value, int max);

   private:
    void associateFileTypes(const QStringList &fileTypes);

    QWinTaskbarButton *taskbarButton;
    QWinTaskbarProgress *taskbarProgress;
    QWinJumpList *jumplist;
};

#endif  // ECWIN7_H
