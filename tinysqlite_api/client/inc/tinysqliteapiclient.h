#ifndef _SQLITEAPICLIENT_H_
#define _SQLITEAPICLIENT_H_

#include <QLocalSocket>
#include <QQueue>
#include <QVariant>
#include <tinysqliteapiglobal.h>
#include <tinysqliteapidefs.h>

//! Timeout for server to response to the previous request,
// before submitting new request (overriding)
const unsigned int TinySqlApiServerAckTimeOutSecs = 10;

/*!
 * Sqlite API client class declaration 
 * Implements client/server socket connection to the Sqlite API server.
 */
 
class TinySqlApiClient : public QLocalSocket
    {
    Q_OBJECT

    // Class declaration
    //! Class for defining the server request. Used for queueing the requests when server is busy.

    class TinySqlApiServerRequest : public QObject
    {
    public:
        /*! Constructs new request object
         *
         * \param request Request id
         * \param msg Request message
         * \param itemKey Request primary key
         */
        inline TinySqlApiServerRequest(ServerRequestType request, const QString &msg, const QVariant &itemKey) :
                                          mRequest(request), mMsg(msg), mItemKey(itemKey) {;}
    public:

        /*! Getter for the request constant id
         *
         * \return Request Id
         */
        inline ServerRequestType request() const { return mRequest; }

        /*! Getter for the request string
         *
         * \return String
         */
        inline QString msg() const { return mMsg; }

        /*! Getter for the primary key
         *
         * \return Key
         */
        inline QVariant itemKey() const { return mItemKey; }

    private:

        ServerRequestType mRequest;
        QString mMsg;
        QVariant mItemKey;
    };

public:
   
    /*!
     * Construct new TinySqlApiClient
     * Also starts the Sqlite API server process if not started yet.
     */
    explicit TinySqlApiClient(QObject *parent, int clientId);
    
    //! Destructor
    virtual ~TinySqlApiClient();

public:
    /*! 
     *  Connects the server and sends the request message
     *  \param request The request code
     *  \param msg The request message
     *  \param itemKey Identifier for the item under change (primary key).
     *                 The change notification is based on this key.
     */
    void sendRequest(ServerRequestType request, const QString &msg, const QVariant &itemKey);
    
    /*! 
     *  Connects the server and sends the request message without any message
     *  \param request The request code
     */    
    void sendRequest(ServerRequestType request);

    /*! 
     *  Connects the server and sends the next request message from the queue
     */      
    void sendNextRequest();
	
    /*!
     *  Server sent response for the last request. Next request can be sent.
     */
    void serverResponseReceived();

    /*!
     *  Check if response to last request is still pending
     *  \return true on no response to request yet received from server
     */
    inline bool isWaitingForResponse() { return mWaitingServerResponse; }

private slots:
    
    //! Slot for QLocalSocket::error signal    
    void handleError(QLocalSocket::LocalSocketError socketError);

    //! Slot for QLocalSocket::connected signal
    void handleConnected();
    
    //! Slot for QLocalSocket::disconnected signal    
    void handleDisconnected();

    //! Slot for QLocalSocket::bytesWritten signal
    void dataSent(qint64 bytes);

    //! Slot for QLocalSocket::readyRead signal        
    void handleReceiveConfirmation();

private:

    Q_DISABLE_COPY(TinySqlApiClient)    

    /*! 
     *  Connects the localsocket to the Server socket
     *  Connects the signals to slot callbacks.
     */     
    void connectServer();

private: // For testing
    #ifdef TEST_EUNIT
        friend class UT_TinySqlApiClient;
       
    #endif

private:    

    // Identifier of this client for the sqliteapi server
    int mClientId;

    // Retry count for connect
    unsigned int mRetries;

    // Current socket connection status
    bool mConnected;

    // Current request/response status
    bool mWaitingServerResponse;

    // Queue for requests, used when waiting for the response
    QQueue<TinySqlApiServerRequest*> mRequestQueue;
    };

#endif // _SQLITEAPICLIENT_H_
