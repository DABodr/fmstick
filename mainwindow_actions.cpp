/*
 * Part of FmStick, see LICENSE.txt.
 * Modified 2026-11-25 by Aurel: simple main screen and restyled advanced mode.
 * Modified 2026-11-25 by Aurel: web UI address shown again after a language change.
 * Modified 2026-11-26 by Aurel: a language change keeps the contents of fields and lists.
 */
#include "mainwindow.h"
#include "querier.h"
#include "ui_mainwindow.h"

#include "fmtx.h"
#include "logging.h"


/******************************************************************
 * Various Mainwindow actions
 ******************************************************************/
void MainWindow::retranslateUi()
{
    /* ui->retranslateUi() resets every translatable text of the .ui, the
       contents of line edits included (PS fields emptied, PI back to
       "0000", web login back to "Admin"), and refills the combo boxes,
       which loses their selection (QTBUG-3863). The language is applied at
       each startup, so keep what the user has, and block the signals
       meanwhile: no intermediate value may reach the device or the config. */
    QHash<QLineEdit *, QString> texts;
    QHash<QComboBox *, int> indexes;
    QHash<QObject *, bool> blocked;
    const QList<QLineEdit *> edits = findChildren<QLineEdit *>();
    for (QLineEdit *e : edits) {
        /* the editors inside spin boxes and combo boxes belong to them */
        if (qobject_cast<QAbstractSpinBox *>(e->parent()) || qobject_cast<QComboBox *>(e->parent()))
            continue;
        texts.insert(e, e->text());
        blocked.insert(e, e->blockSignals(true));
    }
    const QList<QComboBox *> combos = findChildren<QComboBox *>();
    for (QComboBox *c : combos) {
        indexes.insert(c, c->currentIndex());
        blocked.insert(c, c->blockSignals(true));
    }

    ui->retranslateUi(this);

    for (auto it = texts.constBegin(); it != texts.constEnd(); ++it)
        it.key()->setText(it.value());
    for (auto it = indexes.constBegin(); it != indexes.constEnd(); ++it)
        it.key()->setCurrentIndex(it.value());
    for (auto it = blocked.constBegin(); it != blocked.constEnd(); ++it)
        it.key()->blockSignals(it.value());

    retranslateUsbVolumeUi();
    showHTTPListenerAddress();
}

void MainWindow::applyLive(const std::function<void()> &change)
{
    int saved = liveEditingEnabled;
    if(deviceStatus)
        liveEditingEnabled = true;
    change();
    liveEditingEnabled = saved;
}

void MainWindow::toggleLiveEditing(bool bVal)
{
    if(deviceStatus)
        liveEditingEnabled = bVal;
}


void MainWindow::toggleConnection(bool bVal, bool toggleQuerier)
{
    qDebug() << __FUNCTION__ << ": " << bVal;
    /* actions */
    ui->actionRead_from_Device->setEnabled(bVal);
    ui->actionRead_from_EEPROM->setEnabled(bVal);
    ui->actionWrite_to_Device->setEnabled(bVal);
    ui->actionWrite_to_EEPROM->setEnabled(bVal);
    ui->actionToggle_transmission->setEnabled(bVal);
    /* hardware buttons */
    ui->checkBox_hw_run_txAtPowerUp->setEnabled(bVal);
    ui->pushButton_fe_powerdown->setEnabled(bVal);
    ui->pushButton_fe_powerup->setEnabled(bVal);
    ui->pushButton_fe_reset->setEnabled(bVal);
    ui->pushButton_open_firmware->setEnabled(bVal);

    if(bVal) {
        if(ui->checkBox_rds_fifo->isChecked()) {
            ui->groupBox_rds_sendmsg->setEnabled(bVal);
        }
        if(toggleQuerier)
            querier->timer.start();
        SetTransmitIcon(ICON_WARN);
        ui->actionConnect_to_device->setText("Disconnect?");
        ui->actionConnect_to_device->setIcon(QIcon(":/fmstick/images/connect_32.png"));
    } else {
        ui->groupBox_rds_sendmsg->setEnabled(bVal);
        if(toggleQuerier) {
            querier->timer.stop();
        }
        SetTransmitIcon(ICON_NODEV);
        ui->actionConnect_to_device->setIcon(QIcon(":/fmstick/images/disconnect_32.png"));
        ui->actionConnect_to_device->setText("Connect?");
    }
    deviceStatus = bVal;
}


