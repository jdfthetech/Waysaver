#include "appconfig.h"

#include <QSettings>
#include <QtGlobal>

AppConfig AppConfig::load()
{
    QSettings s;
    AppConfig c;
    c.mode = modeFromName(s.value("saver/mode", "images").toString());
    c.imageDirectory = s.value("saver/imageDirectory").toString();
    c.scrPath = s.value("saver/scrPath").toString();
    c.packagePath = s.value("saver/packagePath").toString();
    c.idleMinutes = qBound(1, s.value("timing/idleMinutes", 10).toInt(), 60);
    c.slideSeconds = qBound(2, s.value("timing/slideSeconds", 10).toInt(), 3600);
    c.shuffle = s.value("images/shuffle", true).toBool();
    c.mediaGuard = s.value("behavior/mediaGuard", true).toBool();
    return c;
}

void AppConfig::save() const
{
    QSettings s;
    s.setValue("saver/mode", modeName(mode));
    s.setValue("saver/imageDirectory", imageDirectory);
    s.setValue("saver/scrPath", scrPath);
    s.setValue("saver/packagePath", packagePath);
    s.setValue("timing/idleMinutes", qBound(1, idleMinutes, 60));
    s.setValue("timing/slideSeconds", qMax(2, slideSeconds));
    s.setValue("images/shuffle", shuffle);
    s.setValue("behavior/mediaGuard", mediaGuard);
    s.sync();
}

QString AppConfig::modeName(Mode value)
{
    switch (value) {
    case Mode::WindowsScr: return "windows-scr";
    case Mode::Package: return "package";
    default: return "images";
    }
}

AppConfig::Mode AppConfig::modeFromName(const QString &value)
{
    if (value == "windows-scr") return Mode::WindowsScr;
    if (value == "package") return Mode::Package;
    return Mode::Images;
}

