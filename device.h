#ifndef DEVICE_H
#define DEVICE_H

#include <QObject>
#include <QString>
#include <QAbstractButton>
#include <QTimer>
#include <QDebug>

#include "defs.h"

enum State {Off, ChoosingSession, ChoosingSavedTherapy, InSession, Paused};

class Device : public QObject
{
    Q_OBJECT
public:
    explicit Device(QObject *parent = nullptr);

    // getters
    State getState() const;
    double getBatteryLevel() const;

private:
    State state;
    QTimer sessionTimer;
    QTimer powerButtonTimer;
    QTimer batteryLevelTimer;
    double batteryLevel;
    int intensity;

    // this is peter guessing at how this will work
    // highlighted / currently selected
    int selectedSessionGroup; // (time) 0, 1, 2
    int selectedSessionType; // (frequency) 0, 1, 2, 3

    // stored data of the groups and sessions we have, set up in ctor
//    SessionGroup sessionGroups[3];
//    SessionType sessionTypes[4];

    void powerOn();
    void powerOff(bool);

public slots:
    void PowerButtonPressed();
    void PowerButtonReleased();
    void INTArrowButtonClicked(QAbstractButton*);
    void StartSessionButtonClicked();
    void ResetBattery();

private slots:
    void SessionComplete(); // for session timer
    void PowerButtonHeld(); // for powerbutton timer
    void DepleteBattery(); // for battery timer

signals:
    void deviceUpdated();
};

#endif // DEVICE_H
