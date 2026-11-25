/*
 * Simple main screen of the control application (part of FmStick).
 * See simpleview.h.
 *
 * Copyright (C) 2026 Aurel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "simpleview.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "usbvolume.h"

#include <QtWidgets>
#include <cmath>

/* Power presets, dBµV (the Si4711 accepts 88..120). */
static const int POWER_PRESETS[3] = { 88, 104, 120 };
/* Audio deviation that gives a 75 kHz total with the 6.75 kHz pilot. */
static const int STANDARD_DEVIATION = 68250;
/* Below this the audio is inaudible on a receiver. */
static const uint32_t SILENT_DEVIATION = 1000;

/* Indicators and arrows: a stylesheet replaces Fusion's own drawing, so
   they have to be given back explicitly. */
#define CONTROLS_STYLE \
    "QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid #9AA3AE; border-radius: 4px; background: #FFFFFF; }" \
    "QCheckBox::indicator:hover { border-color: #1D4ED8; }" \
    "QCheckBox::indicator:checked { background: #1D4ED8; border-color: #1D4ED8; image: url(:/fmstick/images/ui-check.svg); }" \
    "QCheckBox::indicator:disabled { background: #F1F3F6; border-color: #C9CED6; }" \
    "QCheckBox::indicator:checked:disabled { background: #9DB3EA; border-color: #9DB3EA; }" \
    "QRadioButton::indicator { width: 16px; height: 16px; border: 1px solid #9AA3AE; border-radius: 9px; background: #FFFFFF; }" \
    "QRadioButton::indicator:checked { width: 8px; height: 8px; border: 5px solid #1D4ED8; }" \
    "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: center right; width: 22px; border: 0; }" \
    "QComboBox::down-arrow { image: url(:/fmstick/images/ui-chevron-down.svg); width: 12px; height: 12px; }" \
    "QComboBox QAbstractItemView { border: 1px solid #C9CED6; background: #FFFFFF; color: #1B1F24;" \
    "  selection-background-color: #E8EEFC; selection-color: #111418; }" \
    "QAbstractSpinBox { padding-right: 22px; }" \
    "QAbstractSpinBox::up-button { subcontrol-origin: border; subcontrol-position: top right; width: 20px;" \
    "  border: 0; border-left: 1px solid #E6E9EE; border-top-right-radius: 6px; background: transparent; }" \
    "QAbstractSpinBox::down-button { subcontrol-origin: border; subcontrol-position: bottom right; width: 20px;" \
    "  border: 0; border-left: 1px solid #E6E9EE; border-bottom-right-radius: 6px; background: transparent; }" \
    "QAbstractSpinBox::up-button:hover, QAbstractSpinBox::down-button:hover { background: #F1F3F6; }" \
    "QAbstractSpinBox::up-arrow { image: url(:/fmstick/images/ui-chevron-up.svg); width: 10px; height: 10px; }" \
    "QAbstractSpinBox::down-arrow { image: url(:/fmstick/images/ui-chevron-down.svg); width: 10px; height: 10px; }"

static const char *STYLE = CONTROLS_STYLE
    "QFrame#fmHeader, QFrame#fmFooter { background: #FFFFFF; border: 1px solid #DDE1E6; border-radius: 10px; }"
    "QFrame[fmCard=\"true\"] { background: #FFFFFF; border: 1px solid #DDE1E6; border-radius: 12px; }"
    "QLabel { color: #1B1F24; background: transparent; border: 0; }"
    "QLabel[fmRole=\"title\"] { font-weight: 600; color: #3A414B; }"
    "QLabel[fmRole=\"muted\"] { color: #5B6470; }"
    "QLabel[fmRole=\"app\"] { font-size: 17px; font-weight: 700; }"
    "QLabel[fmRole=\"unit\"] { font-size: 20px; font-weight: 600; color: #5B6470; }"
    "QPushButton[fmRole=\"round\"] { min-width: 44px; max-width: 44px; min-height: 44px; max-height: 44px;"
    "  border-radius: 22px; border: 1px solid #C9CED6; background: #FFFFFF; color: #1B1F24; font-size: 22px; }"
    "QPushButton[fmRole=\"round\"]:hover { background: #F1F3F6; }"
    "QPushButton[fmRole=\"secondary\"] { min-height: 34px; padding: 0 14px; border-radius: 8px;"
    "  border: 1px solid #C9CED6; background: #FFFFFF; color: #1B1F24; }"
    "QPushButton[fmRole=\"secondary\"]:hover { background: #F1F3F6; }"
    "QPushButton[fmRole=\"primary\"] { min-height: 34px; padding: 0 16px; border-radius: 8px; border: 0;"
    "  background: #1D4ED8; color: #FFFFFF; font-weight: 600; }"
    "QPushButton[fmRole=\"primary\"]:disabled { background: #E6E9EE; color: #5B6470; }"
    "QFrame#fmSeg { background: #F1F3F6; border: 0; border-radius: 10px; }"
    "QPushButton[fmRole=\"seg\"] { min-height: 36px; border: 1px solid transparent; border-radius: 8px;"
    "  background: transparent; color: #3A414B; font-weight: 600; }"
    "QPushButton[fmRole=\"seg\"]:checked { background: #FFFFFF; color: #111418; border: 1px solid #C9CED6; }"
    "QDoubleSpinBox#fmFreq { border: 0; background: transparent; color: #111418; font-size: 52px; font-weight: 700; }"
    "QWidget#fmBody QLineEdit, QWidget#fmBody QComboBox { min-height: 34px; padding: 0 10px; border-radius: 8px;"
    "  border: 1px solid #C9CED6; background: #FFFFFF; color: #1B1F24; }"
    "QWidget#fmBody QLineEdit:focus, QWidget#fmBody QComboBox:focus { border: 1px solid #1D4ED8; }"
    "QProgressBar { border: 0; border-radius: 5px; background: #E6E9EE; max-height: 10px; min-height: 10px; }";

