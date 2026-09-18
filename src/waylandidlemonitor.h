#pragma once

#include <QObject>
#include <QString>

class QSocketNotifier;
struct wl_display;
struct wl_registry;
struct wl_seat;
struct ext_idle_notifier_v1;
struct ext_idle_notification_v1;

class WaylandIdleMonitor final : public QObject {
    Q_OBJECT
public:
    explicit WaylandIdleMonitor(QObject *parent = nullptr);
    ~WaylandIdleMonitor() override;

    bool isAvailable() const { return m_available; }
    QString errorString() const { return m_error; }
    void setTimeoutMilliseconds(quint32 milliseconds);

signals:
    void idled();
    void resumed();
    void errorOccurred(const QString &message);

public: // C callbacks used by the generated Wayland protocol listener tables.
    static void registryGlobal(void *data, wl_registry *registry, quint32 name,
                               const char *interface, quint32 version);
    static void registryGlobalRemove(void *data, wl_registry *registry, quint32 name);
    static void notificationIdled(void *data, ext_idle_notification_v1 *notification);
    static void notificationResumed(void *data, ext_idle_notification_v1 *notification);

private:
    void dispatchWayland();
    void fail(const QString &message);

    wl_display *m_display = nullptr;
    wl_registry *m_registry = nullptr;
    wl_seat *m_seat = nullptr;
    ext_idle_notifier_v1 *m_notifier = nullptr;
    ext_idle_notification_v1 *m_notification = nullptr;
    QSocketNotifier *m_socketNotifier = nullptr;
    bool m_available = false;
    QString m_error;
};
