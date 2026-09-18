#include "waylandidlemonitor.h"

#include "ext-idle-notify-v1-client-protocol.h"

#include <QSocketNotifier>
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <wayland-client.h>

namespace {
const wl_registry_listener registryListener{
    WaylandIdleMonitor::registryGlobal,
    WaylandIdleMonitor::registryGlobalRemove
};

const ext_idle_notification_v1_listener notificationListener{
    WaylandIdleMonitor::notificationIdled,
    WaylandIdleMonitor::notificationResumed
};
}

WaylandIdleMonitor::WaylandIdleMonitor(QObject *parent) : QObject(parent)
{
    m_display = wl_display_connect(nullptr);
    if (!m_display) {
        fail("Could not connect to the Wayland compositor");
        return;
    }

    m_registry = wl_display_get_registry(m_display);
    if (!m_registry) {
        fail("Could not read the Wayland global registry");
        return;
    }
    wl_registry_add_listener(m_registry, &registryListener, this);
    if (wl_display_roundtrip(m_display) < 0) {
        fail("The Wayland compositor disconnected during initialization");
        return;
    }
    if (!m_notifier) {
        fail("The compositor does not support ext-idle-notify-v1");
        return;
    }
    if (!m_seat) {
        fail("The compositor did not advertise an input seat");
        return;
    }

    m_socketNotifier = new QSocketNotifier(wl_display_get_fd(m_display),
                                            QSocketNotifier::Read, this);
    connect(m_socketNotifier, &QSocketNotifier::activated,
            this, &WaylandIdleMonitor::dispatchWayland);
    m_available = true;
}

WaylandIdleMonitor::~WaylandIdleMonitor()
{
    if (m_socketNotifier) m_socketNotifier->setEnabled(false);
    if (m_notification) ext_idle_notification_v1_destroy(m_notification);
    if (m_notifier) ext_idle_notifier_v1_destroy(m_notifier);
    if (m_seat) wl_seat_destroy(m_seat);
    if (m_registry) wl_registry_destroy(m_registry);
    if (m_display) wl_display_disconnect(m_display);
}

void WaylandIdleMonitor::setTimeoutMilliseconds(quint32 milliseconds)
{
    if (!m_available) return;
    if (m_notification) {
        ext_idle_notification_v1_destroy(m_notification);
        m_notification = nullptr;
    }
    m_notification = ext_idle_notifier_v1_get_idle_notification(
        m_notifier, milliseconds, m_seat);
    if (!m_notification) {
        fail("The compositor could not create an idle notification");
        return;
    }
    ext_idle_notification_v1_add_listener(m_notification,
                                           &notificationListener, this);
    if (wl_display_flush(m_display) < 0 && errno != EAGAIN) {
        fail("The Wayland compositor disconnected while setting the idle timer");
    }
}

void WaylandIdleMonitor::registryGlobal(void *data, wl_registry *registry,
                                        quint32 name, const char *interface,
                                        quint32 version)
{
    auto *self = static_cast<WaylandIdleMonitor *>(data);
    if (!self->m_notifier && std::strcmp(interface, ext_idle_notifier_v1_interface.name) == 0) {
        self->m_notifier = static_cast<ext_idle_notifier_v1 *>(
            wl_registry_bind(registry, name, &ext_idle_notifier_v1_interface,
                             std::min(version, 1U)));
    } else if (!self->m_seat && std::strcmp(interface, wl_seat_interface.name) == 0) {
        self->m_seat = static_cast<wl_seat *>(
            wl_registry_bind(registry, name, &wl_seat_interface,
                             std::min(version, 7U)));
    }
}

void WaylandIdleMonitor::registryGlobalRemove(void *, wl_registry *, quint32)
{
}

void WaylandIdleMonitor::notificationIdled(void *data,
                                           ext_idle_notification_v1 *)
{
    emit static_cast<WaylandIdleMonitor *>(data)->idled();
}

void WaylandIdleMonitor::notificationResumed(void *data,
                                             ext_idle_notification_v1 *)
{
    emit static_cast<WaylandIdleMonitor *>(data)->resumed();
}

void WaylandIdleMonitor::dispatchWayland()
{
    if (!m_display || wl_display_dispatch(m_display) < 0)
        fail("The Wayland compositor connection was lost");
}

void WaylandIdleMonitor::fail(const QString &message)
{
    m_available = false;
    m_error = message;
    if (m_socketNotifier) m_socketNotifier->setEnabled(false);
    emit errorOccurred(message);
}
