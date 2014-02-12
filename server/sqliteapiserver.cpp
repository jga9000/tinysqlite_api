// Includes
#include "sqliteapiserver.h"
#include "sqliteapirequesthandler.h"
#include "sqliteapiresponsehandler.h"
#include "sqliteapirequestmsg.h"
#include "sqliteapiresponsemsg.h"
#include "sqliteapistorage.h"
#include "logging.h"

#include <QDataStream>

TinySqlApiServer::~TinySqlApiServer()
{
    DPRINT << "SQLITEAPISRV:~TinySqlApiServer";

    DPRINT << "SQLITEAPISRV:Deleting responsehandlers";

    disconnect(mRequestHandler, SIGNAL(newRequest(TinySqlApiRequestMsg *)), this, SLOT(handleRequest(TinySqlApiRequestMsg *)));    
    disconnect(mStorageHandler, SIGNAL(newResponse(TinySqlApiResponseMsg *)), this, SLOT(handleResponse(TinySqlApiResponseMsg *)));
    
    qDeleteAll(mResponseHandlers.begin(), mResponseHandlers.end());
    mResponseHandlers.clear();
    
    qDeleteAll(mRequestQueue.begin(), mRequestQueue.end());
    mRequestQueue.clear();

    delete mStorageHandler;
    mStorageHandler = NULL;
    delete mRequestHandler;
    mRequestHandler = NULL;

    DPRINT << "SQLITEAPISRV:~TinySqlApiServer end";
}

TinySqlApiServer::TinySqlApiServer(QObject *parent) :
    QObject(parent)
{
    // Create the SQL thread here
    mStorageHandler = new TinySqlApiStorage( 0, *this );
    Q_CHECK_PTR(mStorageHandler);
    mRequestHandler = new TinySqlApiRequestHandler(0);
    Q_CHECK_PTR(mRequestHandler);
    
    connect(mRequestHandler, SIGNAL(newRequest(TinySqlApiRequestMsg *)), this, SLOT(handleRequest(TinySqlApiRequestMsg *)));    
    connect(mStorageHandler, SIGNAL(newResponse(TinySqlApiResponseMsg *)), this, SLOT(handleResponse(TinySqlApiResponseMsg *)));

    connect(mRequestHandler, SIGNAL(abnormalDisconnection()), this, SLOT(abnormalServerExit()) );
}

bool TinySqlApiServer::start( int firstClientId )
{
    if( !mStorageHandler->initialize() ) {
        DPRINT << "SQLITEAPISRV:ERR, Storage initialize failed";
        Q_ASSERT(false);
        return false;
    }
    if( !mRequestHandler->initialize() ) {
        DPRINT << "SQLITEAPISRV:ERR, Request handler initialize failed";
        Q_ASSERT(false);
        return false;
    }
    DPRINT << "SQLITEAPISRV:Registering first client with id" << firstClientId;
    addClientId(firstClientId);

    return true;
}

TinySqlApiRequestMsg *TinySqlApiServer::getNextRequest()
{
    DPRINT << "SQLITEAPISRV:getNextRequest(). Queue count:" << mRequestQueue.count();
    if( mRequestQueue.count() == 0 ) {
        DPRINT << "SQLITEAPISRV:ERR, request queue is empty";
        return NULL;
    }
    return mRequestQueue.dequeue();
}

void TinySqlApiServer::addClientId(int id)
{
    DPRINT << "SQLITEAPISRV:addClientId:" << id;
    QHash<int, TinySqlApiResponseHandler *>::const_iterator i = mResponseHandlers.find(id);
    if(i != mResponseHandlers.end() && i.key() == id) {
        // This will be the case only if the request came from client which created the server
        // Already registered (during creation of the server)        
        return;
    }
    // New client registered
    // Here the server (socket) creates permanent connection to client socket (for responses/notifications)
    TinySqlApiResponseHandler *responsehandler = new TinySqlApiResponseHandler(0, *this, id);
    Q_CHECK_PTR(responsehandler);    
    mResponseHandlers[id] = responsehandler;    // Hash table, client id is the identifier
    DPRINT << "SQLITEAPISRV:Client" << id << "registered.";    
    DPRINT << "SQLITEAPISRV:Client count now:" << mResponseHandlers.count();    
}

void TinySqlApiServer::abnormalServerExit()
{
    if (mResponseHandlers.count() <= 1) {
        DPRINT << "abnormalServerExit";
        emit deleteServerSignal();
    }
}

