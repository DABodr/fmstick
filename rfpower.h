/*
 * Transmit power: the Si4711 sets an RF voltage in dBuV; this gives the
 * matching power into a 50 ohm load, as an estimate (part of FmStick).
 *
 * Only an estimate: the chip is made to drive a short antenna directly,
 * and the board between it and the antenna socket (matching, attenuation
 * or amplification) is not known. 120 dBuV = 1 V rms = 20 mW into 50 ohm.
 *
 * Copyright (C) 2026 Aurel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef RFPOWER_H
#define RFPOWER_H

#include <QLocale>
#include <QString>
#include <cmath>

/* "20 mW", "6,3 mW", "0,50 mW", "13 µW" (in the locale's number format) */
inline QString rfPowerText(int dBuV, const QLocale &locale = QLocale())
{
    double volts = std::pow(10.0, dBuV / 20.0) * 1e-6;
    double mW = volts * volts / 50.0 * 1000.0;
    if (mW >= 10)
        return locale.toString(mW, 'f', 0) + " mW";
    if (mW >= 1)
        return locale.toString(mW, 'f', 1) + " mW";
    if (mW >= 0.1)
        return locale.toString(mW, 'f', 2) + " mW";
    return locale.toString(mW * 1000, 'f', 0) + QString::fromUtf8(" µW");
}

#endif // RFPOWER_H
