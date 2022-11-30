#ifndef DEVICE_H
#define DEVICE_H

#include <QObject>

enum State {Off, InSession, ChoosingSession, ChoosingSavedTherapy, Paused};

class Device : public QObject
{
    Q_OBJECT
public:
    explicit Device(QObject *parent = nullptr);

signals:

};

#endif // DEVICE_H
