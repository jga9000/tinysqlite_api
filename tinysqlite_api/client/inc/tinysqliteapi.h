#ifndef _SQLITEAPIAPI_H_
#define _SQLITEAPIAPI_H_

// System includes
#include <QObject>
#include <QVariant>

// User includes
#include <tinysqliteapiglobal.h>
#include <tinysqliteapidefs.h>

// Forward declarations
class TinySqlApiClient;
class TinySqlApiClientNotifier;

// Class declaration
//! Type class for initializing the storage table for the TinySqlApi.
/*!
 * This class is the generic structure for SQL storage table.
 * Defines the single item for the storage table, can contain any  variable type.
 */
class TinySqlApiInitializer : public QVariant
{
public:
    /*! Constructs new initializer
     *
     * \param type Qt variable type for the storage item
     * \param name Identifying name for the storage item
     * \param maxLength How much data is reserved for the item in the storage
     */
    inline TinySqlApiInitializer(QVariant::Type type, const QString &name, int maxLength) :
                            QVariant(type), mName(name), mMaxLength(maxLength) {;}
public:

    /*! Getter for the name
     *
     * \return The given name for the item
     */
    inline QString name() const { return mName; }

    /*! Getter for the given maximum size
     *
     * \return Maximum length for the item
     */
    inline int maxLength() const { return mMaxLength; }

private:

    QString mName;
    int mMaxLength;
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
    //! Constructs new TinySqlApi object
    explicit TinySqlApi(QObject *parent = 0);
    
    //! Destructor
    virtual ~TinySqlApi();

public:

    /*! Initializes Sqlite API client with detailed storage table initializers.
     * Use only for user-specific data.
     * emits TinySqlApiServiceInitialized signal.   
     *
     * \param table Name of the table to be created. Asynchronous method.
     * \param identifier Defines the type of the identifier (primary key)
     * \param initializers List of initializers defining the type of the stored items
     *
     *    Example: QList<QTinySqlApiInitializer> initializers;
     *    QTinySqlApiInitializer identifier( QVariant::String, "service", 255);
     *    QTinySqlApiInitializer var1(QVariant::Int, "count", 0);
     *    initializers.append(var1);
     *    client->initialize("invitations", identifier, initializers);
     */
    void initialize(const QString &table, const TinySqlApiInitializer &identifier,
                    const QList<TinySqlApiInitializer> &initializers);

   /*!
    * Reads items from cache for a given identifier. Asynchronous method, 
    * emits TinySqlApiRead signal. 
    *
    * \param identifier is the item in the cache (id)
    */
    void read(const QVariant &identifier);

   /*!
    * Request total count of items stored in Sqlite API.
    * Asynchronous method, emits TinySqlApiCount signal.
    */
    void count();

   /*!
    * Request all items from cache. Emits  
    * TinySqlApiRead signal with the items or errorcode.
    */         
    void readAll();

   /*!
    * Request to subscribe for changes in item's information. 
    * The notification is sent if any client changes the idem.
    *
    * \param identifier of the item in the list (based to primary ket)
    */
    void subscribeChangeNotifications(const QVariant &identifier);
    
   /*!
    * Request to unsubscribe for changes in item's information. 
    *
    * \param identifier of the item in the list (based to primary ket)
    */
    void unsubscribeChangeNotifications(const QVariant &identifier);

   /*!
    * Writes any value(s). Inserts new item(row) to the table. Asynchronous method, emits 
    * TinySqlApiWrite signal.
    * Important: written data must match with the first write in the table. 
    * The structure is defined by calling initialize() 
    *
    * \param item contains the values for new item.
    */ 
    void writeItem(QVariant &item);

   /*!
    * Cancels last async operation going on (removes the request from server queue if it
    * still exists). 
    */
    void cancelAsyncRequest();

   /*!
    * Deletes information stored for the given key identifier (primary key).
    * Asynchronous method, emits TinySqlApiDelete signal.
    *
    * \param identifier of the item in the list
    */
    void deleteItem(const QVariant &identifier);

   /*!
    * Delete all items associated with this initialised list.
    * Asynchronous method, emits TinySqlApiDeleteAll signal.
    *
    */
    void deleteAll();

signals:

   /*!
    * This signal is emitted when Sqlite API is successfully initialised.
    * \param error error code.
    */
    void TinySqlApiServiceInitialized(TinySqlApiServerError error);

   /*!
    * This signal is emitted in response to asynchronous method read
    * and readAll.
    * Signal emitted when the operation is complete
    * \param error - NoError, if operation was successful
    * \param itemList - List of items from the cache. Can be empty if not found.
    */
    void TinySqlApiRead(TinySqlApiServerError error, QList< QList<QVariant> > itemList);

   /*!
    * This signal is emitted in response to asynchronous method count
    * Signal emitted when the operation is complete
    * \param error - NoError, if operation was successful
    * \param count - Count of items in the table
    */
    void TinySqlApiItemCount(TinySqlApiServerError error, int count);

   /*!
    * This signal is emitted in response to asynchronous method writeItems.
    * \param error - NoError, if operation was successful
    */
    void TinySqlApiWrite(TinySqlApiServerError error);

   /*!
    * The signal is emitted when changes have been made by another client and
    * this client was subscribed for the item changes.
    * \param identifier - Id of the changed item
    */
    void TinySqlApiUpdateNotification(const QVariant &identifier);

    /*!
     * This signal is emitted in response to asynchronous method delete.
     * Note, this signal is emitted, regardless if the deleted item was found or not (due SQLite)
     */
     void TinySqlApiDelete();

   /*!
    * The signal is emitted when item has been deleted by another client,
    * and this client was subscribed for the item changes.
    * \param identifier - Id of the deleted item
    */
    void TinySqlApiDeleteNotification(const QVariant &identifier);

    /*!
     * This signal is emitted in response to asynchronous method deleteAll.
     * Note, this signal is emitted, regardless if the deleted item was found or not (due SQLite)
     */
    void TinySqlApiDeleteAll();

private:

    Q_DISABLE_COPY(TinySqlApi)

    void handleItemDataRes(QDataStream &stream);
    void handleCountRes(QDataStream &stream);

    void handleNotification(QDataStream &stream, int response);

private slots:

    void handleNewData(QDataStream &stream);

private:

    TinySqlApiClient* mClient;
    TinySqlApiClientNotifier* mClientNotifier;

    // Identifier of this client in the sqliteapi server
    int mClientId;

    // Name of the storage (SQL table)
    QString mTable;

    // Name of the primary key for each item
    QString mPrimaryKey;

    // This tells the size of single item(rows)
    int mRows;

    #ifdef TEST_EUNIT
        friend class UT_TinySqlApi;       
    #endif
};

#endif // _SQLITEAPIAPI_H_
