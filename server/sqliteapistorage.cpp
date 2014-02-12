// Includes
#include "sqliteapistorage.h"
#include "sqliteapisql.h"
#include "logging.h"

TinySqlApiStorage::~TinySqlApiStorage()
{
    DPRINT << "SQLITEAPISRV:~TinySqlApiStorage..";

    disconnect(&mServer, SIGNAL(newRequest()), this, SLOT(handleRequest()));

    delete mSqlHandler;
    mSqlHandler = NULL;
}

TinySqlApiStorage::TinySqlApiStorage(QObject *parent, TinySqlApiServer &server)
 : QObject(parent), mServer(server)
{
    mSqlHandler = new TinySqlApiSql(this);
    Q_CHECK_PTR(mSqlHandler);

    connect(&mServer, SIGNAL(newRequest()), this, SLOT(handleRequest()));
}

bool TinySqlApiStorage::initialize(const QString& name)
{
    DPRINT << "SQLITEAPISRV:TinySqlApiStorage, initializing DB:" << name;
    return mSqlHandler->initialize(name);
}

// 
void TinySqlApiStorage::handleRequest()
{  
    DPRINT << "SQLITEAPISRV:TinySqlApiStorage::handleRequest()";
    TinySqlApiRequestMsg *request = mServer.getNextRequest();
    Q_ASSERT(request);

    // This method blocks the thread until finished
    TinySqlApiResponseMsg *response = mSqlHandler->sqlExecute( *request );
    delete request;
    
    Q_ASSERT(response);
    if( response ) {
        emit newResponse(response);
    }
}