/* Advanced mode: the original tabs, restyled to match the simple screen. */
static const char *ADVANCED_STYLE = CONTROLS_STYLE
    "QTabWidget::pane { border: 1px solid #DDE1E6; border-radius: 12px; background: #F7F8FA; top: -1px; }"
    "QTabBar::tab { padding: 8px 16px; margin-right: 4px; border: 1px solid transparent;"
    "  border-top-left-radius: 8px; border-top-right-radius: 8px; color: #3A414B; background: transparent; font-weight: 600; }"
    "QTabBar::tab:selected { background: #F7F8FA; color: #111418; border-color: #DDE1E6; border-bottom-color: #F7F8FA; }"
    "QTabBar::tab:hover:!selected { background: #E6E9EE; }"
    "QTabWidget QFrame { border: 0; background: transparent; }"
    "QGroupBox { background: #FFFFFF; border: 1px solid #DDE1E6; border-radius: 10px;"
    "  margin-top: 20px; padding: 8px 10px 10px 10px; font-weight: 600; color: #3A414B; }"
    "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 6px; padding: 0 4px; }"
    "QGroupBox QLabel, QGroupBox QCheckBox, QGroupBox QRadioButton { font-weight: 400; color: #1B1F24; background: transparent; }"
    "QTabWidget QLineEdit, QTabWidget QComboBox, QTabWidget QSpinBox, QTabWidget QDoubleSpinBox {"
    "  border: 1px solid #C9CED6; border-radius: 6px; padding: 3px 6px; background: #FFFFFF; color: #1B1F24;"
    "  min-height: 22px; font-weight: 400; selection-background-color: #1D4ED8; }"
    "QTabWidget QPlainTextEdit, QTabWidget QTextBrowser, QTabWidget QTableWidget {"
    "  border: 1px solid #C9CED6; border-radius: 6px; background: #FFFFFF; color: #1B1F24; font-weight: 400; }"
    "QTabWidget QLineEdit:focus, QTabWidget QComboBox:focus, QTabWidget QSpinBox:focus,"
    "QTabWidget QDoubleSpinBox:focus, QTabWidget QPlainTextEdit:focus { border: 1px solid #1D4ED8; }"
    "QTabWidget QLineEdit:disabled, QTabWidget QComboBox:disabled, QTabWidget QSpinBox:disabled,"
    "QTabWidget QDoubleSpinBox:disabled { background: #F1F3F6; color: #8A929C; }"
    "QTabWidget QPushButton { border: 1px solid #C9CED6; border-radius: 6px; padding: 5px 12px;"
    "  background: #FFFFFF; color: #1B1F24; font-weight: 500; }"
    "QTabWidget QPushButton:hover { background: #F1F3F6; }"
    "QTabWidget QPushButton:disabled { color: #8A929C; background: #F1F3F6; }"
    "QTabWidget QCheckBox, QTabWidget QRadioButton { spacing: 8px; }";

QString FreqSpinBox::textFromValue(double value) const
{
    /* one decimal unless the frequency needs two (50 kHz steps) */
    bool tenth = std::fabs(value * 10 - std::round(value * 10)) < 0.001;
    return locale().toString(value, 'f', tenth ? 1 : 2);
}

static QLabel *roleLabel(const char *role, QWidget *parent = nullptr)
{
    QLabel *l = new QLabel(parent);
    l->setProperty("fmRole", role);
    return l;
}

