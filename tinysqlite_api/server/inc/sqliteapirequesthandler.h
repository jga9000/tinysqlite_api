#ifndef _SQLITEAPIREQUESTHANDLER_H_
#define _SQLITEAPIREQUESTHANDLER_H_

#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>
//#include "../mocks/QLocalServer.h"
//#include "../mocks/QLocalSocket.h"

class TinySqlApiRequestMsg;

class TinySqlApiRequestHandler : public QLocalServer
{
    Q_OBJECT

public:
    //! Construct new TinySqlApiRequestHandler
    explicit TinySqlApiRequestHandler(QObject *parent);
    
    //! Destructor
    virtual ~TinySqlApiRequestHandler();

public:
    bool initialize();

private slots:
    void handleNewConnection();
    void handleDisconnect(TinySqlApiRequestMsg *msg);
   
signals:
    void newRequest(TinySqlApiRequestMsg *msg);

private:

    QList<TinySqlApiRequestMsg*> mMsgs;
    
    #ifdef TEST_EUNIT
        friend class UT_TinySqlApiRequestHandler;
        friend class UT_TinySqlApiServer;
    #endif
};

#endif /* _SQLITEAPIREQUESTHANDLER_H_ */
