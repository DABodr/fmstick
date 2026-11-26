/*
 * Part of FmStick, see LICENSE.txt.
 * Modified 2026-11-25 by Aurel: USB volume handling for Vast HiFi-FMT.
 * Modified 2026-11-25 by Aurel: release the USB lock when setting the power fails.
 * Modified 2026-11-26 by Aurel: an out-of-band frequency read on connect is not shown.
 */

#include "mainwindow.h"
#include "querier.h"
#include "ui_mainwindow.h"

#include "fmtx.h"
#include "logging.h"


/******************************************************************
 * Transmitter editor
 ******************************************************************/
void MainWindow::DisplayTransmitterEditor(void)
{
    uint16_t transmitterFlags = 0;
    uint8_t power=0;
    double freq=0.0;
    double rfcap=0;
    int fmtxStatus=0;
    uint32_t audioDeviation=0;

    qDebug() << "Reading Transmitter info...";

    transmitterFlags=fmtxTransmitterGetComponentFlags();
    if(fmtxCmdStatus!=FMTX_MODE_OK) goto display_transmitter_err;

    ui->comboBox_transmitter_preemph->setCurrentIndex(fmtxTransmitterGetPreemphasisId());
    if(fmtxCmdStatus!=FMTX_MODE_OK) goto display_transmitter_err;

    audioDeviation = fmtxTransmitterGetAudioDeviation();
    if(fmtxCmdStatus!=FMTX_MODE_OK) goto display_transmitter_err;
    /* undo the firmware's scaling by the USB volume, see usbvolume.h */
    ui->spinBox_transmitter_audioDev->setValue(audioDeviationFromDevice(audioDeviation));

    ui->checkBox_transmitter_pilot->setChecked(transmitterFlags & TX_COMPONENT_ENABLE_PILOT_MASK);
    toggleTransmitPilot(transmitterFlags & TX_COMPONENT_ENABLE_PILOT_MASK);

    ui->spinBox_transmitter_pilot_devi->setValue(fmtxTransmitterGetPilotDeviation());
    if(fmtxCmdStatus!=FMTX_MODE_OK) goto display_transmitter_err;

    ui->spinBox_transmitter_pilot_freq->setValue(fmtxTransmitterGetPilotFrequency());
    if(fmtxCmdStatus!=FMTX_MODE_OK) goto display_transmitter_err;

    ui->checkBox_transmitter_lmr->setChecked(transmitterFlags & TX_COMPONENT_ENABLE_LMR_MASK);

    ui->checkBox_transmitter_rds->setChecked(transmitterFlags & TX_COMPONENT_ENABLE_RDS_MASK);
    toggleTransmitRds(transmitterFlags & TX_COMPONENT_ENABLE_RDS_MASK);

    //spinBox_transmitter_rds_devi
    ui->spinBox_transmitter_rds_devi->setValue(fmtxTransmitterGetRDSDeviation());
    if(fmtxCmdStatus!=FMTX_MODE_OK) goto display_transmitter_err;

    fmtxStatus = fmtxTransmitterGetTuneStatus(&freq, &power, &rfcap);
    qDebug() << "Transmitter configuration("<< fmtxCmdStatus <<"): " <<
                "freq:" << freq << ", power:" << power << ", rfcap:" << rfcap << ", tuneStatus:" << fmtxStatus;
    if(fmtxCmdStatus!=FMTX_MODE_OK && fmtxCmdStatus!=FMTX_MODE_TRANSMITTING) goto display_transmitter_err;

    ui->checkBox_transmitter_enabled->setChecked(power>0);
    toggleTransmitEnabled(power>0);
    SetTransmitIcon(power>0 ? ICON_TX : ICON_NOTX);

    /* After a replug the chip restarts from its EEPROM, but the firmware may
       not know the frequency yet: a read at the wrong moment gave 0, shown as
       76 MHz while 107.4 was on air (and "Keep at startup" would have saved
       it). Never show an out-of-band value: take the EEPROM's, which the
       chip started with, and read the device again shortly. */
    if(fmtxCmdSpecial == FMTX_SPECIAL_FRONTEND && (freq < 76.0 || freq > 108.0)) {
        double eepromFreq = 0, eepromCap = 0;
        uint8_t eepromPower = 0;
        fmtxCmdSpecial = FMTX_SPECIAL_EEPROM;
        int ok = fmtxTransmitterGetTuneStatus(&eepromFreq, &eepromPower, &eepromCap);
        fmtxCmdSpecial = FMTX_SPECIAL_FRONTEND;
        qDebug() << "Frequency read out of band:" << freq << "MHz, EEPROM:" << eepromFreq << "MHz";
        freq = (ok == FMTX_MODE_OK && eepromFreq >= 76.0 && eepromFreq <= 108.0) ? eepromFreq : 0;
        QTimer::singleShot(4000, this, [this]() { recheckTuneStatus(1); });
    }
    if(freq >= 76.0 && freq <= 108.0)
        ui->doubleSpinBox_transmitter_freq->setValue(freq);

    ui->spinBox_transmitter_power->setValue(power);

    //TODO: we dont know who setted this value, user or ic
    //ui->checkBox_transmitter_antcap->setChecked(false);
    //toggleTransmitAntCap(0);
    //doubleSpinBox_transmitter_antcap_val
    ui->doubleSpinBox_transmitter_antcap_val->setValue(rfcap);

    qDebug() << "Transmitter info readed";
    return;

display_transmitter_err:
    qDebug() << "Error while reading transmitter info";
}

