// System includes

#include <QLocalSocket>
#include <QLocalServer>
#include <QDataStream>

// User includes
#include "sqliteapiserverdefs.h"
#include "tinysqliteapiclientnotifier.h"
#include "logging.h"

// ======== MEMBER FUNCTIONS ========

/*!
 * Construct new TinySqlApiClientNotifier
 */
TinySqlApiClientNotifier::TinySqlApiClientNotifier(QObject *parent, int clientId) :
    QLocalServer(parent), mClientId(clientId)
{
    mSocketNotify =  NULL;
}

//! Destructor
TinySqlApiClientNotifier::~TinySqlApiClientNotifier()
{
    DPRINT << "SQLITEAPICLI:~TinySqlApiClientNotifier";

    if( mSocketNotify && mSocketNotify->isOpen()) {
        mSocketNotify->close();
    }
    delete mSocketNotify;
    mSocketNotify = NULL;
}

/*! Starts listening notifications from the server.
 *  \return true on success, false on failure.
 */  
bool TinySqlApiClientNotifier::startListening()
{
    // Create unique client/server connection name
    QString socketServerName = TinySqlApiServerDefs::TinySqlApiClientSocketName;
    QString num;
    socketServerName += num.setNum(mClientId);
    DPRINT << "SQLITEAPICLI:client id" << mClientId << "Client notifier connection name:" << socketServerName;

    removeServer(socketServerName);
    
    if(!listen(socketServerName)) {
        DPRINT << "SQLITEAPICLI:ERR, ServerSocket listen() failed";
        DPRINT << "SQLITEAPICLI:Error:" << errorString();
        return false;
    }
    connect(this, SIGNAL(newConnection()), this, SLOT(handleNewConnection()));

    return true;
}

//! Slot for QLocalSocket::newConnection signal
void TinySqlApiClientNotifier::handleNewConnection()
{
    DPRINT << "SQLITEAPICLI:client id" << mClientId << "Server just connected to client notifier";

    delete mSocketNotify;
    mSocketNotify = NULL;
    mSocketNotify = nextPendingConnection();
    Q_ASSERT(mSocketNotify);
    DPRINT << "SQLITEAPICLI:client id" << mClientId << "connection for server notifications created";
    connect(mSocketNotify, SIGNAL(readyRead()), this, SLOT(handleServerResponse()));
    connect(mSocketNotify, SIGNAL(disconnected()), this, SLOT(handleDisconnect()));
}

//! Slot for QLocalSocket::readyRead signal
void TinySqlApiClientNotifier::handleServerResponse()
{
    DPRINT << "SQLITEAPICLI:client id" << mClientId << "handleServerResponse";
    if( !mSocketNotify ) {
        DPRINT << "SQLITEAPICLI:ERR, TinySqlApiServer::handleRequest, mSocketNotify is null";
#ifndef UNITTEST        
        Q_ASSERT(false);
#endif
        return;
    }
    int bytesAvailable = mSocketNotify->bytesAvailable();
    if( bytesAvailable > 0 ) {
        QDataStream stream;
        stream.setDevice( mSocketNotify );
        stream.setVersion(int(QDataStream::Qt_4_0));
        emit newDataReceived( stream );
    }
    else {
        DPRINT << "SQLITEAPICLI:ERR, Server sent empty response";
    }
}

//! Slot for QLocalSocket::disconnected signal
void TinySqlApiClientNotifier::handleDisconnect()
{
    DPRINT << "SQLITEAPICLI:ClientNotifier::handleDisconnect";
}

/*! Has to be called after the last received data is processed.
 *  Otherwise the next 'write' from server process would get missed.
 *  Server queues the responses and sends again until this is called.
 */
void TinySqlApiClientNotifier::confirmReadyToReceiveNext()
{
    if( !mSocketNotify ) {
        DPRINT << "SQLITEAPICLI:ERR, client id" << mClientId << "TinySqlApiServer::handleRequest, mSocketNotify is null";
#ifndef UNITTEST        
        Q_ASSERT(false);
#endif
        return;
    }

    mSocketNotify->write("ready");  // Write anything
}

// Signals

/*! This signal is emitted when client receives any data from the server. 
 *  \param stream - Contains reference to the new stream that uses the socket
 void newDataReceived(QDataStream &stream)
 */    
