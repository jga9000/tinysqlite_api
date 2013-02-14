// Includes
#include "sqliteapiserverdefs.h"
#include "sqliteapiresponsehandler.h"
#include "sqliteapiserver.h"
#include "logging.h"

TinySqlApiResponseHandler::~TinySqlApiResponseHandler()
{
    DPRINT << "SQLITEAPISRV:~TinySqlApiResponseHandler";

    disconnect(this, SIGNAL(connected()), this, SLOT(notifierConnected()));
    disconnect(this, SIGNAL(bytesWritten(qint64)), this, SLOT(dataSent(qint64)));
    disconnect(this, SIGNAL(error(QLocalSocket::LocalSocketError)),
               this, SLOT(handleError(QLocalSocket::LocalSocketError)));
    disconnect(this, SIGNAL(readyRead()), this, SLOT(handleReceiveConfirmation()));

    disconnectFromServer();

#ifdef QT_DEBUG
    if( mResponseQueue.count() > 0 ) {
        DPRINT << "SQLITEAPISRV:ERR, there was" << mResponseQueue.count() << "unsent response(s)!";
#ifndef EUNIT_ENABLED
        Q_ASSERT(false);
#endif
    }
#endif    
}

TinySqlApiResponseHandler::TinySqlApiResponseHandler(QObject *parent, TinySqlApiServer &server, int clientId) :
    QLocalSocket(parent), mServer(server), mClientId(clientId)
{
    // This object is owned by TinySqlApiServer
    // When TinySqlApiServer destructs, it will delete this object
    mSending = false;
    mError = 0;
    mSocketServerName = TinySqlApiServerDefs::TinySqlApiClientSocketName;
    QString num;
    mSocketServerName += num.setNum(mClientId);

    connect(this, SIGNAL(connected()), this, SLOT(notifierConnected()));
    connect(this, SIGNAL(bytesWritten(qint64)), this, SLOT(dataSent(qint64)));
    connect(this, SIGNAL(error(QLocalSocket::LocalSocketError)),
            this, SLOT(handleError(QLocalSocket::LocalSocketError)));
    connect(this, SIGNAL(readyRead()), this, SLOT(handleReceiveConfirmation()));

    DPRINT << "SQLITEAPISRV:responsehandler: connecting to client notifier:" << mSocketServerName;
    connectToServer(mSocketServerName);
}

void TinySqlApiResponseHandler::sendData(const QByteArray &data)
{
    DPRINT << "SQLITEAPISRV:sendData, client:" << mClientId;
    QByteArray toBeSent;

    if(isFreeToSend())
    {
        if(mResponseQueue.count() > 0)
        {
            DPRINT << "SQLITEAPISRV:responsehandler send: reading from queue";
            mResponseQueue.append(data);	// Add new one to the tail
            toBeSent = mResponseQueue.dequeue();
        }
        else {
            toBeSent = data;
        }
    }
    else
    {
        DPRINT << "SQLITEAPISRV:responsehandler is busy, response queued";
        mResponseQueue.append(data);
        return;
    }

    DPRINT << "SQLITEAPISRV:Responsehandler, writing response..";
    mSending = true;

    if(isValid()) {
        write(toBeSent);        
    }
    else{
        DPRINT << "SQLITEAPISRV:ERR, Responsehandler client socket is no more valid (disconnected?)";
    }    
}

bool TinySqlApiResponseHandler::isFreeToSend() const
{
    // If we're sending!=free
    return !mSending;
}

void TinySqlApiResponseHandler::notifierConnected()
{
    mSending = false;
    DPRINT << "SQLITEAPISRV:responsehandler: notifier connected";
}

void TinySqlApiResponseHandler::dataSent(qint64 bytes)
{
    DPRINT << "SQLITEAPISRV:responsehandler: dataSent:" << bytes << "bytes. Client:" << mClientId;
}

void TinySqlApiResponseHandler::handleReceiveConfirmation()
{
    DPRINT << "SQLITEAPISRV:responsehandler: ACK received from client:" << mClientId;

    mSending = false;
    
    // After last response is sent, check if we have responses waiting in the queue:
    dequeueNextResponse();
}

void TinySqlApiResponseHandler::dequeueNextResponse()
{
    if(mResponseQueue.count() == 0) {
        DPRINT << "SQLITEAPISRV:responsehandler: queue empty";
    }
    else if( isFreeToSend() ) {
        DPRINT << mResponseQueue.count() << "item(s) in the queue. Client:" << mClientId;
        DPRINT << "SQLITEAPISRV:responsehandler: sending next from queue";
        QByteArray toBeSent = mResponseQueue.dequeue();

        mSending = true;

        if(isValid()) {
            write(toBeSent);        
        }
        else{
            DPRINT << "SQLITEAPISRV:ERR, Responsehandler client socket is no more valid (disconnected?)";
        }  
    }
    else{
    }
}

void TinySqlApiResponseHandler::handleError(QLocalSocket::LocalSocketError socketError)
{
    mSending = false;
    mError = int(socketError);

    switch (socketError) {
    case QLocalSocket::ServerNotFoundError:
        DPRINT << "SQLITEAPISRV:ERR, The host was not found";
        break;
    case QLocalSocket::ConnectionRefusedError:
        DPRINT << "SQLITEAPISRV:ERR, The connection was refused";
        break;
    case QLocalSocket::PeerClosedError: // Ok case
        DPRINT << "SQLITEAPISRV:The client notifier socket was closed";
        break;
    default:
        DPRINT << "SQLITEAPISRV:ERR, error occurred:" << errorString();
    }
    disconnectFromServer();
    mServer.removeClientId(mClientId);
}

bool TinySqlApiResponseHandler::isSubscribedFor(const QVariant& key)
{
    if( mSubscribedItemKeys.indexOf(key) >= 0 ) {
        return true;
    }
    return false;
}

void TinySqlApiResponseHandler::subscribeForNotifications( const QVariant &key )
{
    removeSubscription(key);
    mSubscribedItemKeys.append(key);
}

bool TinySqlApiResponseHandler::removeSubscription( const QVariant &key )
{
    int i=mSubscribedItemKeys.indexOf(key);
    if(i>=0) {
        mSubscribedItemKeys.removeAt(i);
        return true;
    }
    return false;
}