static QFrame *card(QVBoxLayout **layout)
{
    QFrame *f = new QFrame();
    f->setProperty("fmCard", true);
    *layout = new QVBoxLayout(f);
    (*layout)->setContentsMargins(18, 16, 18, 16);
    (*layout)->setSpacing(10);
    return f;
}

static void restyle(QWidget *w, const QString &css)
{
    if (w->styleSheet() != css)
        w->setStyleSheet(css);
}

SimpleView::SimpleView(MainWindow *mw, Ui::MainWindow *ui)
    : QObject(mw), mw(mw), ui(ui), advanced(false), mini(false), dirty(false),
      inputLevel(-70), onAirNow(false),
      bannerFix(FixNone), usbVolumeMax(0)
{
}

void SimpleView::install()
{
    buildHeader();
    buildBanner();
    buildBody();
    buildFooter();

    for (QWidget *w : { (QWidget *)header, (QWidget *)banner, body, (QWidget *)footer })
        w->setStyleSheet(STYLE);

    ui->tabWidget->setStyleSheet(ADVANCED_STYLE);
    ui->tabWidget->setDocumentMode(false);

    QVBoxLayout *root = ui->verticalLayout_49;
    root->insertWidget(0, header);
    root->insertWidget(1, banner);
    root->insertWidget(2, body);
    root->addWidget(footer);

    /* follow the advanced widgets, whoever changes them */
    connect(ui->doubleSpinBox_transmitter_freq, SIGNAL(valueChanged(double)), this, SLOT(syncFromUi()));
    connect(ui->spinBox_transmitter_power, SIGNAL(valueChanged(int)), this, SLOT(syncFromUi()));
    connect(ui->checkBox_transmitter_enabled, SIGNAL(stateChanged(int)), this, SLOT(syncFromUi()));
    connect(ui->lineEdit_station_id, SIGNAL(textChanged(QString)), this, SLOT(syncFromUi()));
    connect(ui->plainTextEdit_rds_rt, SIGNAL(textChanged()), this, SLOT(syncFromUi()));
    connect(ui->comboBox_rds_pty, SIGNAL(currentIndexChanged(int)), this, SLOT(syncFromUi()));
    connect(ui->spinBox_transmitter_audioDev, SIGNAL(valueChanged(int)), this, SLOT(refresh()));
    connect(ui->progressBar_audio_asq_measured, SIGNAL(valueChanged(int)), this, SLOT(inputLevelChanged(int)));
    connect(ui->comboBox_audioPreset, SIGNAL(currentIndexChanged(int)), this, SLOT(syncFromUi()));

    /* state that has no signal (connection, USB volume) */
    /* also catches RT/PS changes the app makes with signals blocked
       (RadioText file, PS fields of the RDS tab) */
    connect(&timer, SIGNAL(timeout()), this, SLOT(syncFromUi()));
    timer.start(1000);

    /* the window retranslates on language change */
    mw->installEventFilter(this);

    retranslate();
    syncFromUi();
    inputLevelChanged(ui->progressBar_audio_asq_measured->value());
    setAdvanced(false);
}

void SimpleView::buildHeader()
{
    header = new QFrame();
    header->setObjectName("fmHeader");
    QHBoxLayout *h = new QHBoxLayout(header);
    h->setContentsMargins(14, 10, 14, 10);
    h->setSpacing(12);

    QLabel *icon = new QLabel();
    icon->setPixmap(QIcon(":/fmstick/images/fmstick.png").pixmap(32, 32));
    h->addWidget(icon);

    QVBoxLayout *names = new QVBoxLayout();
    names->setSpacing(0);
    QLabel *app = roleLabel("app");
    app->setText("FmStick");
    names->addWidget(app);
    deviceLabel = roleLabel("muted");
    names->addWidget(deviceLabel);
    h->addLayout(names);

    statusPill = new QLabel();
    h->addWidget(statusPill);
    h->addStretch(1);

    modeButton = new QPushButton();
    modeButton->setProperty("fmRole", "secondary");
    connect(modeButton, SIGNAL(clicked()), this, SLOT(toggleAdvanced()));
    h->addWidget(modeButton);
}

void SimpleView::buildBanner()
{
    banner = new QFrame();
    banner->setObjectName("fmBanner");
    QHBoxLayout *h = new QHBoxLayout(banner);
    h->setContentsMargins(14, 10, 14, 10);
    h->setSpacing(12);
    bannerText = new QLabel();
    bannerText->setWordWrap(true);
    h->addWidget(bannerText, 1);
    bannerButton = new QPushButton();
    connect(bannerButton, SIGNAL(clicked()), this, SLOT(bannerAction()));
    h->addWidget(bannerButton);
    banner->hide();
}

