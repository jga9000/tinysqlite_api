#ifndef _SQLITEAPIDEFS_H
#define _SQLITEAPIDEFS_H

// Constants

//! Server request codes, used in localsocket communication
enum ServerRequestType
{
    RegisterReq=1,
    UnregisterReq,
    CreateTableReq,
    ReadGenItemReq,
    CountReq,
    ReadTablesReq,
    ReadColumnsReq,
    ReadAllGenItemsReq,
    SubscribeNotificationsReq,
    UnsubscribeNotificationsReq,
    WriteGenItemReq,
    CancelLastReq,
    DeleteReq,
    DeleteAllReq,
    ChangeDBReq
};

//! Server response codes, used in localsocket communication
enum ServerResponseType
{
    UndefinedRes,
    InitializedRes,
    ItemDataRes,
    TablesRes,
    ColumnsRes,
    CountRes,
    WriteGenItemRes,
    DeleteRes,
    DeleteAllRes,
    //ChangeDBRes,
    UpdateNotification,
    DeleteNotification,
    ConfirmationRes
};

//! Common server error codes
enum TinySqlApiServerError
{
    NoError,
    InitializationError,
    NotFoundError,
    AlreadyExistError,
    UndefinedError
};

#endif // _SQLITEAPIDEFS_H
