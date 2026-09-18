#pragma once

#include "appconfig.h"
#include "waylandidlemonitor.h"

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QVector>

class SaverWindow;

class WaySaverDaemon final : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.waysaver.Service")
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
public:
    explicit WaySaverDaemon(QObject *parent = nullptr);
    bool active() const { return m_active; }

public slots:
    Q_SCRIPTABLE void Activate();
    Q_SCRIPTABLE void Deactivate();
    Q_SCRIPTABLE void Reload();
    Q_SCRIPTABLE bool IsActive() const { return m_active; }
    Q_SCRIPTABLE QString Status() const;

signals:
    Q_SCRIPTABLE void activeChanged(bool active);
    Q_SCRIPTABLE void statusChanged(const QString &status);

private slots:
    void idleReached();
    void resumed();
    void retryAfterMedia();

private:
    void armIdleTimer();
    bool startImageSaver(const AppConfig &config);
    bool startWindowsSaver(const QString &path);
    bool startPackageSaver(const QString &path, const AppConfig &config);
    void setStatus(const QString &status);

    AppConfig m_config;
    WaylandIdleMonitor m_idleMonitor;
    QTimer m_mediaRetry;
    bool m_active = false;
    QString m_status = "Waiting";
    QVector<SaverWindow *> m_windows;
    QProcess m_external;
};