void SimpleView::buildBody()
{
    body = new QWidget();
    body->setObjectName("fmBody");
    QHBoxLayout *columns = new QHBoxLayout(body);
    columns->setContentsMargins(0, 0, 0, 0);
    columns->setSpacing(12);
    QVBoxLayout *left = new QVBoxLayout();
    QVBoxLayout *right = new QVBoxLayout();
    left->setSpacing(12);
    right->setSpacing(12);
    columns->addLayout(left, 5);
    columns->addLayout(right, 4);
    QVBoxLayout *c;

    /* Frequency */
    QFrame *freqCard = card(&c);
    freqTitle = roleLabel("title");
    c->addWidget(freqTitle);
    QHBoxLayout *fr = new QHBoxLayout();
    fr->setSpacing(14);
    fr->addStretch(1);
    freqDown = new QPushButton(QString::fromUtf8("−"));
    freqDown->setProperty("fmRole", "round");
    fr->addWidget(freqDown);
    freq = new FreqSpinBox();
    freq->setObjectName("fmFreq");
    freq->setButtonSymbols(QAbstractSpinBox::NoButtons);
    freq->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    freq->setKeyboardTracking(false);   /* retune on Enter, not on each digit */
    freq->setDecimals(2);
    freq->setSingleStep(0.1);
    freq->setRange(ui->doubleSpinBox_transmitter_freq->minimum(), ui->doubleSpinBox_transmitter_freq->maximum());
    freq->setMinimumWidth(190);
    fr->addWidget(freq);
    freqUnit = roleLabel("unit");
    freqUnit->setText("MHz");
    fr->addWidget(freqUnit);
    freqUp = new QPushButton("+");
    freqUp->setProperty("fmRole", "round");
    fr->addWidget(freqUp);
    fr->addStretch(1);
    c->addLayout(fr);
    freqTip = roleLabel("muted");
    freqTip->setWordWrap(true);
    c->addWidget(freqTip);
    connect(freqDown, &QPushButton::clicked, this, [this]() { freqStep(-1); });
    connect(freqUp, &QPushButton::clicked, this, [this]() { freqStep(1); });
    connect(freq, SIGNAL(valueChanged(double)), this, SLOT(freqEdited(double)));
    left->addWidget(freqCard);

    /* Power */
    QFrame *powerCard = card(&c);
    QHBoxLayout *pt = new QHBoxLayout();
    powerTitle = roleLabel("title");
    pt->addWidget(powerTitle);
    pt->addStretch(1);
    powerDetail = roleLabel("muted");
    pt->addWidget(powerDetail);
    c->addLayout(pt);
    QFrame *seg = new QFrame();
    seg->setObjectName("fmSeg");
    QHBoxLayout *sl = new QHBoxLayout(seg);
    sl->setContentsMargins(4, 4, 4, 4);
    sl->setSpacing(4);
    powerGroup = new QButtonGroup(this);
    powerGroup->setExclusive(false);   /* a custom power checks none of them */
    for (int i = 0; i < 3; i++) {
        powerButtons[i] = new QPushButton();
        powerButtons[i]->setProperty("fmRole", "seg");
        powerButtons[i]->setCheckable(true);
        powerGroup->addButton(powerButtons[i], i);
        sl->addWidget(powerButtons[i]);
    }
    connect(powerGroup, SIGNAL(idClicked(int)), this, SLOT(powerPicked(int)));
    c->addWidget(seg);
    powerNote = roleLabel("muted");
    powerNote->setWordWrap(true);
    c->addWidget(powerNote);
    left->addWidget(powerCard);

    /* On / off */
    txButton = new QPushButton();
    txButton->setObjectName("fmTxButton");
    txButton->setMinimumHeight(54);
    connect(txButton, SIGNAL(clicked()), this, SLOT(toggleTransmission()));
    left->addWidget(txButton);
    left->addStretch(1);

    /* Sound */
    QFrame *soundCard = card(&c);
    soundTitle = roleLabel("title");
    c->addWidget(soundTitle);
    QHBoxLayout *lt = new QHBoxLayout();
    levelTitle = new QLabel();
    lt->addWidget(levelTitle);
    lt->addStretch(1);
    levelValue = new QLabel();
    lt->addWidget(levelValue);
    c->addLayout(lt);
    levelBar = new QProgressBar();
    levelBar->setRange(0, 70);
    levelBar->setTextVisible(false);
    c->addWidget(levelBar);
    QHBoxLayout *mt = new QHBoxLayout();
    modTitle = new QLabel();
    mt->addWidget(modTitle);
    mt->addStretch(1);
    modValue = new QLabel();
    mt->addWidget(modValue);
    c->addLayout(mt);
    modBar = new QProgressBar();
    modBar->setRange(0, 100);
    modBar->setTextVisible(false);
    c->addWidget(modBar);
    /* Not the chip's overmodulation flag: the transmitter's firmware latches
       it after the first event, so it stays set even at -42 dBFS. The
       measured input level is live, so warn from it instead. */
    levelWarning = new QLabel();
    levelWarning->setWordWrap(true);
    levelWarning->setStyleSheet("color: #B91C1C; font-weight: 600;");
    levelWarning->hide();
    c->addWidget(levelWarning);
    /* the compressor/limiter presets of the Sound tab, same list */
    dspLabel = new QLabel();
    c->addWidget(dspLabel);
    dspCombo = new QComboBox();
    dspCombo->setModel(ui->comboBox_audioPreset->model());
    dspLabel->setBuddy(dspCombo);
    c->addWidget(dspCombo);
    connect(dspCombo, SIGNAL(activated(int)), this, SLOT(dspPicked(int)));
    right->addWidget(soundCard);

    /* RDS */
    QFrame *rdsCard = card(&c);
    rdsTitle = roleLabel("title");
    c->addWidget(rdsTitle);
    QHBoxLayout *pl = new QHBoxLayout();
    psLabel = new QLabel();
    pl->addWidget(psLabel);
    pl->addStretch(1);
    psCount = roleLabel("muted");
    pl->addWidget(psCount);
    c->addLayout(pl);
    psEdit = new QLineEdit();
    psLabel->setBuddy(psEdit);
    c->addWidget(psEdit);
    psHint = roleLabel("muted");
    psHint->setWordWrap(true);
    c->addWidget(psHint);
    QHBoxLayout *rl = new QHBoxLayout();
    rtLabel = new QLabel();
    rl->addWidget(rtLabel);
    rl->addStretch(1);
    rtCount = roleLabel("muted");
    rl->addWidget(rtCount);
    c->addLayout(rl);
    rtEdit = new QLineEdit();
    rtEdit->setMaxLength(64);
    rtLabel->setBuddy(rtEdit);
    c->addWidget(rtEdit);
    rtHint = roleLabel("muted");
    rtHint->setWordWrap(true);
    c->addWidget(rtHint);
    ptyLabel = new QLabel();
    c->addWidget(ptyLabel);
    ptyCombo = new QComboBox();
    ptyCombo->setModel(ui->comboBox_rds_pty->model());   /* same list as the RDS tab */
    ptyLabel->setBuddy(ptyCombo);
    c->addWidget(ptyCombo);
    c->addStretch(1);
    connect(psEdit, SIGNAL(textEdited(QString)), this, SLOT(psEdited(QString)));
    connect(rtEdit, SIGNAL(editingFinished()), this, SLOT(rtEdited()));
    connect(rtEdit, &QLineEdit::textEdited, this, [this](const QString &t) {
        rtCount->setText(QString("%1 / 64").arg(t.length()));
    });
    connect(ptyCombo, SIGNAL(activated(int)), this, SLOT(ptyPicked(int)));
    right->addWidget(rdsCard, 1);
}

