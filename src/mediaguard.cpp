#include "mediaguard.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusVariant>
#include <QVariantMap>

MediaGuard::Result MediaGuard::query()
{
    Result result;
    auto bus = QDBusConnection::sessionBus();
    auto *iface = bus.interface();
    if (!iface) return result;

    const QDBusReply<QStringList> names = iface->registeredServiceNames();
    if (!names.isValid()) return result;

    for (const QString &name : names.value()) {
        if (!name.startsWith("org.mpris.MediaPlayer2.")) continue;
        QDBusInterface props(name, "/org/mpris/MediaPlayer2",
                             "org.freedesktop.DBus.Properties", bus);
        props.setTimeout(500);
        const QDBusReply<QDBusVariant> reply = props.call(
            "Get", "org.mpris.MediaPlayer2.Player", "PlaybackStatus");
        if (reply.isValid() && reply.value().variant().toString() == "Playing") {
            result.playing = true;
            result.players << name.mid(QString("org.mpris.MediaPlayer2.").size());
        }
    }
    return result;
}
