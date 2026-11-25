/*
 * USB audio volume of the transmitter's sound card (part of FmStick).
 * See usbvolume.h for why the application needs it.
 *
 * Copyright (C) 2026 Aurel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "usbvolume.h"

#include <stdio.h>
#include <string.h>

/* The chip works in 10 Hz units, and so does the firmware's scaling. */
#define FLOOR_UNITS (USBVOL_DEVIATION_FLOOR_HZ / 10)

uint32_t usbVolumeEffectiveDeviation(uint32_t ceilingHz, int volume, int volumeMax)
{
    uint32_t ceil = ceilingHz / 10;
    if (volumeMax <= 0 || volume >= volumeMax)
        return ceilingHz;
    if (volume <= 0 || ceil <= FLOOR_UNITS)
        return USBVOL_DEVIATION_FLOOR_HZ;
    return 10 * (FLOOR_UNITS +
                 ((ceil - FLOOR_UNITS) * (uint32_t)volume + volumeMax / 2) / (uint32_t)volumeMax);
}

uint32_t usbVolumeCeilingDeviation(uint32_t effectiveHz, int volume, int volumeMax)
{
    uint32_t eff = effectiveHz / 10;
    if (volumeMax <= 0 || volume >= volumeMax)
        return effectiveHz;
    if (volume <= 0)
        return 0;
    if (eff <= FLOOR_UNITS)
        return USBVOL_DEVIATION_FLOOR_HZ;
    return 10 * (FLOOR_UNITS +
                 ((eff - FLOOR_UNITS) * (uint32_t)volumeMax + volume / 2) / (uint32_t)volume);
}

#if defined(__linux__) && defined(HAVE_ALSA)

#include <alsa/asoundlib.h>

static const char FMTX_USB_ID[] = "0451:2100";

/* Find the ALSA card whose USB id is the transmitter's. */
static int findCard(void)
{
    int card = -1;
    while (snd_card_next(&card) == 0 && card >= 0) {
        char path[64], id[16] = "";
        FILE *f;
        snprintf(path, sizeof(path), "/proc/asound/card%d/usbid", card);
        f = fopen(path, "r");
        if (!f)
            continue;
        if (fgets(id, sizeof(id), f) && strncmp(id, FMTX_USB_ID, sizeof(FMTX_USB_ID) - 1) == 0) {
            fclose(f);
            return card;
        }
        fclose(f);
    }
    return -1;
}

/* Open the card's mixer and return its first element with a playback volume
 * ("PCM" on the HiFi-FMT). The caller closes *mixer. */
static snd_mixer_elem_t *openVolumeElem(snd_mixer_t **mixer)
{
    char name[16];
    snd_mixer_elem_t *elem;
    int card = findCard();

    *mixer = NULL;
    if (card < 0)
        return NULL;
    snprintf(name, sizeof(name), "hw:%d", card);
    if (snd_mixer_open(mixer, 0) < 0)
        return NULL;
    if (snd_mixer_attach(*mixer, name) < 0 ||
        snd_mixer_selem_register(*mixer, NULL, NULL) < 0 ||
        snd_mixer_load(*mixer) < 0) {
        snd_mixer_close(*mixer);
        *mixer = NULL;
        return NULL;
    }
    for (elem = snd_mixer_first_elem(*mixer); elem; elem = snd_mixer_elem_next(elem))
        if (snd_mixer_selem_is_active(elem) && snd_mixer_selem_has_playback_volume(elem))
            return elem;
    snd_mixer_close(*mixer);
    *mixer = NULL;
    return NULL;
}

int usbVolumeGet(int *volume, int *volumeMax)
{
    snd_mixer_t *mixer;
    snd_mixer_elem_t *elem = openVolumeElem(&mixer);
    long min = 0, max = 0, left = 0, right = 0;
    int ret = -1;

    if (!elem)
        return -1;
    if (snd_mixer_selem_get_playback_volume_range(elem, &min, &max) == 0 &&
        snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &left) == 0) {
        if (snd_mixer_selem_is_playback_mono(elem) ||
            snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, &right) != 0)
            right = left;
        /* Which channel the firmware follows is unknown: report the lower
           one so that a low-volume warning is never hidden. */
        *volume = (int)((left < right ? left : right) - min);
        *volumeMax = (int)(max - min);
        ret = 0;
    }
    snd_mixer_close(mixer);
    return ret;
}

int usbVolumeSet(int volume)
{
    snd_mixer_t *mixer;
    snd_mixer_elem_t *elem = openVolumeElem(&mixer);
    long min = 0, max = 0;
    int ret = -1;

    if (!elem)
        return -1;
    if (snd_mixer_selem_get_playback_volume_range(elem, &min, &max) == 0) {
        long v = min + volume;
        if (v < min) v = min;
        if (v > max) v = max;
        if (snd_mixer_selem_set_playback_volume_all(elem, v) == 0)
            ret = 0;
        if (snd_mixer_selem_has_playback_switch(elem))
            snd_mixer_selem_set_playback_switch_all(elem, 1);
    }
    snd_mixer_close(mixer);
    return ret;
}

#else /* no ALSA: the volume control is simply not available */

int usbVolumeGet(int *volume, int *volumeMax)
{
    (void)volume; (void)volumeMax;
    return -1;
}

int usbVolumeSet(int volume)
{
    (void)volume;
    return -1;
}

#endif