/* Second read of the frequency, see DisplayTransmitterEditor(). */
void MainWindow::recheckTuneStatus(int attempt)
{
    double freq = 0, rfcap = 0;
    uint8_t power = 0;
    if(!deviceStatus || fmtxCmdSpecial != FMTX_SPECIAL_FRONTEND)
        return;
    querier->HidAccessMutex.lock();
    int ret = fmtxTransmitterGetTuneStatus(&freq, &power, &rfcap);
    querier->HidAccessMutex.unlock();
    qDebug() << "Frequency read again:" << freq << "MHz, attempt" << attempt;
    if(ret != FMTX_MODE_OK || freq < 76.0 || freq > 108.0) {
        if(attempt < 3)
            QTimer::singleShot(4000, this, [this, attempt]() { recheckTuneStatus(attempt + 1); });
        return;
    }
    if(qAbs(freq - ui->doubleSpinBox_transmitter_freq->value()) > 0.001) {
        int live = liveEditingEnabled;
        liveEditingEnabled = false;     /* what is on air: nothing to send */
        ui->doubleSpinBox_transmitter_freq->setValue(freq);
        liveEditingEnabled = live;
    }
}

void MainWindow::ConfigureTransmitterEditor(void)
{
    int ret;

    qDebug() << "Setting Transmitter info...";

    SetTransmitPreemphasis(ui->comboBox_transmitter_preemph->currentIndex());
    if(fmtxCmdStatus!=FMTX_MODE_OK) goto configure_transmitter_err;

    SetTransmitAudioDeviation(ui->spinBox_transmitter_audioDev->value());
    if(fmtxCmdStatus!=FMTX_MODE_OK) goto configure_transmitter_err;

    SetTransmitPilotDeviation(ui->spinBox_transmitter_pilot_devi->value());
    if(fmtxCmdStatus!=FMTX_MODE_OK) goto configure_transmitter_err;

    SetTransmitPilotFrequency(ui->spinBox_transmitter_pilot_freq->value());
    if(fmtxCmdStatus!=FMTX_MODE_OK) goto configure_transmitter_err;

    SetTransmitRDSDeviation(ui->spinBox_transmitter_rds_devi->value());
    if(fmtxCmdStatus!=FMTX_MODE_OK) goto configure_transmitter_err;

    SetTransmitFrequency(ui->doubleSpinBox_transmitter_freq->value());
    if(fmtxCmdStatus!=FMTX_MODE_OK) goto configure_transmitter_err;

    querier->HidAccessMutex.lock();
    if(ui->checkBox_transmitter_enabled->isChecked()) {
        if(ui->checkBox_transmitter_antcap->isChecked())
            ret=fmtxTransmitterSetTunePower(ui->spinBox_transmitter_power->value(), ui->doubleSpinBox_transmitter_antcap_val->value());
        else
            ret=fmtxTransmitterSetTunePower(ui->spinBox_transmitter_power->value(), 0.0);
        SetTransmitIcon(ICON_TX);
    } else {
        ret=fmtxTransmitterSetTunePower(0, 0.0);
        SetTransmitIcon(ICON_NOTX);
    }
    querier->HidAccessMutex.unlock();
    if(ret != FMTX_MODE_OK) {
        qDebug() << "Can't enable transmission";
        SetTransmitIcon(ICON_WARN);
        goto configure_transmitter_err;
    }

    SetTransmitComponentFlags();
    if(fmtxCmdStatus!=FMTX_MODE_OK) goto configure_transmitter_err;

    qDebug() << "Transmitter info sended";
    return;

configure_transmitter_err:
    qDebug() << "Error while sending transmitter info";
}



