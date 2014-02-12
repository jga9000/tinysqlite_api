// Includes
#include "sqliteapiresponsemsg.h"
#include "logging.h"

TinySqlApiResponseMsg::TinySqlApiResponseMsg(QObject *parent, ServerRequestType request, QSqlQuery &query, int id, const QVariant &itemKey ) :
    QObject(parent), mRequest(request), mSqlQuery(query), mId(id), mItemKey(itemKey)
{
    mCol = 0;

    if(mSqlQuery.lastError().isValid()) {
        mSqlError = mSqlQuery.lastError().type();
    }
    else {
        mSqlError = QSqlError::NoError;
    }
}

TinySqlApiResponseMsg::~TinySqlApiResponseMsg()
{
}

int TinySqlApiResponseMsg::startReading()
{
    if (!mSqlQuery.next()) {
        DPRINT << "SQLITEAPISRV:No data";
        return 0;
    }
    DPRINT << "SQLITEAPISRV:records:" << mSqlQuery.record().count();
    return mSqlQuery.record().count();
}

bool TinySqlApiResponseMsg::getNextValue( int &value )
{
    DPRINT << "SQLITEAPISRV:curr col:" << mCol;
    if( !nextCol() ) {
        return false;
    }
    DPRINT << "SQLITEAPISRV:Next value:" << mSqlQuery.value(mCol).toString();
    //DPRINT << "SQLITEAPISRV:Type:" << mSqlQuery.value(mCol).type();
    value = mSqlQuery.value(mCol++).toInt();
    return true;
}

bool TinySqlApiResponseMsg::getNextValue( QString &value )
{
    DPRINT << "SQLITEAPISRV:curr col:" << mCol;
    if( !nextCol() ) {
        return false;
    }
    DPRINT << "SQLITEAPISRV:Next value:" << mSqlQuery.value(mCol).toString();
    //DPRINT << "SQLITEAPISRV:Type:" << mSqlQuery.value(mCol).type();
    value = mSqlQuery.value(mCol++).toString();
    return true;
}

bool TinySqlApiResponseMsg::getNextValue( QVariant &value )
{
    DPRINT << "SQLITEAPISRV:curr col:" << mCol;
    if( !nextCol() ) {
        return false;
    }
    DPRINT << "SQLITEAPISRV:Next value:" << mSqlQuery.value(mCol).toString();
    //DPRINT << "SQLITEAPISRV:Type:" << mSqlQuery.value(mCol).type();

    value = mSqlQuery.value(mCol++);
    return true;
}

bool TinySqlApiResponseMsg::nextCol()
{
    if( mCol+1 > mSqlQuery.record().count() ) {
        DPRINT << "SQLITEAPISRV:setNextCol:next row";
        mCol = 0;
        if(!mSqlQuery.next()) {
            DPRINT << "SQLITEAPISRV:setNextCol:no more found";
            return false;
        }
    }
    return true;
}
