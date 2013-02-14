// Includes
#include <QProcess>
#include <QLocalSocket>
#include <QVariant>
#include <QTime>
#include <QCoreApplication>
#include <qwaitcondition.h>
#include <QMutex>
#include <limits.h>

#include "sqliteapiserverdefs.h"
#include "tinysqliteapidefs.h"
#include "tinysqliteapiclient.h"
#include "tinysqliteapiclientnotifier.h"
#include "tinysqliteapi.h"
#include "logging.h"


static QString toSqlVarType(const TinySqlApiInitializer &var)
{
    QString sqlVar;
    QString temp;

    switch(var.type()) {
    case QVariant::Bool:
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
    case QVariant::Double:
        sqlVar = QString("INTEGER");
        return sqlVar;

    case QVariant::String:
    case QVariant::Char:
    case QVariant::Date:
        sqlVar.append("VARCHAR(");
        sqlVar.append(temp.setNum(var.maxLength()));
        sqlVar.append(")");
        return sqlVar;

    case QVariant::ByteArray:
    case QVariant::BitArray:
        return "BLOB";

    case QVariant::Map:
    case QVariant::List:
    case QVariant::StringList:
    default:
        DPRINT << "SQLITEAPICLI:ERR, toSqlVarType: unhandled var type:" << var.type();
#ifndef EUNIT_ENABLED        
        Q_ASSERT(false);
#endif
        break;
    }
    return "";
}

TinySqlApi::TinySqlApi(QObject *parent) : QObject(parent)
{
    DPRINT << "SQLITEAPICLI:TinySqlApiClient" << TinySqlApiServerDefs::TinySqlApiVersion;
    
    // Wait some ms, otherwise client-ids could be same (if no time is passed meanwhile clients are created)
    QWaitCondition sleep;
    QMutex mutex;
    mutex.lock();
    sleep.wait(&mutex, 100);
    mutex.unlock();

    QTime time = QTime::currentTime();
    qsrand((uint)time.msec());

    mClientId = qrand() % INT_MAX;

    DPRINT << "SQLITEAPICLI:Unique client-id:" << mClientId;
    mRows = 0;

    mClientNotifier = new TinySqlApiClientNotifier(this, mClientId);
    Q_CHECK_PTR(mClientNotifier);
    connect(mClientNotifier, SIGNAL(newDataReceived(QDataStream &)), this, SLOT(handleNewData(QDataStream &)));

    // Creates the socket for this client for server to connect:    
    if(!mClientNotifier->startListening()) {
        DPRINT << "SQLITEAPICLI:ERR, Failed to initialize client notifier";
    }
    // Create the client which issues the requests, starts the server, 
    // server starts looking to connect this client:
    mClient = new TinySqlApiClient(this, mClientId);
    Q_CHECK_PTR(mClient);

    // If we created server, we are now registered already and
    // server will connect client's notifier/response handler

    // Register now only if server was already started before, checked by connecting
    QLocalSocket serverSocket;
    serverSocket.connectToServer(TinySqlApiServerDefs::TinySqlApiServerUniqueName);
    if(serverSocket.waitForConnected(1000)) {
        serverSocket.disconnectFromServer();
        if(serverSocket.waitForDisconnected(1000)) {
            DPRINT << "SQLITEAPICLI:ERR, client id" << mClientId << "Failed to disconnect";
        }
        DPRINT << "SQLITEAPICLI:Server already created, just registering";
        mClient->sendRequest(RegisterReq);

        // Before proceeding, wait until server connects to listener
        if( mClient->isWaitingForResponse() ) {
            DPRINT << "SQLITEAPICLI:client id" << mClientId << "waiting for server to respond to register request..";

            QTime waitTime = QTime::currentTime().addSecs(TinySqlApiServerAckTimeOutSecs);
            while( QTime::currentTime() < waitTime && mClient->isWaitingForResponse() ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
            }
            if( mClient->isWaitingForResponse() ) {
                DPRINT << "SQLITEAPICLI:ERR, client id" << mClientId << "FATAL, timeout in waiting for response to register!";
#ifndef EUNIT_ENABLED                
                Q_ASSERT(false);
#endif
            }
        }
    }
    else{
        DPRINT << "SQLITEAPICLI:Server not yet started";
        DPRINT << "SQLITEAPICLI:client id" << mClientId << "Calling SQLite API Server executable..";
        QString program = TinySqlApiServerDefs::TinySqlApiServerExeName;
        QStringList arguments;
    
        // Create unique client/server connection name
        QString argument;
        arguments += argument.setNum(mClientId);
        DPRINT << "SQLITEAPICLI:client id" << mClientId << "..waiting for server to start";
        QProcess::startDetached(program, arguments);

        // Before proceeding, wait until server connects to listener
        if( !mClientNotifier->isConnected() ) {
            DPRINT << "SQLITEAPICLI:client id" << mClientId << "waiting server to connect listener";

            QTime waitTime = QTime::currentTime().addSecs(TinySqlApiServerAckTimeOutSecs);
            while( QTime::currentTime() < waitTime && !mClientNotifier->isConnected() ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
            }
            if( !mClientNotifier->isConnected() ) {
                DPRINT << "SQLITEAPICLI:ERR, client id" << mClientId << "FATAL, timeout in waiting for server to connect!";
                DPRINT << "SQLITEAPICLI:server is either not started or we cannot register the client!";
#ifndef EUNIT_ENABLED                
                Q_ASSERT(false);
#endif
            }
        }
    }
}