/******************************************************************
 * Transmitter slots
 ******************************************************************/
void MainWindow::setAntCapNoEvent(double dVal)
{
    ui->doubleSpinBox_transmitter_antcap_val->blockSignals(true);
    ui->doubleSpinBox_transmitter_antcap_val->setValue(dVal);
    ui->doubleSpinBox_transmitter_antcap_val->blockSignals(false);
}


void MainWindow::SetTransmitPreemphasis(int iVal)
{
    if(!liveEditingEnabled)
        return;
    querier->HidAccessMutex.lock();
    if(fmtxTransmitterSetPreemphasisId(iVal) != FMTX_MODE_OK){
        qDebug() << "Can't set transmitter preemphasis flag";
        SetTransmitIcon(ICON_WARN);
    }
    querier->HidAccessMutex.unlock();
}

void MainWindow::SetTransmitAudioDeviation(int iVal)
{
    if(!liveEditingEnabled)
        return;
    querier->HidAccessMutex.lock();
    if(fmtxTransmitterSetAudioDeviation(iVal) != FMTX_MODE_OK){
        qDebug() << "Can't set transmitter audio deviation flag";
        SetTransmitIcon(ICON_WARN);
    }
    querier->HidAccessMutex.unlock();
}

void MainWindow::SetTransmitComponentFlags()
{
    uint8_t ComponentFlags = 0;
    if(!liveEditingEnabled)
        return;
    if(ui->checkBox_transmitter_lmr->isChecked())
        ComponentFlags |= TX_COMPONENT_ENABLE_LMR_MASK;
    if(ui->checkBox_transmitter_pilot->isChecked())
        ComponentFlags |= TX_COMPONENT_ENABLE_PILOT_MASK;
    if(ui->checkBox_transmitter_rds->isChecked())
        ComponentFlags |= TX_COMPONENT_ENABLE_RDS_MASK;
    querier->HidAccessMutex.lock();
    if(fmtxTransmitterSetComponentFlags(ComponentFlags) != FMTX_MODE_OK){
        qDebug() << "Can't set transmitter components flag";
        SetTransmitIcon(ICON_WARN);
    }

    querier->HidAccessMutex.unlock();
}

void MainWindow::SetTransmitPilotDeviation(int iVal)
{
    if(!liveEditingEnabled)
        return;
    querier->HidAccessMutex.lock();
    if(fmtxTransmitterSetPilotDeviation(iVal) != FMTX_MODE_OK){
        qDebug() << "Can't set transmitter pilot deviation flag";
        SetTransmitIcon(ICON_WARN);
    }
    querier->HidAccessMutex.unlock();
}

void MainWindow::SetTransmitPilotFrequency(int iVal)
{
    if(!liveEditingEnabled)
        return;
    querier->HidAccessMutex.lock();
    if(fmtxTransmitterSetPilotFrequency(iVal) != FMTX_MODE_OK){
        qDebug() << "Can't set transmitter pilot frequency flag";
        SetTransmitIcon(ICON_WARN);
    }
    querier->HidAccessMutex.unlock();
}

void MainWindow::SetTransmitRDSDeviation(int iVal)
{
    if(!liveEditingEnabled)
        return;
    querier->HidAccessMutex.lock();
    if(fmtxTransmitterSetRDSDeviation(iVal) != FMTX_MODE_OK){
        qDebug() << "Can't set transmitter audio deviation flag";
        SetTransmitIcon(ICON_WARN);
    }
    querier->HidAccessMutex.unlock();
}