void TinySqlApiServer::removeClientId(int id)
{
    QHash<int, TinySqlApiResponseHandler *>::const_iterator i = mResponseHandlers.find(id);
    if(i == mResponseHandlers.end() || i.key() != id) {
        DPRINT << "SQLITEAPISRV:ERR, remove client: id not found:" << id;
    }
    else {
        DPRINT << "SQLITEAPISRV: client id:" << id << "removed";
        mResponseHandlers.take(id)->deleteLater();
        if( mResponseHandlers.count()==0) {
            DPRINT << "SQLITEAPISRV:No more registered clients, closing server..";
            emit deleteServerSignal();
        }
    }
}

void TinySqlApiServer::changeSubscription(int id, const QVariant& itemKey, bool enable)
{
    QHash<int, TinySqlApiResponseHandler *>::const_iterator i = mResponseHandlers.find(id);
    if(i != mResponseHandlers.end() && i.key() == id) {
        if( enable ) {
            mResponseHandlers[id]->subscribeForNotifications(itemKey);
        }
        else{
            if( !mResponseHandlers[id]->removeSubscription(itemKey) ) {
                DPRINT << "SQLITEAPISRV:ERR, changeSubscription: key not found:" << itemKey;
            }
        }
    }
    else {
        DPRINT << "SQLITEAPISRV:ERR, Change subscription: client id not found:" << id;
    }
}

TinySqlApiResponseHandler* TinySqlApiServer::handler(int id) const
{
    QHash<int, TinySqlApiResponseHandler *>::const_iterator i = mResponseHandlers.find(id);
    if(i != mResponseHandlers.end() && i.key() == id) {
        return mResponseHandlers[id];
    }
    return NULL;
}

// Used in CancelLastReq
void TinySqlApiServer::removeLastRequest(int id)
{
    if(mRequestQueue.count()==0) {
        DPRINT << "SQLITEAPISRV:ERR, removeLastRequest: no requests found";
        return;
    }
    for( int i=mRequestQueue.count()-1; i>0; i-- ) {
        if(mRequestQueue.at(i)->id() == id) {
            mRequestQueue.removeAt(i);
            DPRINT << "SQLITEAPISRV:Removed request for client id:" << id;
            return;
        }
    }
    DPRINT << "SQLITEAPISRV:ERR, removeLastRequest: request for client id not found:" << id;
}

void TinySqlApiServer::handleRequest(TinySqlApiRequestMsg* msg)
{
    Q_CHECK_PTR(msg);

    switch(msg->type()) {
    case RegisterReq:
        DPRINT << "SQLITEAPISRV:RegisterReq";
        addClientId(msg->id());
        sendPlainResponse(*msg);
        delete msg;
        break;

    case UnregisterReq:
        DPRINT << "SQLITEAPISRV:UnregisterReq";
        sendPlainResponse(*msg);
        delete msg;
        removeClientId(msg->id());
        break;

    case SubscribeNotificationsReq:
        DPRINT << "SQLITEAPISRV:SubscribeNotificationsReq";
        changeSubscription(msg->id(), msg->itemKey(), true);
        sendPlainResponse(*msg);
        delete msg;
        break;

    case UnsubscribeNotificationsReq:
        DPRINT << "SQLITEAPISRV:UnsubscribeNotificationsReq";
        changeSubscription(msg->id(), msg->itemKey(), false);
        sendPlainResponse(*msg);
        delete msg;
        break;

    case CancelLastReq:
        DPRINT << "SQLITEAPISRV:CancelLastReq";
        removeLastRequest(msg->id());
        sendPlainResponse(*msg);
        delete msg;
        break;

    case ChangeDBReq:
        DPRINT << "dbname:" << msg->itemKey().toString();
        qDeleteAll(mRequestQueue.begin(), mRequestQueue.end());
        mRequestQueue.clear();
        delete mStorageHandler;
        mStorageHandler = new TinySqlApiStorage( 0, *this );
        Q_CHECK_PTR(mStorageHandler);
        connect(mStorageHandler, SIGNAL(newResponse(TinySqlApiResponseMsg *)), this, SLOT(handleResponse(TinySqlApiResponseMsg *)));
        if (!mStorageHandler->initialize(msg->itemKey().toString())) {
            DPRINT << "SQLITEAPISRV:ERR, Storage re-initialize failed";
            Q_ASSERT(false);
        }
        sendPlainResponse(*msg);
        delete msg;
        break;

    default:
        // SQL queries are handled here
        QHash<int, TinySqlApiResponseHandler *>::const_iterator i = mResponseHandlers.find(msg->id());
        if(i == mResponseHandlers.end() || i.key() != msg->id()) {
            // Ignore requests from unregistered connections
            DPRINT << "SQLITEAPISRV:ERR, Id " << msg->id() << "is not registered! Reguest is ignored.";
            return;
        }

        // Execution of SQL queries happens only after client has been disconnected.
        // This ensures that socket is reserved minimum of time (request socket is not reserved
        // during SQL execution)
        
        // Use queue for the requests, because there may come another request before the disconnection.
        // After client is disconnected (handleDisconnect signal) we can process the request        
        DPRINT << "SQLITEAPISRV:enqueue request(). Queue count before:" << mRequestQueue.count();
        mRequestQueue.enqueue(msg);
        emit newRequest();
        break;
    }

    // Check and handle unsent responses for every client
    foreach (TinySqlApiResponseHandler* handler, mResponseHandlers) {
        if( handler->unsentResponseCount()>0 ) {
            DPRINT << "SQLITEAPISRV:there are" << handler->unsentResponseCount() << "unsent responses for client" << handler->clientId();

            handler->dequeueNextResponse();
        }
    }
}

