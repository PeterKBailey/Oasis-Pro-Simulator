#ifndef DEFS_H
#define DEFS_H

#include <QString>
#include <QVector>



struct SessionGroup {
    QString name;
    int durationMins;
    SessionGroup(QString n, int d) : name(n), durationMins(d) {}
};

struct SessionType {
    QString name;
    QString wavelength;
    SessionType(QString n, QString w) : name(n), wavelength(w) {}
};

struct Therapy {
    SessionGroup group;
    SessionType type;
    int intensity;
    QString username;
    Therapy(SessionGroup g, SessionType t, int i, QString u) : group(g), type(t), intensity(i), username(u) {}
};

struct UserDesignedSession {
    QString name;
    int durationMins;
    QVector<SessionType> types;
    UserDesignedSession(QString n, int d, QVector<SessionType> t) : name(n), durationMins(d), types(t){}
};

#endif // DEFS_H