TinySqlApi::~TinySqlApi()
{
    DPRINT << "SQLITEAPICLI:~TinySqlApi";

    delete mClient;
    mClient = NULL;

    delete mClientNotifier;
    mClientNotifier = NULL;
}

void TinySqlApi::initialize(const QString &table, const TinySqlApiInitializer &identifier, const QList<TinySqlApiInitializer> &initializers)
{
    mTable = table;
    mPrimaryKey = identifier.name();

    QString query;
    // append table name, primary key, SQL var type matching the Qt variable type
    query.append( QString("CREATE TABLE %1 (%2 %3").arg(mTable).arg(mPrimaryKey).arg(toSqlVarType(identifier)) );

    // In table changes, 'ON CONFLICT REPLACE' clause will automatically select between INSERT/UPDATE testing if primary key exists already
    query.append(" NOT NULL PRIMARY KEY ON CONFLICT REPLACE, ");

    int i = 0;

    // Initialize row count
    mRows = 1;  // Count the identifier (primary key) row too
    foreach (TinySqlApiInitializer initializer, initializers ) {
        mRows++;
        query.append(initializer.name());
        query.append(" ");
        query.append(toSqlVarType(initializer));
        i++;
        if(i<initializers.count()) {
            query.append(", ");
        }
    }
    query.append(")");
    mClient->sendRequest(CreateTableReq, query, "");
}

void TinySqlApi::read(const QVariant &identifier)
{
    QString query;
    query.append( QString("SELECT * FROM %1 WHERE %2 = '%3'").arg(mTable).arg(mPrimaryKey).arg(identifier.toString()) );
    
    mClient->sendRequest(ReadGenItemReq, query, "");
}

void TinySqlApi::count()
{
    QString query;
    query.append( QString("SELECT COUNT(%1) from %2").arg(mPrimaryKey).arg(mTable) );
    mClient->sendRequest(CountReq, query, "");
}

void TinySqlApi::readAll()
{
    QString query;
    query.append( QString("SELECT * FROM %1").arg(mTable) );
    mClient->sendRequest(ReadAllGenItemsReq, query, "");
}

void TinySqlApi::subscribeChangeNotifications( const QVariant &identifier)
{
    mClient->sendRequest(SubscribeNotificationsReq, "", identifier);
}

void TinySqlApi::unsubscribeChangeNotifications(const QVariant &identifier)
{
    mClient->sendRequest(UnsubscribeNotificationsReq, "", identifier);
}

