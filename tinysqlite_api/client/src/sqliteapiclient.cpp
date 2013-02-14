// Includes
#include <QProcess>
#include <QLocalSocket>
#include <QLocalServer>
#include <QTimerEvent>
#include <QVariant>
#include <QTime>
#include <QCoreApplication>

#include "sqliteapiserverdefs.h"
#include "tinysqliteapiclient.h"
#include "logging.h"

//! Number of retries if connection fails
const unsigned int TinySqlApiServerConnectRetries = 3;
    
TinySqlApiClient::~TinySqlApiClient()
{
    DPRINT << "SQLITEAPICLI:client id" << mClientId << "~TinySqlApiClient";

#ifdef QT_DEBUG
    if( mRequestQueue.count() > 0 ) {
        DPRINT << "SQLITEAPICLI:ERR, there was" << mRequestQueue.count() << "unsent request(s) in the queue!";

        for( int i=0; i<mRequestQueue.count(); i++ ) {
            DPRINT << "SQLITEAPICLI:request:" << mRequestQueue.at(i)->msg();
        }
    }
#endif
    qDeleteAll(mRequestQueue.begin(), mRequestQueue.end());
    mRequestQueue.clear();
    
    disconnectFromServer();
}

TinySqlApiClient::TinySqlApiClient(QObject *parent, int clientId) :
    QLocalSocket(parent), mClientId(clientId)
{
    mWaitingServerResponse = false;
    mRetries = 0;
}

void TinySqlApiClient::connectServer()
{
    if( mWaitingServerResponse ) {
        // We are already connected, server is processing previous request
        DPRINT << "SQLITEAPICLI:client id" << mClientId << "busy waiting server response, new request just queued";
        return;
    }

    mRetries = 0;
    mWaitingServerResponse = true;
    mConnected = false;
    
    disconnect(this, SIGNAL(connected()), this, SLOT(handleConnected()));	
    disconnect(this, SIGNAL(disconnected()), this, SLOT(handleDisconnected()));
    disconnect(this, SIGNAL(error(QLocalSocket::LocalSocketError)),
               this, SLOT(handleError(QLocalSocket::LocalSocketError)));
    disconnect(this, SIGNAL(bytesWritten(qint64)), this, SLOT(dataSent(qint64)));
    disconnect(this, SIGNAL(readyRead()), this, SLOT(handleReceiveConfirmation()));
	
    connect(this, SIGNAL(connected()), this, SLOT(handleConnected()));
    connect(this, SIGNAL(disconnected()), this, SLOT(handleDisconnected()));
    connect(this, SIGNAL(error(QLocalSocket::LocalSocketError)),
            this, SLOT(handleError(QLocalSocket::LocalSocketError)));
    connect(this, SIGNAL(bytesWritten(qint64)), this, SLOT(dataSent(qint64)));
    connect(this, SIGNAL(readyRead()), this, SLOT(handleReceiveConfirmation()));

    DPRINT << "SQLITEAPICLI:client id" << mClientId << "connecting to server";

    connectToServer(TinySqlApiServerDefs::TinySqlApiServerUniqueName);
}

void TinySqlApiClient::sendRequest(ServerRequestType request, const QString &msg, const QVariant &itemKey)
{
    DPRINT << "SQLITEAPICLI:client id" << mClientId << "enqueued new request:" << msg;

    TinySqlApiServerRequest* serverRequest = new TinySqlApiServerRequest(request, msg, itemKey);
	Q_CHECK_PTR(serverRequest);	
    mRequestQueue.append( serverRequest );
    
    DPRINT << "SQLITEAPICLI:queue count now:" << mRequestQueue.count();
    
    connectServer();
}

void TinySqlApiClient::sendRequest(ServerRequestType request)
{
    sendRequest( request, "", "");
}

