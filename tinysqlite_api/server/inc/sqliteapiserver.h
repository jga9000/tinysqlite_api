#ifndef SQLITEAPISERVER_H_
#define SQLITEAPISERVER_H_

#include <QQueue>
#include "sqliteapiresponsemsg.h"

class TinySqlApiRequestHandler;
class TinySqlApiResponseHandler;
class TinySqlApiStorage;
class TinySqlApiRequestMsg;
class TinySqlApiResponseMsg;

/*
Owns the server side objects 
*/
class TinySqlApiServer : public QObject
    {
    Q_OBJECT

public:
    //! Constructs new TinySqlApiServer
    explicit TinySqlApiServer(QObject *parent);
    
    //! Destructor    
    virtual ~TinySqlApiServer();

    bool start( int firstClientId );

    // Sends just the confirmation response to the last request
    // not used for SQL related requests, only for simple ones
    void sendPlainResponse(const TinySqlApiRequestMsg& msg);

    // Get next request/response from the queue
    TinySqlApiRequestMsg *getNextRequest();

    inline int registeredCount() const { return mResponseHandlers.count(); }

    void removeClientId(int id);

signals:

    // Has to be connected to storage handler's slot (handleRequest)
    void newRequest();
    
    // Has to be connected to response handler's response slot (handleResponse)
    void newResponse();

    // Signals ready to delete the server root object
    void deleteServerSignal();

private slots:
    // Reads & enqueues new request message
    void handleRequest(TinySqlApiRequestMsg *msg);

    // Parses the response from storage handler
    void handleResponse(TinySqlApiResponseMsg *msg);

private:

    void addClientId(int id);
    void changeSubscription(int id, const QVariant &itemKey, bool state);
    TinySqlApiResponseHandler* handler(int id) const;
    void removeLastRequest(int id);
    void convertToSupportedType(QDataStream &in, const QVariant &from) const;
    TinySqlApiServerError translateSqlError(const QString &from) const;
    void sendToClient(TinySqlApiResponseMsg &msg, ServerResponseType type, TinySqlApiServerError error);

private:

    QQueue<TinySqlApiRequestMsg *> mRequestQueue;
    TinySqlApiRequestHandler *mRequestHandler;

    // List of registered client ids (for async responses/notifications) and
    // response handlers for relative socket
    QHash<int, TinySqlApiResponseHandler *> mResponseHandlers;

    // Server owns the instance of the storage
    TinySqlApiStorage *mStorageHandler;

    #ifdef TEST_EUNIT
        friend class UT_TinySqlApiServer;
        friend class UT_TinySqlApiStorage;        
    #endif
    };

#endif /* SQLITEAPISERVER_H_ */
