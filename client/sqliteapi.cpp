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
#ifndef UNITTEST        
        Q_ASSERT(false);
#endif
        break;
    }
    return "";
}

/*! Constructs new initializer
 *
 * \param type Qt variable type for the storage item
 * \param name Identifying name for the storage item
 * \param maxLength How much data is reserved for the item in the storage
 */
TinySqlApiInitializer::TinySqlApiInitializer(
    QVariant::Type type, 
    const QString &name, 
    int maxLength) :
        QVariant(type),
        _name(name),
        _maxLength(maxLength) {
}

/*! Getter for the name
 *
 * \return The given name for the item
 */
QString TinySqlApiInitializer::name() const {
    return _name;
}

/*! Getter for the given maximum size
 *
 * \return Maximum length for the item
 */
int TinySqlApiInitializer::maxLength() const {
    return _maxLength;
}

//! Constructs new TinySqlApi object
/* \param table Name of the table, will be used in further API calls.
 */
TinySqlApi::TinySqlApi(const QString &table, QObject *parent) : QObject(parent)
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

    tableName = table;

    //mClientId = qrand() % INT_MAX;
    clientId = 1234;

    DPRINT << "SQLITEAPICLI:Unique client-id:" << clientId;
    columns = 0;

    clientNotifier = new TinySqlApiClientNotifier(this, clientId);
    Q_CHECK_PTR(clientNotifier);
    connect(clientNotifier, SIGNAL(newDataReceived(QDataStream &)), this, SLOT(handleNewData(QDataStream &)));

    // Creates the socket for this client for server to connect:    
    if (!clientNotifier->startListening()) {
        DPRINT << "SQLITEAPICLI:ERR, Failed to initialize client notifier";
    }
    // Create the client which issues the requests, starts the server, 
    // server starts looking to connect this client:
    client = new TinySqlApiClient(this, clientId);
    Q_CHECK_PTR(client);

    // If we created server, we are now registered already and
    // server will connect client's notifier/response handler

    // Register now only if server was already started before, checked by connecting
    QLocalSocket serverSocket;
    serverSocket.connectToServer(TinySqlApiServerDefs::TinySqlApiServerUniqueName);
    if (serverSocket.waitForConnected(1000)) {
        serverSocket.disconnectFromServer();
        if (serverSocket.waitForDisconnected(1000)) {
            DPRINT << "SQLITEAPICLI:ERR, client id" << clientId << "Failed to disconnect";
        }
        DPRINT << "SQLITEAPICLI:Server already created, just registering";
        client->sendRequest(RegisterReq);

        // Before proceeding, wait until server connects to listener
        if (client->isWaitingForResponse()) {
            DPRINT << "SQLITEAPICLI:client id" << clientId << "waiting for server to respond to register request..";

            QTime waitTime = QTime::currentTime().addSecs(TinySqlApiServerAckTimeOutSecs);
            while( QTime::currentTime() < waitTime && client->isWaitingForResponse() ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
            }
            if (client->isWaitingForResponse()) {
                DPRINT << "SQLITEAPICLI:ERR, client id" << clientId << "FATAL, timeout in waiting for response to register!";
#ifndef UNITTEST                
                Q_ASSERT(false);
#endif
            }
        }
    }
    else{
        DPRINT << "SQLITEAPICLI:Server not yet started";
        DPRINT << "SQLITEAPICLI:client id" << clientId << "Calling SQLite API Server executable..";
        QString program = TinySqlApiServerDefs::TinySqlApiServerExeName;
        QStringList arguments;
    
        // Create unique client/server connection name
        QString argument;
        arguments += argument.setNum(clientId);
        DPRINT << "SQLITEAPICLI:client id" << clientId << "..waiting for server to start";
        QProcess::startDetached(program, arguments);

        // Before proceeding, wait until server connects to listener
        if (!clientNotifier->isConnected()) {
            DPRINT << "SQLITEAPICLI:client id" << clientId << "waiting server to connect listener";

            QTime waitTime = QTime::currentTime().addSecs(TinySqlApiServerAckTimeOutSecs);
            while( QTime::currentTime() < waitTime && !clientNotifier->isConnected() ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
            }
            if (!clientNotifier->isConnected()) {
                DPRINT << "SQLITEAPICLI:ERR, client id" << clientId << "FATAL, timeout in waiting for server to connect!";
                DPRINT << "SQLITEAPICLI:server is either not started or we cannot register the client!";
#ifndef UNITTEST                
                Q_ASSERT(false);
#endif
            }
        }
    }
}

