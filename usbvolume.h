/*
 * USB audio volume of the transmitter's sound card (part of FmStick).
 *
 * On Vast HiFi-FMT devices (firmware V-FMT2xx) the firmware scales the
 * Si4711 TX_AUDIO_DEVIATION by the USB audio volume:
 *
 *     effective = 50 Hz + (ceiling - 50 Hz) * volume / volume_max
 *
 * where "ceiling" is the value last written to TX_AUDIO_DEVIATION. Reading
 * TX_AUDIO_DEVIATION returns the effective (scaled) value, and a USB volume
 * of 0 leaves the transmitter silent while the carrier and RDS stay on air.
 *
 * Copyright (C) 2026 Aurel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef USBVOLUME_H
#define USBVOLUME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Lowest deviation the firmware applies (USB volume 0), in Hz. */
#define USBVOL_DEVIATION_FLOOR_HZ 50

/** Read the USB playback volume of the transmitter's sound card.
 *  \return 0 on success, -1 if the card or its volume control is not found
 *  (no ALSA, other platform, device unplugged, non-Vast device). */
int usbVolumeGet(int *volume, int *volumeMax);

/** Set the USB playback volume (raw value, clamped to 0..max) on all channels.
 *  \return 0 on success, -1 on failure. */
int usbVolumeSet(int volume);

/** Effective deviation the firmware applies for a ceiling and a USB volume. */
uint32_t usbVolumeEffectiveDeviation(uint32_t ceilingHz, int volume, int volumeMax);

/** Ceiling deviation recovered from an effective deviation read back from the
 *  chip. Exact at full volume; only approximate at low volume.
 *  \return 0 if it cannot be recovered (volume 0). */
uint32_t usbVolumeCeilingDeviation(uint32_t effectiveHz, int volume, int volumeMax);

#ifdef __cplusplus
}
#endif

#endif // USBVOLUME_H