/* This creates a single item */
void TinySqlApi::writeItem(QVariant &item)
{
    QString query;
    query.append( QString("INSERT INTO %1 VALUES (").arg(mTable) );

    int i = 0;
    QList<QVariant> itemList = item.toList();
    foreach (QVariant value, itemList) {
        query.append( QString("'%1'").arg(value.toString()) );
        i++;
        if(i<itemList.count()) {
            query.append(",");
        }
    }
    query.append(")");
    mClient->sendRequest(WriteGenItemReq, query, itemList[0].toString());
}

void TinySqlApi::cancelAsyncRequest()
{
    mClient->sendRequest(CancelLastReq);
}

void TinySqlApi::deleteItem(const QVariant &identifier)
{
    QString query;
    query.append( QString("DELETE FROM %1 WHERE %2 = '%3'").arg(mTable).arg(mPrimaryKey).arg(identifier.toString()) );
    mClient->sendRequest(DeleteReq, query, identifier);
}

void TinySqlApi::deleteAll()
{
    QString query;
    query.append( QString("DROP TABLE %1").arg(mTable) );
    mClient->sendRequest(DeleteAllReq, query, "");
}

void TinySqlApi::handleItemDataRes(QDataStream &stream)
{
    int status;
    stream >> status;
    DPRINT << "SQLITEAPICLI:Status_text:" << status;

    QList< QList<QVariant> > itemList;
    QList<QVariant> item;

    int count=0;
    while( !stream.atEnd() ) {
        while( count++<mRows && !stream.atEnd() ) {
            item << stream;
        }
        DPRINT << "SQLITEAPICLI:new gen item:" << item;            
        itemList << item;
        item.clear();
        count = 0;
    }
    if( itemList.count()==0 ) { 
        status = NotFoundError;
    }
    emit TinySqlApiRead( (TinySqlApiServerError)status, itemList );
}                

void TinySqlApi::handleCountRes(QDataStream &stream)
{
    int status;
    stream >> status;
    DPRINT << "SQLITEAPICLI:Status:" << status;

    int type = -1;  // Type var is found as first for QVariant
    stream >> type;
    int count;
    stream >> count;
    DPRINT << "SQLITEAPICLI:Count:" << count;

    emit TinySqlApiItemCount( (TinySqlApiServerError)status, count );
}

void TinySqlApi::handleNotification(QDataStream &stream, int response)
{
    QVariant itemKey;
    stream >> itemKey;
    if( response == int(UpdateNotification) ) {
        emit TinySqlApiUpdateNotification( itemKey );
    }
    else{
        emit TinySqlApiDeleteNotification( itemKey );
    }
}

void TinySqlApi::handleNewData(QDataStream &stream)
{
    DPRINT << "SQLITEAPICLI:handleNewData";

    int response;
    stream >> response;
    DPRINT << "SQLITEAPICLI:Response:" << response;

    int status;

    switch( response )
    {
    case InitializedRes:
        stream >> status;
        emit TinySqlApiServiceInitialized((TinySqlApiServerError)status);
        break;
        
    // Response to read(id), readAll (list of QVariant's list)
    case ItemDataRes:
        handleItemDataRes( stream );
        break;

    case CountRes:
        handleCountRes( stream );
        break;

    case WriteGenItemRes:
        stream >> status;
        DPRINT << "SQLITEAPICLI:WriteGenItemRes:" << status;
        emit TinySqlApiWrite( (TinySqlApiServerError)status );
        break;

    case DeleteRes:
        stream >> status;
        DPRINT << "SQLITEAPICLI:Delete:" << status;
        emit TinySqlApiDelete();
        break;

    case DeleteAllRes:
        stream >> status;
        DPRINT << "SQLITEAPICLI:DeleteAll:" << status;
        emit TinySqlApiDeleteAll();
        break;

    case UpdateNotification:
    case DeleteNotification:
        handleNotification( stream, response );
        break;

    case ConfirmationRes:
        break;

    default:
        //Error
        DPRINT << "SQLITEAPICLI:ERR, handleNewData:" << response << "is not valid case!";
        Q_ASSERT(false);
        break;
    }
    mClient->serverResponseReceived();
    mClientNotifier->confirmReadyToReceiveNext();
}