void TinySqlApiClient::sendNextRequest()
{
    DPRINT << "SQLITEAPICLI:client id" << mClientId << "dequeue & sending next request";
    TinySqlApiServerRequest *request = mRequestQueue.dequeue();
	Q_CHECK_PTR(request);

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);

    out.setVersion(int(QDataStream::Qt_4_0));    // Qt_4_0 seems required when Qt version is 4.x
    mRetries = 0;

    DPRINT << "SQLITEAPICLI:client id" << mClientId << "New request:" << int(request->request());
    out << mClientId;
    out << int(request->request());

    out << request->itemKey();
    DPRINT << "SQLITEAPICLI:Item key:" << request->itemKey();
    out << request->msg();
    DPRINT << "SQLITEAPICLI:Message:" << request->msg();

    DPRINT << "SQLITEAPICLI:client id" << mClientId << "sending";
    write(block);
    delete request;
}

void TinySqlApiClient::handleConnected()
{
    DPRINT << "SQLITEAPICLI:client id" << mClientId << "handleConnected";
    mRetries = 0;
    mConnected = true;
    // Send the queued request
    if( mRequestQueue.count() > 0 ) {
        sendNextRequest();
    }
    else{
        DPRINT << "SQLITEAPICLI:ERR, client id" << mClientId << "connected, but no request found!";
#ifndef EUNIT_ENABLED        
        Q_ASSERT(false);
#endif
        mWaitingServerResponse = false;
        disconnectFromServer();
    }
}

void TinySqlApiClient::handleDisconnected()
{
    DPRINT << "SQLITEAPICLI:client id" << mClientId << "handleDisconnected";
    mConnected = false;
    if( mRequestQueue.count() > 0 && !mWaitingServerResponse) {
       connectServer();
    }
}

void TinySqlApiClient::serverResponseReceived()
{
    DPRINT << "SQLITEAPICLI:client id" << mClientId << "serverResponseReceived";
    mWaitingServerResponse = false;

    // Check if we have received new requests in the queue while previous request
    // has been under process. If so, connect then send next request from queue
    if( mRequestQueue.count() > 0 ) {
        if( mConnected ) {
            DPRINT << "SQLITEAPICLI:client id" << mClientId << "sending next from queue";
            mWaitingServerResponse = true;
            sendNextRequest();
        }
        else{
            connectServer();
        }
    }
    else{
        if( mConnected ) {
            // Free socket for other clients use
            DPRINT << "SQLITEAPICLI:client id" << mClientId << "disconnecting server";
            mConnected = false;
            disconnectFromServer();
        }
    }
}

void TinySqlApiClient::dataSent(qint64 bytes)
{
    DPRINT << "SQLITEAPICLI:client:" << mClientId << "dataSent:" << bytes << "bytes";
}


void TinySqlApiClient::handleReceiveConfirmation()
{
    DPRINT << "SQLITEAPICLI:client id" << mClientId << "ACK for request received";
    // Here we can disconnect, server has received our request and ACKed
    // (in this way socket is free for other clients use)

    if( mConnected ) {
        // Free socket for other clients use
        DPRINT << "SQLITEAPICLI:client id" << mClientId << "disconnecting server";
        mConnected = false;
        disconnectFromServer();
    }    
}

void TinySqlApiClient::handleError(QLocalSocket::LocalSocketError socketError)
{
    mWaitingServerResponse = false;
    mConnected = false;

    switch (socketError) {
    case QLocalSocket::ServerNotFoundError:
        DPRINT << "SQLITEAPICLI:ERR, The host was not found";
        if(mRetries++<TinySqlApiServerConnectRetries) {
            DPRINT << "SQLITEAPICLI:Retrying connect..";
            connectToServer(TinySqlApiServerDefs::TinySqlApiServerUniqueName);
        }
        else{
            DPRINT << "SQLITEAPICLI:ERR, Sqlite API connect retry failed" ;
#ifndef EUNIT_ENABLED            
            Q_ASSERT(false);
#endif
        }
        break;
    case QLocalSocket::ConnectionRefusedError:
        DPRINT << "SQLITEAPICLI:ERR, The connection was refused";
#ifndef EUNIT_ENABLED        
        Q_ASSERT(false);
#endif
        break;
    case QLocalSocket::PeerClosedError: // This is OK case actually
        DPRINT << "SQLITEAPICLI:The peer was closed";      
        break;
    default:
        DPRINT << "SQLITEAPICLI:ERR, error occurred:" << errorString();
#ifndef EUNIT_ENABLED        
        Q_ASSERT(false);
#endif
        break;
    }
}
