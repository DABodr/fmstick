/*
 * USB audio volume of the transmitter (part of FmStick).
 *
 * On Vast HiFi-FMT devices the firmware scales the audio deviation by the
 * USB audio volume (see usbvolume.h). This file adds the "USB Volume" group
 * to the Sound tab, warns when the volume silences the transmitter, can raise
 * it to maximum on connect, and recovers the real deviation setting from the
 * scaled value the chip reports.
 *
 * Copyright (C) 2026 Aurel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "fmtx.h"
#include "usbvolume.h"

void MainWindow::setupUsbVolumeUi()
{
    usbVolGroup = new QGroupBox(this);

    usbVolSlider = new QSlider(Qt::Horizontal, usbVolGroup);
    usbVolSlider->setRange(0, 31);
    usbVolSlider->setPageStep(4);
    usbVolValueLabel = new QLabel(usbVolGroup);
    usbVolValueLabel->setMinimumWidth(40);
    usbVolValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QHBoxLayout *sliderRow = new QHBoxLayout();
    sliderRow->addWidget(usbVolSlider);
    sliderRow->addWidget(usbVolValueLabel);

    usbVolDeviationLabel = new QLabel(usbVolGroup);
    usbVolDeviationLabel->setWordWrap(true);

    usbVolWarningLabel = new QLabel(usbVolGroup);
    usbVolWarningLabel->setWordWrap(true);
    usbVolWarningLabel->setStyleSheet("QLabel { color: #c62828; font-weight: bold; }");
    usbVolWarningLabel->hide();

    usbVolMaxOnConnect = new QCheckBox(usbVolGroup);
    usbVolMaxOnConnect->setChecked(true);

    QVBoxLayout *layout = new QVBoxLayout(usbVolGroup);
    layout->addLayout(sliderRow);
    layout->addWidget(usbVolDeviationLabel);
    layout->addWidget(usbVolWarningLabel);
    layout->addWidget(usbVolMaxOnConnect);

    /* right below "Sound in Stand-Alone mode" in the Sound tab */
    ui->verticalLayout_9->insertWidget(ui->verticalLayout_9->indexOf(ui->groupBox_12) + 1, usbVolGroup);

    connect(usbVolSlider, SIGNAL(valueChanged(int)), this, SLOT(SetUsbVolume(int)));
    connect(ui->spinBox_transmitter_audioDev, SIGNAL(valueChanged(int)), this, SLOT(refreshUsbVolume()));

    /* the volume also changes from outside (system mixer, PulseAudio...) */
    connect(&usbVolTimer, SIGNAL(timeout()), this, SLOT(refreshUsbVolume()));
    usbVolTimer.start(1000);
    retranslateUsbVolumeUi();
}

void MainWindow::retranslateUsbVolumeUi()
{
    usbVolGroup->setTitle(tr("USB Volume"));
    usbVolGroup->setToolTip(tr("The volume of the transmitter's USB sound card.\n"
                               "The device firmware multiplies the audio deviation by this volume:\n"
                               "at maximum the deviation is the one set in the Transmitter tab,\n"
                               "at 0 nothing is modulated (carrier and RDS only, no sound)."));
    usbVolMaxOnConnect->setText(tr("Set to maximum on connect"));
    refreshUsbVolume();
}

void MainWindow::refreshUsbVolume()
{
    int vol = 0, max = 0;

    if (usbVolumeGet(&vol, &max) != 0 || max <= 0) {
        usbVolGroup->setEnabled(false);
        usbVolValueLabel->setText("-");
        usbVolDeviationLabel->setText(tr("Transmitter sound card not found."));
        usbVolWarningLabel->hide();
        return;
    }
    usbVolGroup->setEnabled(true);

    usbVolSlider->blockSignals(true);
    usbVolSlider->setMaximum(max);
    usbVolSlider->setValue(vol);
    usbVolSlider->blockSignals(false);
    usbVolValueLabel->setText(QString("%1/%2").arg(vol).arg(max));

    uint32_t setting = ui->spinBox_transmitter_audioDev->value();
    uint32_t effective = usbVolumeEffectiveDeviation(setting, vol, max);
    usbVolDeviationLabel->setText(tr("Audio deviation on air: %1 Hz (setting: %2 Hz)")
                                  .arg(effective).arg(setting));

    if (vol == 0) {
        usbVolWarningLabel->setText(tr("USB volume is 0: carrier and RDS are on air, but no sound."));
        usbVolWarningLabel->show();
    } else if (vol < max) {
        usbVolWarningLabel->setText(tr("USB volume below maximum: the audio deviation is reduced."));
        usbVolWarningLabel->show();
    } else {
        usbVolWarningLabel->hide();
    }
}

void MainWindow::SetUsbVolume(int iVal)
{
    if (usbVolumeSet(iVal) != 0)
        qDebug() << "Can't set the USB volume";
    refreshUsbVolume();
}

/* Called on connect. Returns true when the volume was changed: the firmware
 * then needs a moment before the chip reports the new deviation. */
bool MainWindow::usbVolumeToMaxIfNeeded()
{
    int vol = 0, max = 0;

    if (!usbVolMaxOnConnect->isChecked())
        return false;
    if (usbVolumeGet(&vol, &max) != 0 || vol >= max)
        return false;
    qDebug() << "USB volume" << vol << "/" << max << ": setting it to maximum";
    if (usbVolumeSet(max) != 0)
        return false;
    refreshUsbVolume();
    return true;
}

/* The chip reports the deviation already scaled by the USB volume. Turn it
 * back into the setting, so that what the Transmitter tab shows (and what
 * "Write to EEPROM" stores) is the real setting, not a reduced value. */
uint32_t MainWindow::audioDeviationFromDevice(uint32_t readHz)
{
    int vol = 0, max = 0;

    if (fmtxCmdSpecial != FMTX_SPECIAL_FRONTEND)    /* EEPROM values are not scaled */
        return readHz;
    if (usbVolumeGet(&vol, &max) != 0 || vol >= max)
        return readHz;
    if (vol == 0)   /* nothing to recover from: keep what the UI already has */
        return ui->spinBox_transmitter_audioDev->value();
    return usbVolumeCeilingDeviation(readHz, vol, max);
}
