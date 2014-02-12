#ifndef _SQLITEAPIAPI_H_
#define _SQLITEAPIAPI_H_

// System includes
#include <QObject>
#include <QVariant>

// User includes
#include "tinysqliteapiglobal.h"
#include "tinysqliteapidefs.h"

// Forward declarations
class TinySqlApiClient;
class TinySqlApiClientNotifier;

// Class declaration
//! Type class for initializing the storage table for the TinySqlApi.
/*!
 * This class is the generic structure for SQL storage table.
 * Defines the single item for the storage table, can contain any variable type.
 */
class TinySqlApiInitializer : public QVariant
{
public:
    TinySqlApiInitializer(QVariant::Type type, const QString &name, int maxLength);
public:
    QString name() const;
    int maxLength() const;
private:
    QString _name;
    int _maxLength;
};

// Class declaration

/*! Sqlite API client API.
 *
 * Sqlite API API class
 */
class SQLITEAPI_EXPORT TinySqlApi : public QObject
{
    Q_OBJECT

public:
    explicit TinySqlApi(const QString &table = "", QObject *parent = 0);
    virtual ~TinySqlApi();

public:

    void initialize(const TinySqlApiInitializer &identifier,
                    const QList<TinySqlApiInitializer> &initializers);
    void read(const QVariant &identifier);
    void count();
    void readTables();
    void readColumns();
    void readAll(int columnsCount = -1);
    void subscribeChangeNotifications(const QVariant &identifier);
    void unsubscribeChangeNotifications(const QVariant &identifier);
    void writeItem(QVariant &item);
    void cancelAsyncRequest();
    void deleteItem(const QVariant &identifier);
    void deleteAll(const QString &name = "");
    void setTable(const QString &name);
    void setPrimaryKey(const QString &name);
    void changeDB(const QString &fileName);

signals:

    void tinySqlApiServiceInitialized(TinySqlApiServerError error);
    void tinySqlApiRead(TinySqlApiServerError error, QList< QList<QVariant> > itemList);
    void tinySqlApiTablesRes(TinySqlApiServerError error, QList<QVariant> tables);
    void tinySqlApiColumnsRes(TinySqlApiServerError error, QList<QVariant> columns);
    void tinySqlApiItemCount(TinySqlApiServerError error, int count);
    void tinySqlApiWrite(TinySqlApiServerError error);
    void tinySqlApiUpdateNotification(const QVariant &identifier);
    void tinySqlApiDelete();
    void tinySqlApiDeleteNotification(const QVariant &identifier);
    void tinySqlApiDeleteAll();

private:

    Q_DISABLE_COPY(TinySqlApi)

    void handleItemDataRes(QDataStream &stream);
    void handleTablesDataRes(QDataStream &stream);
    void handleColumnsDataRes(QDataStream &stream);
    void handleCountRes(QDataStream &stream);
    void handleNotification(QDataStream &stream, int response);

private slots:

    void handleNewData(QDataStream &stream);

private:

    TinySqlApiClient* client;
    TinySqlApiClientNotifier* clientNotifier;

    // Identifier of this client in the sqliteapi server
    int clientId;

    // Name of the storage (SQL table)
    QString tableName;

    // Name of the primary key for each item
    QString primaryKey;

    // This tells the structure size(columns)
    int columns;

#ifdef UNITTEST
    friend class UT_TinySqlApi;
#endif
};

#endif // _SQLITEAPIAPI_H_
