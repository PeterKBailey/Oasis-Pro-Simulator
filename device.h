#ifndef DEVICE_H
#define DEVICE_H

#include <QObject>
#include <QString>
#include <QAbstractButton>
#include <QTimer>
#include <QDebug>
#include <QVector>

#include "defs.h"

enum State {Off, ChoosingSession, ChoosingSavedTherapy, InSession, Paused, TestingConnection};
enum BatteryState { High, Low, CriticallyLow };

class Device : public QObject
{
    Q_OBJECT
public:
    explicit Device(QObject *parent = nullptr);
    ~Device();

    // getters
    State getState() const;
    BatteryState getBatteryState() const;
    double getBatteryLevel() const;

private:
    State state;
    BatteryState batteryState;

    QTimer sessionTimer;
    int remainingSessionTime; // time, ms

    QTimer powerButtonTimer;

    QTimer batteryLevelTimer;
    double batteryLevel;

    int intensity;

    // this is peter guessing at how this will work
    // highlighted / currently selected
    int selectedSessionGroup; // (time) 0, 1, 2
    int selectedSessionType; // (frequency) 0, 1, 2, 3

    // stored data of the groups and sessions we have, set up in ctor
    QVector<SessionGroup*> sessionGroups;
    QVector<SessionType*> sessionTypes;

    void powerOn();
    void powerOff(bool);
    void pauseSession();
    void resumeSession();
    void enterTestMode();
    void configureDevice();

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
