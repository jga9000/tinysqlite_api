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

/*! Constructs new request object
 *
 * \param request Request id
 * \param msg Request message
 * \param itemKey Request primary key
 */
TinySqlApiClient::TinySqlApiServerRequest::TinySqlApiServerRequest(
    ServerRequestType request, 
    const QString &msg, 
    const QVariant &itemKey) :
        mRequest(request),
        mMsg(msg),
        mItemKey(itemKey) {
}

/*! Getter for the request constant id
 *
 * \return Request Id
 */
ServerRequestType TinySqlApiClient::TinySqlApiServerRequest::request() const {
    return mRequest;
}

/*! Getter for the request string
 *
 * \return String
 */
QString TinySqlApiClient::TinySqlApiServerRequest::msg() const {
    return mMsg;
}

/*! Getter for the primary key
 *
 * \return Key
 */
QVariant TinySqlApiClient::TinySqlApiServerRequest::itemKey() const {
    return mItemKey;
}

//! Destructor    
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

    sendRequest(UnregisterReq);
    disconnectFromServer();
}

/*!
 * Construct new TinySqlApiClient
 * Also starts the Sqlite API server process if not started yet.
 */
TinySqlApiClient::TinySqlApiClient(QObject *parent, int clientId) :
    QLocalSocket(parent), mClientId(clientId)
{
    mWaitingServerResponse = false;
    mRetries = 0;
}


/*! 
 *  Connects the localsocket to the Server socket
 *  Connects the signals to slot callbacks.
 */
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

/*! 
 *  Connects the server and sends the request message
 *  \param request The request code
 *  \param msg The request message
 *  \param itemKey Identifier for the item under change (primary key).
 *                 The change notification is based on this key.
 */
void TinySqlApiClient::sendRequest(ServerRequestType request, const QString &msg, const QVariant &itemKey)
{
    DPRINT << "SQLITEAPICLI:client id" << mClientId << "enqueued new request:" << msg;

    TinySqlApiServerRequest* serverRequest = new TinySqlApiServerRequest(request, msg, itemKey);
	Q_CHECK_PTR(serverRequest);	
    mRequestQueue.append( serverRequest );
    
    DPRINT << "SQLITEAPICLI:queue count now:" << mRequestQueue.count();
    
    connectServer();
}

/*! 
 *  Connects the server and sends the request message without any message
 *  \param request The request code
 */
void TinySqlApiClient::sendRequest(ServerRequestType request)
{
    sendRequest( request, "", "");
}

/*! 
 *  Connects the server and sends the next request message from the queue
 */    
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
    //DPRINT << "SQLITEAPICLI:Message:" << request->msg();

    DPRINT << "SQLITEAPICLI:client id" << mClientId << "sending";
    write(block);
    delete request;
}

//! Slot for QLocalSocket::connected signal
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
#ifndef UNITTEST        
        Q_ASSERT(false);
#endif
        mWaitingServerResponse = false;
        disconnectFromServer();
    }
}

//! Slot for QLocalSocket::disconnected signal  
void TinySqlApiClient::handleDisconnected()
{
    DPRINT << "SQLITEAPICLI:client id" << mClientId << "handleDisconnected";
    mConnected = false;
    if( mRequestQueue.count() > 0 && !mWaitingServerResponse) {
       connectServer();
    }
}

/*!
 *  Server sent response for the last request. Next request can be sent.
 */
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

//! Slot for QLocalSocket::bytesWritten signal
void TinySqlApiClient::dataSent(qint64 bytes)
{
    DPRINT << "SQLITEAPICLI:client:" << mClientId << "dataSent:" << bytes << "bytes";
}

//! Slot for QLocalSocket::readyRead signal    
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

//! Slot for QLocalSocket::error signal   
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
#ifndef UNITTEST            
            Q_ASSERT(false);
#endif
        }
        break;
    case QLocalSocket::ConnectionRefusedError:
        DPRINT << "SQLITEAPICLI:ERR, The connection was refused";
#ifndef UNITTEST        
        Q_ASSERT(false);
#endif
        break;
    case QLocalSocket::PeerClosedError: // This is OK case actually
        DPRINT << "SQLITEAPICLI:The peer was closed";      
        break;
    default:
        DPRINT << "SQLITEAPICLI:ERR, error occurred:" << errorString();
#ifndef UNITTEST        
        Q_ASSERT(false);
#endif
        break;
    }
}
