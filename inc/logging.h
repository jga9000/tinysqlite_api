#ifndef SQLITEAPILOGGING_H
#define SQLITEAPILOGGING_H

#include <QDebug>
#include <QTime>

//#define DPRINT qDebug() << QTime::currentTime() << __PRETTY_FUNCTION__
#define DPRINT qDebug() << QTime::currentTime().toString("ss.zzz")

#endif // SQLITEAPILOGGING_H
