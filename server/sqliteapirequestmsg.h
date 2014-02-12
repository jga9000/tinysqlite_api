#ifndef SQLITEAPIREQUESTMSG_H_
#define SQLITEAPIREQUESTMSG_H_

#include <QObject>
#include <QVariant>
#include <QtNetwork/QLocalSocket>
#include "tinysqliteapidefs.h"

class TinySqlApiRequestMsg : public QObject
{
    Q_OBJECT

public:
    
    enum ClientSocketState{
        NotConnected,
        Connected,
        DataRead,
        Disconnected,
        Error
    };    
    
    //! Construct new TinySqlApiRequestMsg    
    explicit TinySqlApiRequestMsg(QObject *parent, QLocalSocket *socket);

    //! Destructor
    virtual ~TinySqlApiRequestMsg();

public:
    inline QString request() const { return mMessage; }
    inline QVariant itemKey() const { return mItemKey; }
    inline int id() const { return mId; }
    inline ServerRequestType type() const { return mRequestType; }
    inline ClientSocketState state() const { return mState; }

signals:
    void clientDisconnected(TinySqlApiRequestMsg* msg);

private slots:
    void handleRequest();
    void handleDisconnect();
    void handleError(QLocalSocket::LocalSocketError socketError);

private:
    QLocalSocket *mClientConnection;
    ServerRequestType mRequestType;
    QString mMessage;
    int mId;
    QVariant mItemKey;
    ClientSocketState mState;
    
#ifdef UNITTEST
    friend class UT_TinySqlApiRequestMsg;
    friend class UT_TinySqlApiServer;    
#endif    
};

#endif /* SQLITEAPIREQUESTMSG_H_ */
