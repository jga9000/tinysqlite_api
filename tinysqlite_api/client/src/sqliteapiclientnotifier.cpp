// System includes

#include "Qlocalsocket.h"
#include "Qlocalserver.h"

// User includes
#include "sqliteapiserverdefs.h"
#include "tinysqliteapiclientnotifier.h"
#include "logging.h"

// ======== MEMBER FUNCTIONS ========

//! Starts listening notifications from the server.
TinySqlApiClientNotifier::~TinySqlApiClientNotifier()
{
    DPRINT << "SQLITEAPICLI:~TinySqlApiClientNotifier";

    if( mSocketNotify && mSocketNotify->isOpen()) {
        mSocketNotify->close();
    }
    delete mSocketNotify;
    mSocketNotify = NULL;
}

TinySqlApiClientNotifier::TinySqlApiClientNotifier(QObject *parent, int clientId) :
    QLocalServer(parent), mClientId(clientId)
{
    mSocketNotify =  NULL;
}

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

void TinySqlApiClientNotifier::handleServerResponse()
{
    DPRINT << "SQLITEAPICLI:client id" << mClientId << "handleServerResponse";
    if( !mSocketNotify ) {
        DPRINT << "SQLITEAPICLI:ERR, TinySqlApiServer::handleRequest, mSocketNotify is null";
#ifndef EUNIT_ENABLED        
        Q_ASSERT(false);
#endif
        return;
    }
    int bytesAvailable = mSocketNotify->bytesAvailable();
    if( bytesAvailable > 0 ) {
        QDataStream stream(mSocketNotify);
        stream.setVersion(int(QDataStream::Qt_4_0));
        emit newDataReceived( stream );
    }
    else {
        DPRINT << "SQLITEAPICLI:ERR, Server sent empty response";
    }
}

void TinySqlApiClientNotifier::handleDisconnect()
{
    DPRINT << "SQLITEAPICLI:ClientNotifier::handleDisconnect";
}

void TinySqlApiClientNotifier::confirmReadyToReceiveNext()
{
    if( !mSocketNotify ) {
        DPRINT << "SQLITEAPICLI:ERR, client id" << mClientId << "TinySqlApiServer::handleRequest, mSocketNotify is null";
#ifndef EUNIT_ENABLED        
        Q_ASSERT(false);
#endif
        return;
    }

    mSocketNotify->write("ready");  // Write anything
}
