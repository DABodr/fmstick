/*
 * What media players are playing, read over D-Bus (MPRIS), to put the
 * current track in the RadioText (part of FmStick).
 *
 * Works with any player that implements MPRIS on Linux: VLC, Chrome,
 * Firefox, Spotify, Rhythmbox, mpv (with its MPRIS plugin)...
 *
 * Copyright (C) 2026 Aurel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef NOWPLAYING_H
#define NOWPLAYING_H

#include <QHash>
#include <QList>
#include <QString>

struct MediaPlayer
{
    QString id;         /* stable: the bus name without its instance, e.g. "vlc" */
    QString identity;   /* shown to the user; translated by some players
                           ("Lecteur multimédia VLC"): never used to match */
    bool playing;
    QString text;       /* "Artist - Title", or the stream's current title */
};

class NowPlaying
{
public:
    /* The MPRIS players on the session bus, each listed once. Blocking
       D-Bus calls with a short timeout: call it every few seconds at most. */
    QList<MediaPlayer> players();

private:
    QHash<QString, QString> identities;   /* bus owner -> identity */
};

#endif // NOWPLAYING_H
