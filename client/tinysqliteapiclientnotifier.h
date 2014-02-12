#ifndef _SQLITEAPICLIENTNOTIFIER_H_
#define _SQLITEAPICLIENTNOTIFIER_H_

#include "QtNetwork/QLocalServer"
//#include "mock_qlocalserver.h"
#include "tinysqliteapiglobal.h"
#include "tinysqliteapidefs.h"

class QLocalSocket;

/*! Sqlite API client notification handler.
 *  Implements server-2-client connection.
 */
class TinySqlApiClientNotifier : public QLocalServer
    {
    Q_OBJECT

public:
    explicit TinySqlApiClientNotifier(QObject *parent, int clientId);
    virtual ~TinySqlApiClientNotifier();

public:
      
    bool startListening();
    void confirmReadyToReceiveNext();

public:
    inline bool isConnected() { return (mSocketNotify ? true : false); }
    
signals:
    void newDataReceived(QDataStream &stream);
    
private slots:
    
    void handleServerResponse();
    
    void handleDisconnect();

    void handleNewConnection();  

private:
    
    Q_DISABLE_COPY(TinySqlApiClientNotifier)

private: // For testing

#ifdef UNIT_TEST
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