void SimpleView::buildFooter()
{
    footer = new QFrame();
    footer->setObjectName("fmFooter");
    QHBoxLayout *h = new QHBoxLayout(footer);
    h->setContentsMargins(14, 8, 14, 8);
    h->setSpacing(10);
    footerDot = new QLabel();
    footerDot->setFixedSize(10, 10);
    h->addWidget(footerDot);
    footerText = new QLabel();
    h->addWidget(footerText, 1);
    saveButton = new QPushButton();
    saveButton->setProperty("fmRole", "primary");
    connect(saveButton, SIGNAL(clicked()), this, SLOT(saveToEeprom()));
    h->addWidget(saveButton);
}

void SimpleView::retranslate()
{
    modeButton->setText(advanced ? tr("Simple mode") : tr("Advanced mode"));
    freqTitle->setText(tr("Frequency"));
    freqDown->setToolTip(tr("Lower the frequency by 0.1 MHz"));
    freqDown->setAccessibleName(freqDown->toolTip());
    freqUp->setToolTip(tr("Raise the frequency by 0.1 MHz"));
    freqUp->setAccessibleName(freqUp->toolTip());
    freq->setAccessibleName(tr("Frequency in MHz"));
    freqTip->setText(tr("Pick a frequency where your radio only hears static, away from local stations."));
    powerTitle->setText(tr("Transmit power"));
    powerButtons[0]->setText(tr("Low"));
    powerButtons[1]->setText(tr("Medium"));
    powerButtons[2]->setText(tr("Maximum"));
    powerNote->setText(tr("At high power, check what the regulations of your country allow."));
    soundTitle->setText(tr("Sound"));
    levelTitle->setText(tr("Input level"));
    modTitle->setText(tr("Modulation"));
    levelWarning->setText(tr("Input level very high: lower the volume of the source."));
    dspLabel->setText(tr("Sound processing"));
    dspCombo->setToolTip(tr("Compressor and limiter built into the transmitter: they even out the volume\n"
                            "and cap the peaks. Details in Advanced mode, Sound tab."));
    rdsTitle->setText(tr("RDS information"));
    psLabel->setText(tr("Station name"));
    rtLabel->setText(tr("Radio text"));
    rtHint->setText(tr("Scrolling text shown by the receiver, 64 characters at most."));
    ptyLabel->setText(tr("Programme type"));
    saveButton->setText(tr("Keep at startup"));
    saveButton->setToolTip(tr("Write these settings to the transmitter's memory (EEPROM)."));
    syncFromUi();
    refresh();
}