//! Destructor
TinySqlApi::~TinySqlApi()
{
    DPRINT << "SQLITEAPICLI:~TinySqlApi";

    delete client;
    client = NULL;

    delete clientNotifier;
    clientNotifier = NULL;
}

/*! Initializes Sqlite API client with detailed storage table initializers.
 * emits tinySqlApiServiceInitialized signal.
 *
 * \param identifier Defines the type of the identifier (primary key)
 * \param initializers List of initializers defining the type of the stored items
 *
 *    Example: QList<QTinySqlApiInitializer> initializers;
 *    QTinySqlApiInitializer identifier( QVariant::String, "service", 255);
 *    QTinySqlApiInitializer var1(QVariant::Int, "count", 10);
 *    initializers.append(var1);
 *    client->initialize("invitations", identifier, initializers);
 */
void TinySqlApi::initialize(const TinySqlApiInitializer &identifier, const QList<TinySqlApiInitializer> &initializers)
{
    primaryKey = identifier.name();

    DPRINT << "SQLITEAPICLI:new mPrimaryKey:" << primaryKey;

    QString query;
    // append table name, primary key, SQL var type matching the Qt variable type
    query.append( QString("CREATE TABLE %1 (%2 %3").arg(tableName).arg(primaryKey).arg(toSqlVarType(identifier)) );

    // In table changes, 'ON CONFLICT REPLACE' clause will automatically select between INSERT/UPDATE testing if primary key exists already
    query.append(" NOT NULL PRIMARY KEY ON CONFLICT REPLACE, ");

    int i = 0;

    // Initialize schema
    columns = 1;  // Count the identifier (primary key) row too
    foreach (TinySqlApiInitializer initializer, initializers) {
        columns++;
        query.append(initializer.name());
        DPRINT << "SQLITEAPICLI:new initializer:" << initializer.name();
        query.append(" ");
        query.append(toSqlVarType(initializer));
        i++;
        if (i<initializers.count()) {
            query.append(", ");
        }
    }
    DPRINT << "columns:" << columns;
    query.append(")");
    client->sendRequest(CreateTableReq, query, "");
}

/*!
* Reads items from table for a given identifier. Asynchronous method, 
* emits tinySqlApiRead signal.
*
* \param identifier is the item in the table (id)
*/
void TinySqlApi::read(const QVariant &identifier)
{
    QString query;
    query.append(QString("SELECT * FROM %1 WHERE %2 = '%3'").arg(tableName).arg(primaryKey).arg(identifier.toString()) );
    
    client->sendRequest(ReadGenItemReq, query, "");
}

/*!
 * Request total count of rows.
 * Asynchronous method, emits tinySqlApiCount signal.
 */
void TinySqlApi::count()
{
    QString query;
    query.append(QString("SELECT COUNT(*) AS NumberOfOrders FROM %1").arg(tableName));
    client->sendRequest(CountReq, query, "");
}

/*!
 * Request list of all tables from DB.
 * Asynchronous method, emits tinySqlApiTablesRes signal.
 */
void TinySqlApi::readTables()
{
    QString query;
    query.append(QString("SELECT name FROM sqlite_master WHERE type='table'"));
    client->sendRequest(ReadTablesReq, query, "");
}

/*!
 * Request all columns (schema) for a current table.
 * Asynchronous method, emits tinySqlApiColumnsRes signal.
 */
void TinySqlApi::readColumns()
{
    QString query;
    query.append( QString("PRAGMA table_info(%1)").arg(tableName) );
    client->sendRequest(ReadColumnsReq, query, "");
}

/*!
 * Request all items from table. Emits  
 * TinySqlApiRead signal with the items or errorcode.
 */       
