#ifndef _SQLITEAPIGLOBAL_H
#define _SQLITEAPIGLOBAL_H

#ifdef SQLITEAPI_NO_EXPORT
#define SQLITEAPI_EXPORT 
#elif BUILD_SQLITEAPI
#define SQLITEAPI_EXPORT Q_DECL_EXPORT
#else
#define SQLITEAPI_EXPORT Q_DECL_IMPORT
#endif

#endif  // _SQLITEAPIGLOBAL_H