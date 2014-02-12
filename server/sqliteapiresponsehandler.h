#ifndef SQLITEAPIRESPONSEHANDLER_H_
#define SQLITEAPIRESPONSEHANDLER_H_

#include <QLocalSocket>
#include "sqliteapiserver.h"

class TinySqlApiResponseHandler : public QLocalSocket
{
    Q_OBJECT

public:
    //! Constructs new TinySqlApiResponseHandler
    explicit TinySqlApiResponseHandler(QObject *parent, TinySqlApiServer &server, int clientId);

    //! Destructor
    virtual ~TinySqlApiResponseHandler();

public:
    void sendData(const QByteArray &data);
    void enqueueData(const QByteArray &data);
    inline int lastError() const { return mError; }
    inline int clientId() const { return mClientId; }
    inline int unsentResponseCount() const { return mResponseQueue.count(); }

    bool isSubscribedFor( const QVariant& key );
    void subscribeForNotifications( const QVariant& key );
    bool removeSubscription( const QVariant& key );
    void dequeueNextResponse();
    bool isFreeToSend() const;

private slots:
    void notifierConnected();    
    void handleError(QLocalSocket::LocalSocketError socketError);
    void handleReceiveConfirmation();
    void dataSent(qint64);

private:
    QQueue<QByteArray> mResponseQueue;

    TinySqlApiServer &mServer;

    // List of primary keys/ids which the receiving client
    // has subscribed for
    QList<QVariant> mSubscribedItemKeys;

    int mClientId;
    QString mSocketServerName;

    bool mSending;
    int mError;

    #ifdef UNITTEST
        friend class UT_TinySqlApiResponseHandler;
        friend class UT_TinySqlApiServer;        
    #endif
};

#endif /* SQLITEAPIRESPONSEHANDLER_H_ */
