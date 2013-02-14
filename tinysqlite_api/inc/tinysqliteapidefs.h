#ifndef _SQLITEAPIDEFS_H
#define _SQLITEAPIDEFS_H

// Constants

//! Server request codes, used in localsocket communication
enum ServerRequestType
{
    RegisterReq=1,
    CreateTableReq,
    ReadGenItemReq,
    CountReq,
    ReadAllGenItemsReq,
    SubscribeNotificationsReq,
    UnsubscribeNotificationsReq,
    WriteGenItemReq,
    CancelLastReq,
    DeleteReq,
    DeleteAllReq
};

//! Server response codes, used in localsocket communication
enum ServerResponseType
{
    UndefinedRes,
    InitializedRes,
    ItemDataRes,
    CountRes,
    WriteGenItemRes,
    DeleteRes,
    DeleteAllRes,
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
