#include "daemon.h"

#include "mediaguard.h"
#include "packageapi.h"
#include "saverwindow.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScreen>
#include <QStandardPaths>
#include <utility>

WaySaverDaemon::WaySaverDaemon(QObject *parent) : QObject(parent)
{
    connect(&m_idleMonitor, &WaylandIdleMonitor::idled,
            this, &WaySaverDaemon::idleReached);
    connect(&m_idleMonitor, &WaylandIdleMonitor::resumed,
            this, &WaySaverDaemon::resumed);
    connect(&m_idleMonitor, &WaylandIdleMonitor::errorOccurred,
            this, [this](const QString &message) { setStatus(message); });
    m_mediaRetry.setInterval(5000);
    connect(&m_mediaRetry, &QTimer::timeout,
            this, &WaySaverDaemon::retryAfterMedia);
    connect(&m_external, &QProcess::finished, this, [this] {
        if (m_active) Deactivate();
    });
    Reload();
}

void WaySaverDaemon::armIdleTimer()
{
    m_idleMonitor.setTimeoutMilliseconds(
        static_cast<quint32>(m_config.idleMinutes * 60 * 1000));
}

void WaySaverDaemon::Reload()
{
    m_config = AppConfig::load();
    armIdleTimer();
    if (!m_idleMonitor.isAvailable())
        setStatus(m_idleMonitor.errorString());
    else
        setStatus(m_active ? "Active" : "Waiting on Wayland idle notification");
}

void WaySaverDaemon::idleReached()
{
    if (m_active) return;
    if (m_config.mediaGuard) {
        const auto media = MediaGuard::query();
        if (media.playing) {
            setStatus("Paused while media is playing: " + media.players.join(", "));
            m_mediaRetry.start();
            return;
        }
    }
    Activate();
}

void WaySaverDaemon::resumed()
{
    m_mediaRetry.stop();
    if (m_active) Deactivate();
}

void WaySaverDaemon::retryAfterMedia()
{
    if (m_active) {
        m_mediaRetry.stop();
        return;
    }
    const auto media = MediaGuard::query();
    if (media.playing) {
        setStatus("Paused while media is playing: " + media.players.join(", "));
        return;
    }
    m_mediaRetry.stop();
    Activate();
}

void WaySaverDaemon::Activate()
{
    if (m_active) return;
    bool started = false;
    switch (m_config.mode) {
    case AppConfig::Mode::WindowsScr:
        started = startWindowsSaver(m_config.scrPath);
        break;
    case AppConfig::Mode::Package:
        started = startPackageSaver(m_config.packagePath, m_config);
        break;
    default:
        started = startImageSaver(m_config);
        break;
    }
    if (!started) return;
    m_active = true;
    setStatus("Active");
    emit activeChanged(true);
}

void WaySaverDaemon::Deactivate()
{
    if (!m_active && m_windows.isEmpty() && m_external.state() == QProcess::NotRunning) return;
    m_active = false;
    for (SaverWindow *window : std::as_const(m_windows)) {
        if (window) window->close();
    }
    m_windows.clear();
    if (m_external.state() != QProcess::NotRunning) {
        m_external.terminate();
        if (!m_external.waitForFinished(1500)) m_external.kill();
    }
    setStatus(m_idleMonitor.isAvailable()
                  ? "Waiting on Wayland idle notification"
                  : m_idleMonitor.errorString());
    emit activeChanged(false);
}

bool WaySaverDaemon::startImageSaver(const AppConfig &config)
{
    for (QScreen *screen : QGuiApplication::screens()) {
        auto *window = new SaverWindow(screen, config.imageDirectory,
                                       config.slideSeconds, config.shuffle);
        connect(window, &SaverWindow::dismissRequested, this, &WaySaverDaemon::Deactivate);
        m_windows << window;
        window->showFullScreen();
        window->raise();
        window->activateWindow();
    }
    return !m_windows.isEmpty();
}

bool WaySaverDaemon::startWindowsSaver(const QString &path)
{
    if (!QFileInfo::exists(path)) {
        setStatus("The selected Windows screensaver is missing");
        return false;
    }
    const QString wine = QStandardPaths::findExecutable("wine");
    if (wine.isEmpty()) {
        setStatus("Wine is required to run a Windows screensaver");
        return false;
    }
    QRect desktop;
    for (QScreen *screen : QGuiApplication::screens()) desktop = desktop.united(screen->geometry());
    const QString virtualDesktop = QString("WaySaver,%1x%2").arg(desktop.width()).arg(desktop.height());
    m_external.setProgram(wine);
    m_external.setArguments({"explorer", "/desktop=" + virtualDesktop, path, "/s"});
    m_external.start();
    if (!m_external.waitForStarted(3000)) {
        setStatus("Wine could not start the screensaver: " + m_external.errorString());
        return false;
    }
    return true;
}

bool WaySaverDaemon::startPackageSaver(const QString &path, const AppConfig &config)
{
    PackageApi::Definition d;
    const auto result = PackageApi::validate(path, &d);
    if (!result.ok) {
        setStatus("Invalid WaySaver package: " + result.error);
        return false;
    }
    const QDir base = QFileInfo(path).absoluteDir();
    const QString source = QFileInfo(d.source).isAbsolute() ? d.source : base.filePath(d.source);
    if (d.type == "images") {
        AppConfig packageConfig = config;
        packageConfig.imageDirectory = source;
        packageConfig.slideSeconds = d.slideSeconds;
        return startImageSaver(packageConfig);
    }
    if (!QFileInfo::exists(source)) {
        setStatus("Package executable is missing");
        return false;
    }
    m_external.setProgram(source);
    m_external.setArguments({"--waysaver-fullscreen"});
    m_external.setWorkingDirectory(base.absolutePath());
    m_external.start();
    if (!m_external.waitForStarted(3000)) {
        setStatus("Package executable could not start: " + m_external.errorString());
        return false;
    }
    return true;
}

QString WaySaverDaemon::Status() const
{
    return m_status;
}

void WaySaverDaemon::setStatus(const QString &status)
{
    if (m_status == status) return;
    m_status = status;
    emit statusChanged(status);
}
