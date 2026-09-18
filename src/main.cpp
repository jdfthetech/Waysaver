#include "daemon.h"
#include "packageapi.h"
#include "settingswindow.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTextStream>
#include <QUrl>

static int createPackage(const QCommandLineParser &parser)
{
    PackageApi::Definition d;
    d.name = parser.value("name");
    d.id = parser.value("id");
    d.type = parser.value("type");
    d.source = parser.value("source");
    d.slideSeconds = parser.value("slide-seconds").toInt();
    const auto result = PackageApi::create(d, parser.value("output"));
    QTextStream stream(result.ok ? stdout : stderr);
    stream << (result.ok ? "Created " + parser.value("output") : "Error: " + result.error) << Qt::endl;
    return result.ok ? 0 : 2;
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("WaySaver");
    QCoreApplication::setOrganizationDomain("waysaver.org");
    QCoreApplication::setApplicationName("WaySaver");
    QCoreApplication::setApplicationVersion(WAYSAVER_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Wayland screensaver service and settings application");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({"daemon", "Run the background screensaver service."});
    parser.addOption({"settings", "Open the settings window."});
    parser.addOption({"activate", "Activate the running screensaver service."});
    parser.addOption({"deactivate", "Deactivate the running screensaver service."});
    parser.addOption({"status-json", "Print Waybar-compatible service status JSON."});
    parser.addOption({"create-package", "Create a .waysaver package manifest."});
    parser.addOption({"self-test", "Validate the installed data model."});
    parser.addOption({"name", "Package display name.", "name"});
    parser.addOption({"id", "Package reverse-domain identifier.", "id"});
    parser.addOption({"type", "Package type: images or executable.", "type", "images"});
    parser.addOption({"source", "Relative source directory or executable.", "path"});
    parser.addOption({"output", "Output .waysaver file.", "file"});
    parser.addOption({"slide-seconds", "Image interval.", "seconds", "10"});
    parser.addPositionalArgument("url", "Optional waysaver:// action URL.");
    parser.process(app);

    if (parser.isSet("create-package")) return createPackage(parser);
    if (parser.isSet("status-json")) {
        QDBusInterface daemon("org.waysaver.Service", "/WaySaver", "org.waysaver.Service");
        const QDBusReply<bool> active = daemon.call("IsActive");
        const QDBusReply<QString> status = daemon.call("Status");
        const bool isActive = active.isValid() && active.value();
        const QJsonObject output{
            {"text", isActive ? "◉" : "○"},
            {"tooltip", status.isValid() ? status.value() : "WaySaver service unavailable"},
            {"class", isActive ? "active" : "waiting"}
        };
        const QByteArray json = QJsonDocument(output).toJson(QJsonDocument::Compact);
        QTextStream(stdout) << QString::fromUtf8(json) << Qt::endl;
        return (active.isValid() && status.isValid()) ? 0 : 3;
    }
    if (parser.isSet("self-test")) {
        AppConfig c = AppConfig::load();
        return (c.idleMinutes >= 1 && c.idleMinutes <= 60) ? 0 : 1;
    }

    QString action;
    if (parser.isSet("activate")) action = "Activate";
    if (parser.isSet("deactivate")) action = "Deactivate";
    if (!parser.positionalArguments().isEmpty()) {
        const QUrl url(parser.positionalArguments().first());
        if (url.scheme() == "waysaver") {
            if (url.host() == "activate") action = "Activate";
            else if (url.host() == "deactivate") action = "Deactivate";
            else if (url.host() == "settings") action = "Settings";
        }
    }
    if (!action.isEmpty() && action != "Settings") {
        QDBusInterface daemon("org.waysaver.Service", "/WaySaver", "org.waysaver.Service");
        if (!daemon.isValid()) {
            QTextStream(stderr) << "WaySaver service is not running." << Qt::endl;
            return 3;
        }
        daemon.call(action);
        return 0;
    }

    if (parser.isSet("daemon")) {
        auto bus = QDBusConnection::sessionBus();
        if (!bus.registerService("org.waysaver.Service")) {
            QTextStream(stderr) << "Another WaySaver service is already running." << Qt::endl;
            return 4;
        }
        WaySaverDaemon daemon;
        if (!bus.registerObject("/WaySaver", &daemon,
                                QDBusConnection::ExportScriptableSlots |
                                QDBusConnection::ExportScriptableSignals |
                                QDBusConnection::ExportScriptableProperties)) {
            QTextStream(stderr) << "Could not export the WaySaver D-Bus object." << Qt::endl;
            return 5;
        }
        return app.exec();
    }

    SettingsWindow window;
    window.show();
    return app.exec();
}