bool SimpleView::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == mw && event->type() == QEvent::LanguageChange)
        retranslate();
    return QObject::eventFilter(obj, event);
}

/* ---------------------------------------------------------------- state */

SimpleView::State SimpleView::currentState(uint32_t *effectiveDeviation, int *usbVolume, int *usbMax) const
{
    int vol = -1, max = 0;
    uint32_t setting = ui->spinBox_transmitter_audioDev->value();
    uint32_t effective = setting;

    if (usbVolumeGet(&vol, &max) == 0 && max > 0)
        effective = usbVolumeEffectiveDeviation(setting, vol, max);
    else
        vol = -1;
    *effectiveDeviation = effective;
    *usbVolume = vol;
    *usbMax = max;

    if (!mw->deviceStatus)
        return Disconnected;
    if (!ui->checkBox_transmitter_enabled->isChecked())
        return Stopped;
    if (effective < SILENT_DEVIATION)
        return Silent;
    return OnAir;
}

void SimpleView::refresh()
{
    uint32_t effective = 0;
    int vol = -1, max = 0;
    State state = currentState(&effective, &vol, &max);
    if (vol >= 0)
        usbVolumeMax = max;

    /* header */
    QString cpu = ui->lineEdit_hw_id_cpuid->text();
    deviceLabel->setText(state == Disconnected ? tr("No transmitter detected")
                         : cpu.isEmpty() ? tr("Transmitter connected")
                                         : tr("Transmitter connected · %1").arg(cpu));
    QString pill, pillCss;
    switch (state) {
    case OnAir:  pill = tr("On air");               pillCss = "background:#E7F6EC; color:#14532D;"; break;
    case Silent: pill = tr("On air · no sound"); pillCss = "background:#FEF3E2; color:#7C2D12;"; break;
    case Stopped: pill = tr("Stopped");             pillCss = "background:#EEF0F3; color:#3A414B;"; break;
    default:     pill = tr("Transmitter unplugged"); pillCss = "background:#FDECEC; color:#7F1D1D;"; break;
    }
    statusPill->setText(QString::fromUtf8("●  ") + pill);
    restyle(statusPill, "QLabel { border-radius: 12px; padding: 4px 12px; font-weight: 600; " + pillCss + " }");

    /* alert banner */
    bannerFix = FixNone;
    QString text, action;
    bool error = false;
    if (state == Silent && vol == 0) {
        bannerFix = FixUsbVolume;
        text = tr("The transmitter's USB volume is at 0: the carrier and RDS are on air, but no sound.");
        action = tr("Set volume to maximum");
    } else if (state == Silent) {
        bannerFix = FixDeviation;
        text = tr("The modulation level is set almost to zero: the carrier and RDS are on air, but no sound.");
        action = tr("Restore the standard level");
    } else if (state == Disconnected) {
        error = true;
        if (ui->checkBox_sw_autoconnect->isChecked()) {
            text = tr("Plug the transmitter into a USB port: it will connect automatically.");
        } else {
            bannerFix = FixConnect;
            text = tr("The transmitter is not connected.");
            action = tr("Connect");
        }
    }
    banner->setVisible(!text.isEmpty() && !mini);
    bannerText->setText(text);
    bannerButton->setVisible(bannerFix != FixNone);
    bannerButton->setText(action);
    restyle(banner, error
            ? "QFrame#fmBanner { background:#FDECEC; border:1px solid #F5B5B5; border-radius:10px; } QLabel { color:#7F1D1D; }"
              "QPushButton { background:#B91C1C; color:#FFFFFF; border:0; border-radius:8px; padding:7px 14px; font-weight:600; }"
            : "QFrame#fmBanner { background:#FEF3E2; border:1px solid #F8C98F; border-radius:10px; } QLabel { color:#7C2D12; }"
              "QPushButton { background:#9A3412; color:#FFFFFF; border:0; border-radius:8px; padding:7px 14px; font-weight:600; }");

    /* controls need the device */
    bool connected = state != Disconnected;
    body->setEnabled(connected);
    saveButton->setEnabled(connected && dirty);

    /* on / off */
    bool onAir = state == OnAir || state == Silent;
    txButton->setText(onAir ? tr("Stop transmitting") : tr("Transmit"));
    restyle(txButton, onAir
            ? "QPushButton { background:#FFFFFF; color:#B91C1C; border:2px solid #E9A3A3; border-radius:12px; font-size:17px; font-weight:700; }"
              "QPushButton:hover { background:#FDF2F2; }"
            : "QPushButton { background:#15803D; color:#FFFFFF; border:0; border-radius:12px; font-size:17px; font-weight:700; }"
              "QPushButton:hover { background:#166534; } QPushButton:disabled { background:#E6E9EE; color:#5B6470; }");

    /* modulation: 100 % = the standard 68.25 kHz audio deviation */
    int percent = (int)((effective * 100 + STANDARD_DEVIATION / 2) / STANDARD_DEVIATION);
    QString modText, modColor = "#16A34A", modTextColor = "#14532D";
    if (state == Disconnected) {
        modText = QString::fromUtf8("—");
        percent = 0;
    } else if (state == Stopped) {
        modText = tr("transmission stopped");
        percent = 0;
        modColor = "#8A929C";
        modTextColor = "#5B6470";
    } else if (state == Silent) {
        modText = tr("%1 % · no sound transmitted").arg(percent);
        modColor = "#EA580C";
        modTextColor = "#9A3412";
    } else if (percent < 60) {
        modText = tr("%1 % · low").arg(percent);
        modColor = "#EAB308";
        modTextColor = "#854D0E";
    } else if (percent <= 100) {
        modText = tr("%1 % · correct").arg(percent);
    } else {
        modText = tr("%1 % · too strong").arg(percent);
        modColor = "#DC2626";
        modTextColor = "#991B1B";
    }
    modValue->setText(modText);
    restyle(modValue, "QLabel { font-weight: 600; color: " + modTextColor + "; }");
    modBar->setValue(qBound(0, percent, 100));
    restyle(modBar, "QProgressBar::chunk { border-radius: 5px; background: " + modColor + "; }");
    if (vol >= 0 && vol < usbVolumeMax && state == OnAir)
        modBar->setToolTip(tr("Reduced by the USB volume (%1/%2)").arg(vol).arg(usbVolumeMax));
    else
        modBar->setToolTip(QString());

    onAirNow = state == OnAir;
    levelWarning->setVisible(onAirNow && inputLevel > -3);

    /* footer */
    footerText->setText(dirty ? tr("Changes applied live, not yet kept for the next startup")
                              : tr("No change since the transmitter was read"));
    restyle(footerDot, QString("QLabel { border-radius: 5px; background: %1; }").arg(dirty ? "#B45309" : "#16A34A"));
}

