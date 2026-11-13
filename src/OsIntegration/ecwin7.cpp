#include "ecwin7.h"

#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QSettings>
#include <QStandardPaths>

#include <propkey.h>
#include <shlguid.h>
#include <shlobj.h>

#include "Common/global.h"

EcWin7::EcWin7() : taskbarList(nullptr), hwnd(nullptr) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
}

EcWin7::~EcWin7() {
    if (taskbarList) {
        taskbarList->Release();
        taskbarList = nullptr;
    }
    CoUninitialize();
}

void EcWin7::init(QWidget *widget) {
    hwnd = reinterpret_cast<HWND>(widget->winId());

    HRESULT hr = CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_ITaskbarList3, reinterpret_cast<void **>(&taskbarList));
    if (SUCCEEDED(hr)) {
        taskbarList->HrInit();
    }

    associateFileTypes(getAllHdrFileExtensions());

    PWSTR appId;
    hr = GetCurrentProcessExplicitAppUserModelID(&appId);
    if (SUCCEEDED(hr)) {
        SHAddToRecentDocs(SHARD_APPIDINFO, nullptr);
        CoTaskMemFree(appId);
    }
}

void EcWin7::addRecentFile(const QString &filename) {
    PWSTR appId;
    HRESULT hr = GetCurrentProcessExplicitAppUserModelID(&appId);
    if (SUCCEEDED(hr)) {
        SHAddToRecentDocs(SHARD_PATHW, filename.toStdWString().c_str());
        CoTaskMemFree(appId);
    }
}

void EcWin7::setProgressValue(int value, int max) {
    if (!taskbarList || !hwnd) return;

    if (value < 0) {
        taskbarList->SetProgressState(hwnd, TBPF_NOPROGRESS);
        return;
    }

    taskbarList->SetProgressState(hwnd, TBPF_NORMAL);
    taskbarList->SetProgressValue(hwnd, static_cast<ULONGLONG>(value),
                                  static_cast<ULONGLONG>(max));
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
    for (const QString &fileType : fileTypes)
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
