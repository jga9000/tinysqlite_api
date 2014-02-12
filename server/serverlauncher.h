#ifndef _SERVERLAUNCHER_H_
#define _SERVERLAUNCHER_H_

#include <QCoreApplication>
#include <QMutex>

class TinySqlApiServer;

/*
 * Sqlite API server launcher
 */
class ServerLauncher : public QCoreApplication
{
    Q_OBJECT

public:
   //! Constructs new ServerLauncher
    explicit ServerLauncher(int &argc, char *argv[]);
    
    //! Destructor    
    virtual ~ServerLauncher();

    void start(QMutex &mutex);
    inline int clientId() { return mClientId; }
        
public slots:
    // Signaled from server.
    void handleExit();

private:
    TinySqlApiServer *mServer;
    int mClientId;
};

#endif // _SERVERLAUNCHER_H_