void MainWindow::SetTransmitFrequency(double dVal)
{
    if(!liveEditingEnabled)
        return;
    querier->HidAccessMutex.lock();
    if(fmtxTransmitterSetTuneFreq(dVal) != FMTX_MODE_OK){
        qDebug() << "Can't set transmitter audio deviation flag";
        SetTransmitIcon(ICON_WARN);
    }
    querier->HidAccessMutex.unlock();
}

/* sets power and antcap at the same time */
int MainWindow::SetTransmitPower()
{
    uint8_t ret;
    ui->label_transmitter_level->setText(QString("%1")
                                         .arg(ui->spinBox_transmitter_power->value()));
    if(!liveEditingEnabled)
        return FMTX_MODE_NONE;
    querier->HidAccessMutex.lock();
    if(ui->checkBox_transmitter_antcap->isChecked()) {
        ret=fmtxTransmitterSetTunePower(ui->spinBox_transmitter_power->value(), ui->doubleSpinBox_transmitter_antcap_val->value());
    } else {
        ret=fmtxTransmitterSetTunePower(ui->spinBox_transmitter_power->value(), 0.0);
    }
    querier->HidAccessMutex.unlock();
    if(ret != FMTX_MODE_OK){
        qDebug() << "Can't set transmitter power and capacitor";
        SetTransmitIcon(ICON_WARN);
    }

    querier->timer.setInterval(2000);
    return ret;
}

void MainWindow::SetTransmitIcon( int iVal )
{
    qDebug() << "SetTransmitIcon" << iVal;
    switch(iVal){
    case ICON_NOTX:
        ui->commandLinkButton_station->setIcon(notxicon);
        trayIcon->setIcon(notxicon);
        ui->actionToggle_transmission->setIcon(notxicon);
        ui->actionToggle_transmission->setText(tr("Enable TX?"));
        break;
    case ICON_TX:
        ui->commandLinkButton_station->setIcon(txicon);
        trayIcon->setIcon(txicon);
        ui->actionToggle_transmission->setIcon(txicon);
        ui->actionToggle_transmission->setText(tr("Disable TX?"));
        break;
    case ICON_WARN:
        ui->commandLinkButton_station->setIcon(warningicon);
        trayIcon->setIcon(warningicon);
        break;
    case ICON_ERR:
        ui->commandLinkButton_station->setIcon(erroricon);
        trayIcon->setIcon(erroricon);
        break;
    case ICON_NODEV:
        ui->commandLinkButton_station->setIcon(nodeviceicon);
        trayIcon->setIcon(nodeviceicon);
        break;
    }
}

void MainWindow::SetTransmitEnabled( bool bEnabled )
{
    uint8_t ret;
    if(!liveEditingEnabled)
        return;

    qDebug() << QString("Transmission: %1").arg(bEnabled?"enabled":"disabled");

    if(bEnabled) {
        ret = SetTransmitPower();
        SetTransmitIcon(ICON_TX);
    } else {
        querier->HidAccessMutex.lock();
        ret=fmtxTransmitterSetTunePower(0, 0.0);
        querier->HidAccessMutex.unlock();
        querier->timer.setInterval(2000);
        SetTransmitIcon(ICON_NOTX);
    }

    if(ret != FMTX_MODE_OK){
        qDebug() << "Can't enable transmission";
        SetTransmitIcon(ICON_WARN);
    }
}

void MainWindow::toggleTransmitEnabled(int value)
{
    ui->frame_transmitter_tx->setVisible(value);
    ui->verticalSlider_main_power->setEnabled(value);
    SetTransmitEnabled( value );
}

void MainWindow::toggleTransmitAntCap(int value)
{
    ui->frame_transmitter_antcap->setEnabled(value);
    SetTransmitPower();
}

void MainWindow::toggleTransmitRds(int value)
{
    ui->frame_rds_deviation->setVisible(value);
    ui->widget_rds->setEnabled(value);
    SetTransmitComponentFlags();
}

void MainWindow::toggleTransmitPilot(int value)
{
    ui->frame_transmitter_pilot->setVisible(value);
    SetTransmitComponentFlags();
}
