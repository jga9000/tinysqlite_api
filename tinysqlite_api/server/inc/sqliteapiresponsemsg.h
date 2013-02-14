#ifndef _SQLITEAPIRESPONSEMSG_H_
#define _SQLITEAPIRESPONSEMSG_H_

#include <QObject>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QVariant>
#include "tinysqliteapidefs.h"

class TinySqlApiResponseMsg : public QObject
{
    Q_OBJECT

public:
    //! Constructs new TinySqlApiResponseMsg
    explicit TinySqlApiResponseMsg(QObject *parent, ServerRequestType request, QSqlQuery &query, int id, const QVariant &itemKey );

    //! Destructor    
    virtual ~TinySqlApiResponseMsg();

public:
    inline int rows() const { return mSqlQuery.record().count(); }
    int startReading();
    bool getNextValue( int &value );
    bool getNextValue( QString &value );
    bool getNextValue( QVariant &value );
    inline int id() const { return mId; }
    inline int request() const { return mRequest; }
    inline QSqlError::ErrorType queryError() const { return mSqlError; }
    inline QString queryErrorStr() const { return mSqlQuery.lastError().text(); }
    inline void setError(QSqlError::ErrorType error)  { mSqlError = error; }

    // SQL primary key for the response item/row
    inline QVariant itemKey() const { return mItemKey; }

private:
    bool nextCol();

private:
    ServerRequestType mRequest;
    QSqlQuery mSqlQuery;
    int mId;
    int mCol;

    QVariant mItemKey;
    QSqlError::ErrorType mSqlError;

    #ifdef TEST_EUNIT
        friend class UT_TinySqlApiResponseMsg;
    #endif

};            
            
#endif /* _SQLITEAPIRESPONSEMSG_H_ */
