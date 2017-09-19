#include "ecwin7.h"

#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QSettings>

#include "Common/global.h"

// Constructor: variabiles initialization
EcWin7::EcWin7() {
    taskbarButton = 0;
    taskbarProgress = 0;
    jumplist = 0;
}

// Init taskbar communication
void EcWin7::init(QWidget *widget) {
    taskbarButton = new QWinTaskbarButton(widget);
    taskbarButton->setWindow(widget->windowHandle());

    taskbarProgress = taskbarButton->progress();
    associateFileTypes(getAllHdrFileExtensions());

    jumplist = new QWinJumpList(widget);
    jumplist->recent()->setVisible(true);
}

void EcWin7::addRecentFile(const QString &filename) {
    jumplist->recent()->addDestination(filename);
}

// Set progress bar current value
void EcWin7::setProgressValue(int value, int max) {
    if (!taskbarProgress) return;

    if (value < 0) {
        taskbarProgress->hide();
        return;
    }

    taskbarProgress->show();
    taskbarProgress->resume();
    taskbarProgress->setMaximum(max);
    taskbarProgress->setValue(value);
}

void EcWin7::associateFileTypes(const QStringList &fileTypes) {
    QString displayName = QGuiApplication::applicationDisplayName();
    QString filePath = QCoreApplication::applicationFilePath();
    QString fileName = QFileInfo(filePath).fileName();

    QSettings settings(
        "HKEY_CURRENT_USER\\Software\\Classes\\Applications\\" + fileName,
        QSettings::NativeFormat);
    settings.setValue("FriendlyAppName", displayName);

    settings.beginGroup("SupportedTypes");
    foreach (const QString &fileType, fileTypes)
        settings.setValue(fileType, QString());
    settings.endGroup();

    settings.beginGroup("shell");
    settings.beginGroup("open");
    settings.setValue("FriendlyAppName", displayName);
    settings.beginGroup("Command");
    settings.setValue(
        ".",
        QChar('"') + QDir::toNativeSeparators(filePath) + QString("\" \"%1\""));
}