// Includes
#include "sqliteapiserverdefs.h"
#include "sqliteapirequesthandler.h"
#include "sqliteapirequestmsg.h"
#include "qalgorithms.h"
#include "logging.h"

TinySqlApiRequestHandler::~TinySqlApiRequestHandler()
{
    DPRINT << "SQLITEAPISRV:~TinySqlApiRequestHandler IN";

    disconnect(this, SIGNAL(newConnection()), this, SLOT(handleNewConnection()));
    
    close();

    // Just in case check if there are client connection active
    QLocalSocket *clientConnection = nextPendingConnection();
    while( clientConnection ) {
        DPRINT << "SQLITEAPISRV:disconnecting client..";
        clientConnection->disconnectFromServer();
        delete clientConnection;
        clientConnection = NULL;
        clientConnection = nextPendingConnection();
    }
    
    DPRINT << "SQLITEAPISRV:~TinySqlApiRequestHandler count:" << mMsgs.count();
    qDeleteAll(mMsgs);
    DPRINT << "SQLITEAPISRV:~TinySqlApiRequestHandler OUT";
}

TinySqlApiRequestHandler::TinySqlApiRequestHandler(QObject *parent) :
    QLocalServer(parent)
{
    connect(this, SIGNAL(newConnection()), this, SLOT(handleNewConnection()));
}

bool TinySqlApiRequestHandler::initialize()
{
    removeServer(TinySqlApiServerDefs::TinySqlApiServerUniqueName);
    
    if(!listen(TinySqlApiServerDefs::TinySqlApiServerUniqueName)) {
        DPRINT << "SQLITEAPISRV:ERR, RequestHandler listen() failed";
        DPRINT << "SQLITEAPISRV:Error:" << errorString();
    }
    return true;
}

void TinySqlApiRequestHandler::handleNewConnection()
{
    DPRINT << "SQLITEAPISRV:handleNewConnection";

    QLocalSocket* clientConnection = nextPendingConnection();
    Q_CHECK_PTR(clientConnection);
    TinySqlApiRequestMsg *msg = new TinySqlApiRequestMsg(0, clientConnection);
    DPRINT << "TinySqlApiRequestHandler::handleNewConnection msg" << msg;
    Q_CHECK_PTR(msg);
    mMsgs.append(msg);
    
    connect(msg, SIGNAL(clientDisconnected(TinySqlApiRequestMsg *)), this, SLOT(handleDisconnect(TinySqlApiRequestMsg *)));
}

void TinySqlApiRequestHandler::handleDisconnect(TinySqlApiRequestMsg *msg)
{
    // Client has sent the data successfully and disconnected
    DPRINT << "SQLITEAPISRV:RequestHandler::handleDisconnect";
	Q_CHECK_PTR( msg );

	disconnect(msg, SIGNAL(clientDisconnected(TinySqlApiRequestMsg *)), this, SLOT(handleDisconnect(TinySqlApiRequestMsg *)));
	
    // This is the end of the single request&receive sequence
    // emit signal to the server object
    DPRINT << "SQLITEAPISRV:Client-id:" << msg->id();
    DPRINT << "SQLITEAPISRV:Request code:" << msg->type();
    DPRINT << "SQLITEAPISRV:Item key:" << msg->itemKey();
    DPRINT << "SQLITEAPISRV:Message:" << msg->request();

    int index = mMsgs.indexOf(msg);
    if( msg->state() == TinySqlApiRequestMsg::DataRead || msg->state() == TinySqlApiRequestMsg::Disconnected ) {
        emit newRequest(msg);
        if(index != -1){
            mMsgs.removeAt(index);
        }
    }
    else{
        DPRINT << "SQLITEAPISRV:ERR, RequestHandler: not valid request, client socket state:" << msg->state();
        if(index == -1 || msg->id() == -1){
            // TBD: this is only for one-client-server apps:
            // No clients exists anymore, close server
            emit abnormalDisconnection();
        }
        else{
            delete mMsgs.takeAt(index);
        }
    }
	DPRINT << "SQLITEAPISRV:-------------"; // End of request processing	
}