void TinySqlApi::readAll(int columnsCount)
{
    DPRINT << "readAll";
    if( columnsCount >= 0) {
        columns = columnsCount;
        DPRINT << "columns:" << columns;
    }
    QString query;
    query.append( QString("SELECT * FROM %1").arg(tableName) );
    client->sendRequest(ReadAllGenItemsReq, query, "");
}

/*!
 * Request to subscribe for changes in item's information. 
 * The notification is sent if any client changes the idem.
 *
 * \param identifier of the item in the list (based to primary key)
 */
void TinySqlApi::subscribeChangeNotifications(const QVariant &identifier)
{
    client->sendRequest(SubscribeNotificationsReq, "", identifier);
}

/*!
 * Request to unsubscribe for changes in item's information. 
 *
 * \param identifier of the item in the list (based to primary key)
 */
void TinySqlApi::unsubscribeChangeNotifications(const QVariant &identifier)
{
    client->sendRequest(UnsubscribeNotificationsReq, "", identifier);
}

/*!
 * Writes any value(s). Inserts new item(row) to the table. Asynchronous method, emits 
 * TinySqlApiWrite signal.
 * Important: written data must match with the first write in the table. 
 * The structure is defined by calling initialize() 
 *
 * \param item contains the values for new item.
 */ 
void TinySqlApi::writeItem(QVariant &item)
{
    QString query;
    query.append( QString("INSERT INTO %1 VALUES (").arg(tableName) );

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
    client->sendRequest(WriteGenItemReq, query, itemList[0].toString());
}

/*!
 * Cancels last async operation going on
 * (removes the request from server queue if it still  exists). 
 */
void TinySqlApi::cancelAsyncRequest()
{
    client->sendRequest(CancelLastReq);
}

/*!
 * Deletes information stored for the given key identifier (primary key).
 * Asynchronous method, emits tinySqlApiDelete signal.
 *
 * \param identifier of the item in the list
 */
void TinySqlApi::deleteItem(const QVariant &identifier)
{
    QString query;
    query.append( QString("DELETE FROM %1 WHERE %2 = '%3'").arg(tableName).arg(primaryKey).arg(identifier.toString()) );
    client->sendRequest(DeleteReq, query, identifier);
}

/*!
 * Delete all items associated with this initialised list.
 * Asynchronous method, emits tinySqlApiDeleteAll signal.
 *
 */
void TinySqlApi::deleteAll(const QString &name)
{
    QString query;
    if (name == "") {
        query.append( QString("DROP TABLE %1").arg(tableName) );
    }
    else {
        query.append( QString("DROP TABLE %1").arg(name) );
    }
    client->sendRequest(DeleteAllReq, query, "");
}

/*!
 * Changes the current table name.
 */
void TinySqlApi::setTable(const QString &name)
{
    tableName = name;
}

/*!
 * Changes the current primary key value.
 */
void TinySqlApi::setPrimaryKey(const QString &key)
{
    primaryKey = key;
}

/*!
 * Changes the current database.
 */
void TinySqlApi::changeDB(const QString &fileName)
{
    client->sendRequest(ChangeDBReq, "", fileName);
}

void TinySqlApi::handleItemDataRes(QDataStream &stream)
{
    int status;
    stream >> status;
    DPRINT << "SQLITEAPICLI:Status_text:" << status;
    DPRINT << "SQLITEAPICLI:columns:" << columns;

    QList< QList<QVariant> > itemList;
    QList<QVariant> item;

    int count=0;
    while (!stream.atEnd()) {
        DPRINT << "SQLITEAPICLI:new row";
        while (count++<columns && !stream.atEnd()) {
            DPRINT << "reading item " << count;
            item << stream;
        }
#ifdef QT_DEBUG
        foreach (QVariant rowValue, item) {
            DPRINT << "SQLITEAPICLI:new gen item:" << rowValue.toString();
        }
#endif
        itemList << item;
        item.clear();
        count = 0;
    }
    DPRINT << "SQLITEAPICLI:rows:" << itemList.count();
    if (itemList.count()==0) {
        status = NotFoundError;
    }
    emit tinySqlApiRead( (TinySqlApiServerError)status, itemList );
}

