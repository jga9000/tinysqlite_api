#include <QCoreApplication>
#include <QStringList>
#include <QSharedMemory>
#include "sqliteapiserverdefs.h"
#include "sqliteapiserver.h"
#include "serverlauncher.h"
#include "logging.h"

ServerLauncher::~ServerLauncher()
{
    DPRINT << "SQLITEAPISRV:~ServerLauncher";
    
    disconnect(mServer, SIGNAL(deleteServerSignal()), this, SLOT(handleExit()));
    
    delete mServer;
    mServer = NULL;
    DPRINT << "SQLITEAPISRV:server deleted";
}

#ifndef EUNIT_ENABLED
ServerLauncher::ServerLauncher(int &argc, char *argv[])
 : QCoreApplication(argc, argv), mServer(NULL)
#else
ServerLauncher::ServerLauncher(int &argc, char *argv[])
 : mServer(NULL)
#endif
{
    mClientId = -1;

    if( argc>0 ) {
        QStringList arguments = QCoreApplication::arguments();
        DPRINT << "SQLITEAPISRV:************";    
        DPRINT << "SQLITEAPISRV:main called with arguments:" << arguments;
        
        if( argc>1 ) {
            mClientId = arguments.at(1).toInt();
        }
    }
    else{
        DPRINT << "SQLITEAPISRV:ERR, no arguments found!";
#ifndef EUNIT_ENABLED
        Q_ASSERT(false);
#endif
    }
    
    DPRINT << "SQLITEAPISRV:TinySqlApiServer" << TinySqlApiServerDefs::TinySqlApiVersion;
}

void ServerLauncher::handleExit()
{
    DPRINT << "SQLITEAPISRV:ServerLauncher::handleExit";
#ifndef EUNIT_ENABLED
    quit();
#endif
}

static bool isStarted()
{
    QSharedMemory sharedMemory;
    
    sharedMemory.setKey(TinySqlApiServerDefs::TinySqlApiServerUniqueKey);
    if(!sharedMemory.attach()) {
        // Not yet created/started, create shared memory
        DPRINT << "SQLITEAPISRV:Server unique ID:" <<
                  TinySqlApiServerDefs::TinySqlApiServerUniqueKey;
        
        if (!sharedMemory.create(1))
        {
            DPRINT << "SQLITEAPISRV:ERR, Unable to create sharedmemory.";
            // Do not handle as error..server can be created
        }
        return false;
    }
    DPRINT << "SQLITEAPISRV:Server already started, OK using existing";

    return true;
}

void ServerLauncher::start(QMutex& mutex)
{
    mServer = new TinySqlApiServer( this );
    Q_CHECK_PTR(mServer);
    DPRINT << "SQLITEAPISRV:Starting server";
 
    connect(mServer, SIGNAL(deleteServerSignal()), this, SLOT(handleExit()));

    if( !mServer->start(mClientId)) {
        DPRINT << "ERR, SQLITEAPISRV:Server failed to start";
        Q_ASSERT(false);
    }
    else{
        DPRINT << "SQLITEAPISRV:Server running OK";
        DPRINT << "SQLITEAPISRV:unlocking mutex..";
        mutex.unlock();
        DPRINT << "SQLITEAPISRV:Mutex unlocked";        
#ifndef EUNIT_ENABLED        
        exec();
#endif
    }
}
#ifndef EUNIT_ENABLED
int main(int argc, char *argv[])
{
    // Server singleton launch using SharedMemory must be done in server EXE side,
    // from client DLL it won't work
    QMutex mutex;   // Safe to be in stack

    DPRINT << "SQLITEAPISRV:locking mutex..";
    mutex.lock();
    DPRINT << "SQLITEAPISRV:Mutex locked";

    if(!isStarted())
    {
        ServerLauncher *launcher = new ServerLauncher( argc, argv );
        Q_CHECK_PTR(launcher);

        if( launcher->clientId() > 0 ) {
            DPRINT << "SQLITEAPISRV:Creating server";
#ifdef Q_OS_SYMBIAN
            int error = 0;
            QT_TRYCATCH_ERROR(error, launcher->start(mutex));
            if(0 != error){
                DPRINT << "SQLITEAPISRV:exception in ServerLauncher::start() :" << error;
            }
#else   // This branch is for simulation only (WIN32)
            try {
                launcher->start(mutex);
            }
            catch(int error) {
                DPRINT << "SQLITEAPISRV:exception in ServerLauncher::start() :" << error;
            }
            catch(QString& s) {
                DPRINT << "SQLITEAPISRV:exception in ServerLauncher::start() :" << s << endl;
            }
            catch( ... ) {
              DPRINT << "SQLITEAPISRV:exception in ServerLauncher::start() :" << endl;
            }

#endif
        }
        else{
            DPRINT << "SQLITEAPISRV:client-id argument is missing! Server not started.";
            }
        delete launcher;
    }
    else{
        qDebug() << "SQLITEAPISRV:unlocking mutex..";
        mutex.unlock();
        qDebug() << "SQLITEAPISRV:Mutex unlocked";
    }
    
    qDebug() << "SQLITEAPISRV:Server process exit";
    return 0;
}
#endif
