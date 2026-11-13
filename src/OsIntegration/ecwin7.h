#ifndef ECWIN7_H
#define ECWIN7_H

#include <QWidget>
#include <QtGlobal>

#include <shobjidl.h>

class EcWin7 {
   public:
    EcWin7();
    ~EcWin7();
    void init(QWidget *widget);
    void addRecentFile(const QString &filename);

    void setProgressValue(int value, int max);

   private:
    void associateFileTypes(const QStringList &fileTypes);

    ITaskbarList3 *taskbarList;
    HWND hwnd;
};

#endif  // ECWIN7_H
