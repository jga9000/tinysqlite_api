#ifndef _SQLITEAPICLIENT_H_
#define _SQLITEAPICLIENT_H_

#include <QLocalSocket>
#include <QQueue>
#include <QVariant>
#include "tinysqliteapiglobal.h"
#include "tinysqliteapidefs.h"

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
    //! Class for defining the server request.
    //  Used for queueing the requests when server is busy.
    class TinySqlApiServerRequest : public QObject
    {
    public:
        TinySqlApiServerRequest(ServerRequestType request, const QString &msg, 
                                const QVariant &itemKey);
    public:
        ServerRequestType request() const;
        QString msg() const;
        QVariant itemKey() const;
        
    private:
        ServerRequestType mRequest;
        QString mMsg;
        QVariant mItemKey;
    };

public:   
    explicit TinySqlApiClient(QObject *parent, int clientId);
    virtual ~TinySqlApiClient();

public:
    void sendRequest(ServerRequestType request, const QString &msg, const QVariant &itemKey);
    void sendRequest(ServerRequestType request);
    void sendNextRequest();
    void serverResponseReceived();

    /*!
     *  Check if response to last request is still pending
     *  \return true on no response to request yet received from server
     */
    bool isWaitingForResponse() { return mWaitingServerResponse; }

private slots:
    void handleError(QLocalSocket::LocalSocketError socketError);
    void handleConnected();
    void handleDisconnected();
    void dataSent(qint64 bytes);
    void handleReceiveConfirmation();

private:
    Q_DISABLE_COPY(TinySqlApiClient)
    void connectServer();

private: // For testing
    #ifdef UNITTEST
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