void TinySqlApiServer::sendPlainResponse(const TinySqlApiRequestMsg& msg)
{
    DPRINT << "SQLITEAPISRV:Sending plain response";

    TinySqlApiResponseHandler *responseHandler = handler(msg.id());
    if( responseHandler ) {

        QByteArray block;

        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(int(QDataStream::Qt_4_0));

        out << int(ConfirmationRes);

        DPRINT << "SQLITEAPISRV:Sending response to client id:" << msg.id();
        responseHandler->sendData(block);
    }
    else{
        // Client may be removed before the request was received
        DPRINT << "SQLITEAPISRV:ERR, Responsehandler not found for id:" << msg.id();
    }
}

void TinySqlApiServer::handleResponse(TinySqlApiResponseMsg *msg)
{
    ServerResponseType responseType = UndefinedRes;
    ServerResponseType notificationType = UndefinedRes;
    bool sendChangeNotification = false;
    int queryError = int(msg->queryError());
    TinySqlApiServerError translatedErrorCode = NoError;
    if( queryError != QSqlError::NoError ) {
        translatedErrorCode = translateSqlError( msg->queryErrorStr() );
    }

    switch(msg->request()) {

    case ReadGenItemReq:
        responseType = ItemDataRes;
        break;

    case WriteGenItemReq:
        responseType = WriteGenItemRes;
        break;

    case CountReq:
        responseType = CountRes;
        break;

    case ReadTablesReq:
        responseType = TablesRes;
        break;

    case ReadColumnsReq:
        responseType = ColumnsRes;
        break;

    case CreateTableReq:
        responseType = InitializedRes;
        // If the error message was "table already exists Unable to execute statement"
        // handle not as error
        if( translatedErrorCode==AlreadyExistError ) {
            DPRINT << "SQLITEAPISRV:Handled as no error";
            translatedErrorCode = NoError;
        }
        break;

    case DeleteReq:
        sendChangeNotification = true;
        notificationType = DeleteNotification;
        responseType = DeleteRes;
        break;

    case DeleteAllReq:
        sendChangeNotification = true;
        notificationType = DeleteNotification;
        responseType = DeleteAllRes;
        break;

    // For ReadAllGenItemsReq Request
    // 1. send message with first msg, put all the next items in the response queue
    // 2. after readyRead signal, next response is sent from the queue
    // 3. repeats 2 until queue is empty

    case ReadAllGenItemsReq:
        responseType = ItemDataRes;
        if (queryError==QSqlError::NoError) {
            enqueueItems(*msg, responseType, translatedErrorCode);
        }
        break;

    // These are not handled here = no response
    case CancelLastReq:
    case RegisterReq:
    case SubscribeNotificationsReq:
    case UnsubscribeNotificationsReq:
    default:
        DPRINT << "SQLITEAPISRV:ERR, handleResponse:" << msg->request() << "is not valid case!";
        Q_ASSERT(false);
        break;
    }
        
    // Send the response
    sendToClient(*msg, responseType, translatedErrorCode);

    // Send change notification also if relevant for the type (and if operation was successful)
    if(sendChangeNotification && (queryError==QSqlError::NoError)){
        sendToClient(*msg, notificationType, translatedErrorCode);
    }
    delete msg;
}