void SimpleView::inputLevelChanged(int dB)
{
    /* the advanced meter goes from -70 to 0 dB */
    inputLevel = dB;
    levelWarning->setVisible(onAirNow && dB > -3);
    levelBar->setValue(qBound(0, dB + 70, 70));
    levelValue->setText(mw->deviceStatus ? QString("%1 dB").arg(dB) : QString::fromUtf8("—"));
    const char *color = dB > -3 ? "#DC2626" : dB > -10 ? "#EAB308" : "#16A34A";
    restyle(levelBar, QString("QProgressBar::chunk { border-radius: 5px; background: %1; }").arg(color));
}

void SimpleView::syncFromUi()
{
    freq->blockSignals(true);
    freq->setValue(ui->doubleSpinBox_transmitter_freq->value());
    freq->blockSignals(false);

    int power = ui->spinBox_transmitter_power->value();
    int preset = -1;
    for (int i = 0; i < 3; i++) {
        powerButtons[i]->setChecked(power == POWER_PRESETS[i]);
        if (power == POWER_PRESETS[i])
            preset = i;
    }
    powerDetail->setText(preset < 0 ? tr("Custom: %1 dBµV").arg(power)
                                    : QString::fromUtf8("%1 dBµV").arg(power));

    QString ps = ui->lineEdit_station_id->text();
    if (!psEdit->hasFocus() && psEdit->text() != ps)
        psEdit->setText(ps);
    psCount->setText(QString("%1 / 8").arg(psEdit->text().length()));
    psHint->setVisible(psEdit->text().length() > 8);
    psHint->setText(tr("More than 8 characters: receivers show the name in parts of 8."));

    QString rt = ui->plainTextEdit_rds_rt->toPlainText();
    while (rt.endsWith(' '))
        rt.chop(1);
    if (!rtEdit->hasFocus() && rtEdit->text() != rt)
        rtEdit->setText(rt);
    rtCount->setText(QString("%1 / 64").arg(rtEdit->text().length()));

    if (dspCombo->currentIndex() != ui->comboBox_audioPreset->currentIndex())
        dspCombo->setCurrentIndex(ui->comboBox_audioPreset->currentIndex());

    if (ptyCombo->currentIndex() != ui->comboBox_rds_pty->currentIndex())
        ptyCombo->setCurrentIndex(ui->comboBox_rds_pty->currentIndex());

    refresh();
}

