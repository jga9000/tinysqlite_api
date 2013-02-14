#ifndef _SQLITEAPISQL_H_
#define _SQLITEAPISQL_H_

#include <QSqlDatabase>
#include "sqliteapirequestmsg.h"

class TinySqlApiResponseMsg;

/*
 * Interface for Sqlite API storage.
 */
class TinySqlApiSql : public QObject
{
    Q_OBJECT

public:
    //! Constructs new TinySqlApiSql object  
    explicit TinySqlApiSql(QObject *parent);
    
    //! Destructor    
    virtual ~TinySqlApiSql();

public:
    bool initialize();
    TinySqlApiResponseMsg *sqlExecute(TinySqlApiRequestMsg& msg);

private: // For testing    

    #ifdef TEST_EUNIT
        friend class UT_TinySqlApiSql;
    #endif

    QSqlDatabase mDb;
};

#endif // _SQLITEAPISTORAGE_H_
