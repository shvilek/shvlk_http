#include <iostream>
#include <signal.h>
#include <unistd.h>
#include "cserver.h"

shvilek::CServer *srv;

void handle_exit(int aSig) {
    srv->stop();

}

int main(int argc, char **argv )
{
    daemon(1, 0);
    srv = new shvilek::CServer(32);
    std::string host;
    int port = 8080;
    std::string dir;
    bool host_set = false;
    bool dir_set = false;
    bool port_set = false;

    int opt = getopt( argc, argv, "h:p:d:");
        while( opt != -1 ) {
            switch( opt ) {
                case 'h':
                    host=optarg;
                    host_set=true;
                    break;

                case 'p':
                    port = atoi(optarg);
                    port_set = true;
                    break;

                case 'd':
                    dir = optarg;
                    dir_set = true;
                    break;
            }

            opt = getopt( argc, argv, "h:p:d:");
        }
    signal(SIGTERM, handle_exit);
    srv->run(host, dir, port);
    delete srv;
    return 0;
}

