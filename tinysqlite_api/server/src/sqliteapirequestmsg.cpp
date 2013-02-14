// Includes
#include "sqliteapirequestmsg.h"
#include "logging.h"

TinySqlApiRequestMsg::TinySqlApiRequestMsg(QObject *parent, QLocalSocket *socket) :
    QObject(parent), mClientConnection(socket)
{
    mId = -1;
    mState = NotConnected;
    connect(mClientConnection, SIGNAL(readyRead()), this, SLOT(handleRequest()));

    connect(mClientConnection, SIGNAL(disconnected()), this, SLOT(handleDisconnect()));
    connect(mClientConnection, SIGNAL(error(QLocalSocket::LocalSocketError)),
            this, SLOT(handleError(QLocalSocket::LocalSocketError)));
}

TinySqlApiRequestMsg::~TinySqlApiRequestMsg()
{
    DPRINT << "SQLITEAPISRV:~TinySqlApiRequestMsg";

    disconnect(mClientConnection, SIGNAL(readyRead()), this, SLOT(handleRequest()));

    disconnect(mClientConnection, SIGNAL(disconnected()), this, SLOT(handleDisconnect()));
    disconnect(mClientConnection, SIGNAL(error(QLocalSocket::LocalSocketError)),
            this, SLOT(handleError(QLocalSocket::LocalSocketError)));

    // We actually don't need to delete socket, system takes care of it, but it is recommended
    if( mClientConnection && mClientConnection->isOpen()){
        DPRINT << "SQLITEAPISRV:closing client socket..";
        mClientConnection->close();
        mClientConnection->deleteLater();        
        DPRINT << "SQLITEAPISRV:..closed";
    }
}

void TinySqlApiRequestMsg::handleRequest()
{
    DPRINT << "SQLITEAPISRV:*************";
    DPRINT << "SQLITEAPISRV:handleRequest";

    // Request contains items in following order: clientId, requestId, itemKey, message

    if( mClientConnection ) {
        DPRINT << "SQLITEAPISRV:Reading request data..";
        int bytesAvailable = mClientConnection->bytesAvailable();
        DPRINT << "SQLITEAPISRV:" << bytesAvailable << "bytes available";

        if( bytesAvailable > 0 ) {
            QDataStream in(mClientConnection);
            in.setVersion(int(QDataStream::Qt_4_0));

            in >> mId;

            int requestType;
            in >> requestType;
            mRequestType = static_cast<ServerRequestType>(requestType);

            in >> mItemKey;
            in >> mMessage;
            mState = DataRead;
            DPRINT << "SQLITEAPISRV:message read successfully";
        }
        else{
            DPRINT << "SQLITEAPISRV:ERR, no data in request socket!";
        }
		
        mClientConnection->write("ready");  // Write anything
    }
    else {
        DPRINT << "SQLITEAPISRV:ERR, client connection not found!";
    }    
}


void TinySqlApiRequestMsg::handleDisconnect()
{
    DPRINT << "SQLITEAPISRV:request socket disconnected. Client:" << mId;
    if( mState == DataRead ){
        mState = Disconnected;
    }
    else{
        mState = NotConnected;
    }
    DPRINT << "SQLITEAPISRV:client socket state now:" << mState;
    emit clientDisconnected(this);
}


void TinySqlApiRequestMsg::handleError(QLocalSocket::LocalSocketError socketError)
{
    switch (socketError) {
    case QLocalSocket::ServerNotFoundError:
        DPRINT << "SQLITEAPISRV:ERR, The host was not found";
        mState = Error;
        break;
    case QLocalSocket::ConnectionRefusedError:
        DPRINT << "SQLITEAPISRV:ERR, The connection was refused";
        mState = Error;
        break;
    case QLocalSocket::PeerClosedError: // This is OK case actually
        DPRINT << "SQLITEAPISRV:client closed the connection";
        if( mState == DataRead ){
            mState = Disconnected;
        }
        else{
            mState = NotConnected;
        }         
        break;
    default:
        mState = Error;
		if( mClientConnection ) {
	        DPRINT << "SQLITEAPISRV:ERR, error occurred:" << mClientConnection->errorString();
		}
    }
    emit clientDisconnected(this);
}
