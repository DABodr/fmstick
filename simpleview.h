/*
 * Simple main screen of the control application (part of FmStick).
 *
 * Shows what most people need (on/off, frequency, power, sound, RDS name
 * and text) and hides the Si4711 registers behind an "Advanced mode"
 * button that brings back the original tabs.
 *
 * It does not talk to the device itself: it drives the existing widgets of
 * the advanced tabs, so live editing, the EEPROM, the configuration file
 * and the web UI keep working exactly as before.
 *
 * Copyright (C) 2026 Aurel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SIMPLEVIEW_H
#define SIMPLEVIEW_H

#include <QObject>
#include <QTimer>
#include <QDoubleSpinBox>

class MainWindow;
class QFrame;
class QWidget;
class QLabel;
class QPushButton;
class QLineEdit;
class QComboBox;
class QProgressBar;
class QButtonGroup;
class QLayout;
namespace Ui { class MainWindow; }

/* Frequency field: "107,4" rather than "107,40", in the user's locale. */
class FreqSpinBox : public QDoubleSpinBox
{
    Q_OBJECT
public:
    explicit FreqSpinBox(QWidget *parent = nullptr) : QDoubleSpinBox(parent) {}
    QString textFromValue(double value) const override;
};

class SimpleView : public QObject
{
    Q_OBJECT
public:
    SimpleView(MainWindow *mw, Ui::MainWindow *ui);

    /* Adds the header, alert banner, simple screen and footer to the window. */
    void install();
    bool isAdvanced() const { return advanced; }
    void setAdvanced(bool adv);
    /* Mini mode keeps the original compact strip only. */
    void setMiniMode(bool mini);
    /* The device was (re)read or its config saved to EEPROM. */
    void markClean();

public slots:
    void refresh();
    void syncFromUi();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void toggleAdvanced();
    void freqStep(int steps);
    void freqEdited(double value);
    void powerPicked(int id);
    void toggleTransmission();
    void psEdited(const QString &text);
    void rtEdited();
    void ptyPicked(int index);
    void bannerAction();
    void saveToEeprom();
    void inputLevelChanged(int dB);
    void dspPicked(int index);

private:
    enum State { Disconnected, Stopped, Silent, OnAir };
    enum BannerFix { FixNone, FixUsbVolume, FixDeviation, FixConnect };

    MainWindow *mw;
    Ui::MainWindow *ui;
    bool advanced;
    bool mini;
    bool dirty;
    BannerFix bannerFix;
    int usbVolumeMax;
    QTimer timer;

    QFrame *header, *banner, *footer;
    QWidget *body;
    QLabel *deviceLabel, *statusPill, *bannerText;
    QPushButton *modeButton, *bannerButton;
    QLabel *freqTitle, *freqUnit, *freqTip;
    QPushButton *freqDown, *freqUp;
    FreqSpinBox *freq;
    QLabel *powerTitle, *powerDetail, *powerNote;
    QButtonGroup *powerGroup;
    QPushButton *powerButtons[3];
    QPushButton *txButton;
    QLabel *soundTitle, *levelTitle, *levelValue, *modTitle, *modValue, *levelWarning;
    QLabel *dspLabel;
    QComboBox *dspCombo;
    int inputLevel;
    bool onAirNow;
    QProgressBar *levelBar, *modBar;
    QLabel *rdsTitle, *psLabel, *psCount, *psHint, *rtLabel, *rtCount, *rtHint, *ptyLabel;
    QLineEdit *psEdit, *rtEdit;
    QComboBox *ptyCombo;
    QLabel *footerDot, *footerText;
    QPushButton *saveButton;

    void buildHeader();
    void buildBanner();
    void buildBody();
    void buildFooter();
    void retranslate();
    State currentState(uint32_t *effectiveDeviation, int *usbVolume, int *usbMax) const;
    void applyVisibility();
    static void setLayoutVisible(QLayout *layout, bool visible);
    void markDirty();
};

#endif // SIMPLEVIEW_H
