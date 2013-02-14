#ifndef _SQLITEAPICLIENTNOTIFIER_H_
#define _SQLITEAPICLIENTNOTIFIER_H_

#include "QtNetwork/QLocalServer"
//#include "mock_qlocalserver.h"
#include <tinysqliteapiglobal.h>
#include <tinysqliteapidefs.h>

class QLocalSocket;

/*! Sqlite API client notification handler.
 *  Implements server-2-client connection.
 */
class TinySqlApiClientNotifier : public QLocalServer
    {
    Q_OBJECT

public:
   
    /*!
     * Construct new TinySqlApiClientNotifier
     */
    explicit TinySqlApiClientNotifier(QObject *parent, int clientId);
    
    //! Destructor
    virtual ~TinySqlApiClientNotifier();

public:
    
    /*! Starts listening notifications from the server.
     *  \return true on success, false on failure.
     */    
    bool startListening();

    /*! Has to be called after the last received data is processed.
     *  Otherwise the next 'write' from server process would get missed.
     *  Server queues the responses and sends again until this is called.
     */
    void confirmReadyToReceiveNext();

public:
    inline bool isConnected() { return (mSocketNotify ? true : false); }
    
signals:
    
    /*! This signal is emitted when client receives any data
     *  from the server. 
     *  \param stream - Contains reference to the new stream that uses the socket
     */    
    void newDataReceived(QDataStream &stream);
    
private slots:
    
    //! Slot for QLocalSocket::readyRead signal
    void handleServerResponse();
    
    //! Slot for QLocalSocket::disconnected signal
    void handleDisconnect();

    //! Slot for QLocalSocket::newConnection signal
    void handleNewConnection();  

private:
    
    Q_DISABLE_COPY(TinySqlApiClientNotifier)

private: // For testing

#ifdef TEST_EUNIT
    friend class UT_TinySqlApiClientNotifier;
#endif

private: // data
    
    // Member variables

    //! Socket which listens the server notifications. 
    QLocalSocket *mSocketNotify;
    
    //! Unique client id, used in socket connection naming
    int mClientId;
    };

#endif // _SQLITEAPICLIENTNOTIFIER_H_
