/*
 * The PS and RadioText the chip is sending (part of FmStick).
 *
 * The Si4711 cannot report the PS/RT it is transmitting: on connect, the
 * app used to show the ones of the EEPROM image, which differ as soon as
 * the text was changed live. Remember what was last sent, together with a
 * key of the USB plug session: if the transmitter has not been unplugged
 * since (same boot, same USB device number), the chip still sends it.
 *
 * Copyright (C) 2026 Aurel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef ONAIR_H
#define ONAIR_H

#include <QDir>
#include <QFile>
#include <QSettings>
#include <QString>

namespace OnAir {

/* "<boot id>:<bus>:<device number>": Linux gives the transmitter a new
   device number each time it is plugged in. Empty if unknown (other
   platforms, device not found): then nothing is restored. */
inline QString plugSession()
{
#ifdef Q_OS_LINUX
    auto readFile = [](const QString &path) {
        QFile f(path);
        return f.open(QIODevice::ReadOnly) ? QString::fromLatin1(f.readAll()).trimmed() : QString();
    };
    QString boot = readFile("/proc/sys/kernel/random/boot_id");
    QDir usb("/sys/bus/usb/devices");
    const QStringList devices = usb.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &d : devices) {
        QString base = usb.filePath(d) + "/";
        if (readFile(base + "idVendor") == "0451" && readFile(base + "idProduct") == "2100") {
            QString bus = readFile(base + "busnum"), dev = readFile(base + "devnum");
            if (!boot.isEmpty() && !bus.isEmpty() && !dev.isEmpty())
                return boot + ":" + bus + ":" + dev;
        }
    }
#endif
    return QString();
}

/* what = "rt" or "ps" */
inline void remember(const QString &what, const QString &text)
{
    QString session = plugSession();
    if (session.isEmpty())
        return;
    QSettings settings;
    settings.beginGroup("on_air");
    settings.setValue(what, text);
    settings.setValue(what + "_session", session);
    settings.endGroup();
}

/* The text last sent, if the chip is still sending it; else false. */
inline bool recall(const QString &what, QString *text)
{
    QString session = plugSession();
    QSettings settings;
    settings.beginGroup("on_air");
    bool same = !session.isEmpty() && settings.value(what + "_session").toString() == session
                && settings.contains(what);
    if (same)
        *text = settings.value(what).toString();
    settings.endGroup();
    return same;
}

}

#endif // ONAIR_H
