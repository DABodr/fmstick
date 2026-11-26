/*
 * What media players are playing, read over D-Bus (MPRIS).
 * See nowplaying.h.
 *
 * Copyright (C) 2026 Aurel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "nowplaying.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusVariant>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <QVariantMap>

static const char MPRIS_PREFIX[] = "org.mpris.MediaPlayer2.";
static const int TIMEOUT_MS = 300;

static QVariant getProperty(QDBusConnection &bus, const QString &service,
                            const QString &interface, const QString &name)
{
    QDBusMessage call = QDBusMessage::createMethodCall(service, "/org/mpris/MediaPlayer2",
                                                      "org.freedesktop.DBus.Properties", "Get");
    call << interface << name;
    QDBusMessage reply = bus.call(call, QDBus::Block, TIMEOUT_MS);
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty())
        return QVariant();
    return reply.arguments().first().value<QDBusVariant>().variant();
}

/* Metadata is an a{sv}: it arrives as a QDBusArgument */
static QVariantMap toMap(const QVariant &v)
{
    if (v.canConvert<QDBusArgument>())
        return qdbus_cast<QVariantMap>(v.value<QDBusArgument>());
    return v.toMap();
}

static QString toText(const QVariant &v)
{
    if (v.canConvert<QDBusArgument>())
        return qdbus_cast<QStringList>(v.value<QDBusArgument>()).join(", ");
    if (v.typeId() == QMetaType::QStringList)
        return v.toStringList().join(", ");
    return v.toString();
}

QList<MediaPlayer> NowPlaying::players()
{
    QList<MediaPlayer> list;
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected() || !bus.interface())
        return list;

    QSet<QString> owners;
    const QStringList names = bus.interface()->registeredServiceNames().value();
    for (const QString &service : names) {
        if (!service.startsWith(MPRIS_PREFIX))
            continue;
        /* a player can own several names (VLC: "vlc" and "vlc.instance1234") */
        QString owner = bus.interface()->serviceOwner(service).value();
        if (owner.isEmpty() || owners.contains(owner))
            continue;
        owners.insert(owner);

        MediaPlayer p;
        /* "vlc.instance7843", "chromium.instance2345", "firefox.instance_1_42" */
        p.id = service.mid(sizeof(MPRIS_PREFIX) - 1);
        p.id.remove(QRegularExpression("\\.instance[_0-9]+$"));
        if (!identities.contains(owner)) {
            QString id = getProperty(bus, service, "org.mpris.MediaPlayer2", "Identity").toString();
            identities.insert(owner, id.isEmpty() ? service.mid(sizeof(MPRIS_PREFIX) - 1) : id);
        }
        p.identity = identities.value(owner);
        p.playing = getProperty(bus, service, "org.mpris.MediaPlayer2.Player", "PlaybackStatus").toString() == "Playing";

        QVariantMap meta = toMap(getProperty(bus, service, "org.mpris.MediaPlayer2.Player", "Metadata"));
        /* a web radio: the title is the station, the track is in vlc:nowplaying */
        QString nowPlaying = toText(meta.value("vlc:nowplaying")).trimmed();
        QString artist = toText(meta.value("xesam:artist")).trimmed();
        QString title = toText(meta.value("xesam:title")).trimmed();
        if (!nowPlaying.isEmpty())
            p.text = nowPlaying;
        else if (!artist.isEmpty() && !title.isEmpty())
            p.text = artist + " - " + title;
        else
            p.text = title;
        list.append(p);
    }

    /* forget players that are gone */
    for (auto it = identities.begin(); it != identities.end(); )
        it = owners.contains(it.key()) ? std::next(it) : identities.erase(it);
    return list;
}