void MainWindow::SetFrontendPowerDown() {
    qDebug() << __FUNCTION__;
    if(fmtxIoAppFeDown() != FMTX_MODE_POWER_DOWN) {
        qDebug() << "error while powering down";
    }
    SetTransmitIcon(ICON_WARN);
    querier->is_ready=false;
    liveEditingEnabled=false;
    ui->actionRead_from_Device->setEnabled(false);
    ui->actionWrite_to_Device->setEnabled(false);
    fmtxCmdStatus=FMTX_MODE_NONE;
}

int MainWindow::SetFrontendPowerUp() {
    int ret;
    qDebug() << __FUNCTION__;
    querier->HidAccessMutex.lock();
    ret = fmtxIoAppFeUp();
    querier->HidAccessMutex.unlock();
    switch(ret){
    case FMTX_MODE_POWER_UP:
        qDebug() << "Frontend powered up!";
        SetTransmitIcon(ICON_NOTX);
        fmtxCmdStatus=FMTX_MODE_OK;
        break;
    case FMTX_MODE_TRANSMITTING:
        qDebug() << "Frontend already powered up!";
        SetTransmitIcon(ICON_TX);
        fmtxCmdStatus=FMTX_MODE_OK;
        break;
    default:
        qDebug() << "Error while powering up";
        fmtxCmdStatus=FMTX_MODE_NONE;
        break;
    }

    if(ret == FMTX_MODE_POWER_UP || ret == FMTX_MODE_TRANSMITTING) {
        querier->is_ready=true;
        liveEditingEnabled=ui->checkBox_sw_live_edit->isChecked();
        ui->actionRead_from_Device->setEnabled(true);
        ui->actionWrite_to_Device->setEnabled(true);
    }

    return ret;
}

void MainWindow::SetFrontendReset() {
    qDebug() << __FUNCTION__;
    if(fmtxIoAppFeReset() != FMTX_MODE_POWER_DOWN)
        qDebug() << "Error while resetting frontend";
    querier->is_ready=false;
    liveEditingEnabled=false;
    SetTransmitIcon(ICON_WARN);
    ui->actionRead_from_Device->setEnabled(false);
    ui->actionWrite_to_Device->setEnabled(false);
    fmtxCmdStatus=FMTX_MODE_NONE;
}

void MainWindow::StationButtonClicked()
{
    qDebug() << __FUNCTION__;
    bool is_checked = ui->checkBox_transmitter_enabled->isChecked();
    ui->checkBox_transmitter_enabled->setChecked(!is_checked);
}


int MainWindow::GetFrontendParams()
{
    qDebug() << __FUNCTION__;
    liveEditingEnabled=false;

    DisplayAudioEditor();
    if(fmtxCmdStatus!=FMTX_MODE_OK)
        goto frontend_params_err;

    DisplayTransmitterEditor();
    if(fmtxCmdStatus!=FMTX_MODE_OK)
        goto frontend_params_err;

    DisplayRDSEditor();
    if(fmtxCmdStatus!=FMTX_MODE_OK)
        goto frontend_params_err;
    liveEditingEnabled=ui->checkBox_sw_live_edit->isChecked();
    /* run querier, so we can read interrupts and send rds timestamps */
    return fmtxCmdStatus;

frontend_params_err:
    liveEditingEnabled=false;
    qDebug() << "Can't get frontend parameters";
    SetTransmitIcon(ICON_ERR);
    return fmtxCmdStatus;
}

int MainWindow::SetFrontendParams()
{
    qDebug() << __FUNCTION__;
    liveEditingEnabled=true;

    ConfigureAudioEditor();
    if(fmtxCmdStatus!=FMTX_MODE_OK)
        goto frontend_params_err;

    ConfigureTransmitterEditor();
    if(fmtxCmdStatus!=FMTX_MODE_OK)
        goto frontend_params_err;

    ConfigureRDSEditor();
    if(fmtxCmdStatus!=FMTX_MODE_OK)
        goto frontend_params_err;

    if(!SetDigitalAudioHack())
        goto frontend_params_err;

    /* run querier, so we can read interrupts and send rds timestamps */
    qDebug() << "Frontend parameters setted correctly.";
    liveEditingEnabled=ui->checkBox_sw_live_edit->isChecked();
    return fmtxCmdStatus;

frontend_params_err:
    liveEditingEnabled=false;
    qDebug() << "Can't set frontend parameters";
    SetTransmitIcon(ICON_ERR);
    return fmtxCmdStatus;
}