void TinySqlApiServer::convertToSupportedType(QDataStream &in, const QVariant &from) const
{
    QVariant converted;

    switch(from.type()) {
    case QVariant::Bool:
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::String:
    case QVariant::StringList:
    case QVariant::Char:
    case QVariant::Map:
    case QVariant::List:
    case QVariant::ByteArray:
    case QVariant::BitArray:
    case QVariant::Double:
        // Handle QVariant of this type "as-it-is", no conversion
        in << from;
        break;

    case QVariant::LongLong:
    case QVariant::ULongLong:
        // Conversion needed
        converted = from.toInt();
        //DPRINT << "SQLITEAPISRV:Converted to Int:" << converted;
        in << converted;
        break;

    default:
        // Not used yet, conversion needed, QString by default
        converted = from.toString();
        //DPRINT << "SQLITEAPISRV:Converted to Int:" << converted;
        in << converted;
        break;
    }
}

TinySqlApiServerError TinySqlApiServer::translateSqlError(const QString &from) const
{
    if( from.contains("already exists", Qt::CaseInsensitive) ) {
        return AlreadyExistError;
    }    
    else if( from.contains("no such table", Qt::CaseInsensitive) ) {
        return NotFoundError;
    }
    return UndefinedError;
}

void TinySqlApiServer::sendToClient(TinySqlApiResponseMsg &msg, ServerResponseType type, TinySqlApiServerError error)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(int(QDataStream::Qt_4_0));

    out << int(type);

    if( type == UpdateNotification || type == DeleteNotification ) {
        DPRINT << "SQLITEAPISRV:Checking if there are subscribed clients to notify";

        // Output the SQL primary key (identifier for the item)
        out << msg.itemKey();

        // For change notifications,
        // loop thru all recipients, send only subscribed recipients
        foreach (TinySqlApiResponseHandler* handler, mResponseHandlers) {            
            // Do not send change notification for the client making the change (only inform other clients)
            // (match msg client-id to the current responsehandler client-id)            
            if( (handler->isSubscribedFor(msg.itemKey())) && (msg.id()!=handler->clientId()) ) {
                DPRINT << "SQLITEAPISRV:Sending change notification to client id:" << handler->clientId();
                DPRINT << "SQLITEAPISRV:Primary key of the changed item:" << msg.itemKey();
                handler->sendData(block);
            }
        }
    }
    else{
        // This is not a notification, but response for single client's request

        DPRINT << "SQLITEAPISRV:Response type:" << int(type);
        DPRINT << "SQLITEAPISRV:Response error:" << int(error);
        out << error;

        QVariant value;

        if(msg.startReading()>0){
            DPRINT << "SQLITEAPISRV:row count:" << msg.columns();
            while(msg.getNextValue(value)) {
                // convertToSupportedType writes variant into the stream
                DPRINT << "SQLITEAPISRV:Writing value:" << value.toString();
                convertToSupportedType(out, value );
            }
        }
        else{
            DPRINT << "SQLITEAPISRV:No SQL values found";
        }

        TinySqlApiResponseHandler *responseHandler = handler(msg.id());
        if( responseHandler ) {
            DPRINT << "SQLITEAPISRV:Sending response to client id:" << msg.id();
            responseHandler->sendData(block);
        }
        else{
            // Client may be removed before the request was received
            DPRINT << "SQLITEAPISRV:ERR, Responsehandler not found for id:" << msg.id();
        }
    }
}

void TinySqlApiServer::enqueueItems(TinySqlApiResponseMsg &msg, ServerResponseType type, TinySqlApiServerError error)
{
    if (msg.startReading()>0){
        DPRINT << "SQLITEAPISRV:column count:" << msg.columns();

        TinySqlApiResponseHandler *responseHandler = handler(msg.id());
        if (responseHandler){

            bool bytesAvailable(true); // true until !getNextValue

            while(bytesAvailable) {

                QByteArray block;
                QDataStream out(&block, QIODevice::WriteOnly);
                out.setVersion(int(QDataStream::Qt_4_0));

                out << int(type);
                out << error;

                // Read whole row
                while (bytesAvailable) {
                    QVariant value;
                    bytesAvailable = msg.getNextValue(value);

                    if (bytesAvailable) {
                        // convertToSupportedType writes variant into the stream
                        DPRINT << "SQLITEAPISRV:Writing value:" << value.toString();
                        convertToSupportedType(out, value );
                        // Check if column was last
                        if (msg.column()>=msg.columns()) {
                            DPRINT << "SQLITEAPISRV:last item in the row";
                            break;
                        }
                    }
                    else{
                        DPRINT << "SQLITEAPISRV:last value";
                    }
                }
                responseHandler->enqueueData(block);
            }
        }
        else{
            // Client may be removed before the request was received
            DPRINT << "SQLITEAPISRV:ERR, Responsehandler not found for id:" << msg.id();
        }
    }
    else{
        DPRINT << "SQLITEAPISRV:No SQL values found";
    }
}