void TinySqlApi::handleTablesDataRes(QDataStream &stream)
{
    int status;
    stream >> status;
    DPRINT << "SQLITEAPICLI:Status_text:" << status;

    QList<QVariant> tables;

    while (!stream.atEnd()) {
        tables << stream;
    }

    if (tables.count()==0) {
        status = NotFoundError;
    }
    emit tinySqlApiTablesRes( (TinySqlApiServerError)status, tables );
}

void TinySqlApi::handleColumnsDataRes(QDataStream &stream)
{
    int status;
    stream >> status;
    DPRINT << "SQLITEAPICLI:Status_text:" << status;

    QList<QVariant> columns;

    while (!stream.atEnd()) {
        columns << stream;
    }

    if (columns.count()==0) {
        status = NotFoundError;
    }
    emit tinySqlApiColumnsRes( (TinySqlApiServerError)status, columns );
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

    emit tinySqlApiItemCount( (TinySqlApiServerError)status, count );
}

void TinySqlApi::handleNotification(QDataStream &stream, int response)
{
    QVariant itemKey;
    stream >> itemKey;
    if (response == int(UpdateNotification)) {
        emit tinySqlApiUpdateNotification( itemKey );
    }
    else{
        emit tinySqlApiDeleteNotification( itemKey );
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
        emit tinySqlApiServiceInitialized((TinySqlApiServerError)status);
        break;
        
    // Response to read(id), readAll (list of QVariant's list)
    case ItemDataRes:
        handleItemDataRes( stream );
        break;

    case TablesRes:
        handleTablesDataRes( stream );
        break;

    case ColumnsRes:
        handleColumnsDataRes( stream );
        break;

    case CountRes:
        handleCountRes( stream );
        break;

    case WriteGenItemRes:
        stream >> status;
        DPRINT << "SQLITEAPICLI:WriteGenItemRes:" << status;
        emit tinySqlApiWrite( (TinySqlApiServerError)status );
        break;

    case DeleteRes:
        stream >> status;
        DPRINT << "SQLITEAPICLI:Delete:" << status;
        emit tinySqlApiDelete();
        break;

    case DeleteAllRes:
        stream >> status;
        DPRINT << "SQLITEAPICLI:DeleteAll:" << status;
        emit tinySqlApiDeleteAll();
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
        //Q_ASSERT(false);
        clientNotifier->confirmReadyToReceiveNext();
        return;
        break;
    }
    client->serverResponseReceived();
    clientNotifier->confirmReadyToReceiveNext();
}

// Signals


/*!
 * This signal is emitted when Sqlite API is successfully initialised.
 * \param error error code.
 * void tinySqlApiServiceInitialized(TinySqlApiServerError error)
 */

/*!
 * This signal is emitted in response to asynchronous method read
 * and readAll. Signal emitted when the operation is complete.
 * \param error - NoError, if operation was successful
 * \param itemList - List of items from the table. Can be empty if not found.
 * void tinySqlApiRead(TinySqlApiServerError error, QList< QList<QVariant> > itemList)
 */

/*!
 * This signal is emitted in response to asynchronous method count
 * Signal emitted when the operation is complete
 * \param error - NoError, if operation was successful
 * \param count - Count of items in the table
 *  void tinySqlApiItemCount(TinySqlApiServerError error, int count)
 */

/*!
 * This signal is emitted in response to asynchronous method writeItems.
 * \param error - NoError, if operation was successful
 * void tinySqlApiWrite(TinySqlApiServerError error)
 */

/*!
 * The signal is emitted when changes have been made by another client and
 * this client was subscribed for the item changes.
 * \param identifier - Id of the changed item
 * void tinySqlApiUpdateNotification(const QVariant &identifier)
 */

/*!
 * This signal is emitted in response to asynchronous method delete.
 * Note, this signal is emitted, regardless if the deleted item was found or not (due SQLite)
 * void tinySqlApiDelete()
 */

/*!
 * The signal is emitted when item has been deleted by another client,
 * and this client was subscribed for the item changes.
 * \param identifier - Id of the deleted item
 * void tinySqlApiDeleteNotification(const QVariant &identifier)
 */

/*!
 * This signal is emitted in response to asynchronous method deleteAll.
 * Note, this signal is emitted, regardless if the deleted item was found or not (due SQLite)
 * void tinySqlApiDeleteAll()
 */
