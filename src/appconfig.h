#pragma once

#include <QString>

struct AppConfig {
    enum class Mode { Images, WindowsScr, Package };

    Mode mode = Mode::Images;
    QString imageDirectory;
    QString scrPath;
    QString packagePath;
    int idleMinutes = 10;
    int slideSeconds = 10;
    bool shuffle = true;
    bool mediaGuard = true;

    static AppConfig load();
    void save() const;
    static QString modeName(Mode value);
    static Mode modeFromName(const QString &value);
};

