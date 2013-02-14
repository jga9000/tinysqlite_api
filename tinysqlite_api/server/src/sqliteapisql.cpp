// Includes
#include "sqliteapiresponsemsg.h"
#include "sqliteapisql.h"
#include "logging.h"

TinySqlApiSql::~TinySqlApiSql()
{
    DPRINT << "SQLITEAPISRV:~TinySqlApiSql";
    mDb.close();
}

TinySqlApiSql::TinySqlApiSql(QObject *parent) :
    QObject(parent)
{
}

bool TinySqlApiSql::initialize()
{
    mDb = QSqlDatabase::addDatabase( "QSQLITE");
    if(!mDb.isValid()){
        DPRINT << "SQLITEAPISRV:ERR, QSqlDatabase is not valid, error:" << mDb.lastError().text();
        return false;
    }
    mDb.setDatabaseName("sqliteapidb.db");

    if(!mDb.open()){
        DPRINT << "SQLITEAPISRV:ERR, Unable to connect to DB, error:" << mDb.lastError().text();
        return false;
    }
    return true;
}

TinySqlApiResponseMsg *TinySqlApiSql::sqlExecute(TinySqlApiRequestMsg& msg)
{
    QString sqlQuery = msg.request();
    QSqlQuery query( mDb );
    DPRINT << "SQLITEAPISRV:Executing SQL query..";
    
    // Note QSqlQuery::exec() executes synchronously, blocks the whole process
    bool ret = query.exec( sqlQuery );
    DPRINT << "SQLITEAPISRV:..done. Status:" << ret;
    // Instead of ret value, we check lastError()
    
    if( query.lastError().number() > 0) {
        DPRINT << "SQLITEAPISRV:SQL error validity :" << query.lastError().isValid();
        DPRINT << "SQLITEAPISRV:SQL error text:" << query.lastError().text();
        DPRINT << "SQLITEAPISRV:SQL error type:" << int(query.lastError().type());
    }
    TinySqlApiResponseMsg *responsemsg = new TinySqlApiResponseMsg(this, msg.type(), query, msg.id(), msg.itemKey() );
    Q_CHECK_PTR(responsemsg);
    return responsemsg;
}