/* -------------------------------------------------------------- actions */

void SimpleView::markDirty()
{
    dirty = true;
    refresh();
}

void SimpleView::markClean()
{
    dirty = false;
    syncFromUi();
}

void SimpleView::freqStep(int steps)
{
    freq->stepBy(steps);
}

void SimpleView::freqEdited(double value)
{
    mw->applyLive([&]() { ui->doubleSpinBox_transmitter_freq->setValue(value); });
    markDirty();
}

void SimpleView::powerPicked(int id)
{
    mw->applyLive([&]() { ui->spinBox_transmitter_power->setValue(POWER_PRESETS[id]); });
    syncFromUi();   /* also re-checks the right button */
    markDirty();
}

void SimpleView::toggleTransmission()
{
    mw->applyLive([&]() { mw->StationButtonClicked(); });
    markDirty();
}

void SimpleView::psEdited(const QString &text)
{
    mw->applyLive([&]() { ui->lineEdit_station_id->setText(text); });
    psCount->setText(QString("%1 / 8").arg(text.length()));
    psHint->setVisible(text.length() > 8);
    markDirty();
}

void SimpleView::rtEdited()
{
    QString current = ui->plainTextEdit_rds_rt->toPlainText();
    while (current.endsWith(' '))
        current.chop(1);
    if (current == rtEdit->text())
        return;
    mw->applyLive([&]() { ui->plainTextEdit_rds_rt->setPlainText(rtEdit->text()); });
    markDirty();
}

void SimpleView::ptyPicked(int index)
{
    mw->applyLive([&]() { ui->comboBox_rds_pty->setCurrentIndex(index); });
    markDirty();
}

void SimpleView::dspPicked(int index)
{
    if (index == ui->comboBox_audioPreset->currentIndex())
        return;
    mw->applyLive([&]() { ui->comboBox_audioPreset->setCurrentIndex(index); });
    markDirty();
}

void SimpleView::bannerAction()
{
    switch (bannerFix) {
    case FixUsbVolume:
        if (usbVolumeMax > 0)
            mw->SetUsbVolume(usbVolumeMax);
        break;
    case FixDeviation:
        mw->applyLive([&]() { ui->spinBox_transmitter_audioDev->setValue(STANDARD_DEVIATION); });
        markDirty();
        break;
    case FixConnect:
        mw->SetConnectionState();
        break;
    case FixNone:
        break;
    }
    refresh();
}

void SimpleView::saveToEeprom()
{
    if (mw->SaveEEPROMParams())
        markClean();
}

/* ----------------------------------------------------------- simple/advanced */

void SimpleView::setLayoutVisible(QLayout *layout, bool visible)
{
    for (int i = 0; i < layout->count(); i++) {
        QLayoutItem *item = layout->itemAt(i);
        if (item->widget())
            item->widget()->setVisible(visible);
        else if (item->layout())
            setLayoutVisible(item->layout(), visible);
    }
}

void SimpleView::applyVisibility()
{
    header->setVisible(!mini);
    footer->setVisible(!mini);
    body->setVisible(!mini && !advanced);
    if (mini)
        banner->hide();
    /* the original compact strip: mini mode only (the header replaces it) */
    setLayoutVisible(ui->horizontalLayout_44, mini);
    ui->tabWidget->setVisible(!mini && advanced);
}

void SimpleView::setMiniMode(bool m)
{
    mini = m;
    applyVisibility();
    refresh();
}

void SimpleView::setAdvanced(bool adv)
{
    advanced = adv;
    applyVisibility();
    modeButton->setText(advanced ? tr("Simple mode") : tr("Advanced mode"));
    /* shrink to the new content once the layouts have been updated */
    QTimer::singleShot(0, mw, [this]() { mw->resize(1, 1); });
}

void SimpleView::toggleAdvanced()
{
    setAdvanced(!advanced);
}
