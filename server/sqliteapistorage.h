#ifndef _SQLITEAPISTORAGE_H_
#define _SQLITEAPISTORAGE_H_

#include <QObject>
#include "sqliteapiserver.h"

class TinySqlApiSql;

/*
 * Generic Sqlite API storage handler.
 */
class TinySqlApiStorage : public QObject
{
    Q_OBJECT

public:
    //! Constructs new TinySqlApiStorage
    explicit TinySqlApiStorage(QObject *parent, TinySqlApiServer &server);
    
    //! Destructor    
    virtual ~TinySqlApiStorage();

    bool initialize(const QString& name = "tinysqlapidb.db");
    
signals:
    void newResponse(TinySqlApiResponseMsg *msg);

private slots:
    // Signaled from server.
    void handleRequest();

private: // For testing    
    #ifdef UNITTEST
        friend class UT_TinySqlApiStorage;
    #endif

    TinySqlApiSql *mSqlHandler;
    TinySqlApiServer &mServer;
};

#endif // _SQLITEAPISTORAGE_H_
