#include "cserver.h"
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <stdio.h>

#include "utils.h"

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

namespace shvilek {

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

CServer::CServer(int aHandlerCount) : running_(true) {
  for (int i = 0; i < aHandlerCount; ++i)
    add_handler();
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

int CServer::run(const std::string& aHost, const std::string& aDir, int aPort) {
    root_path_ = aDir;
    //std::cout << "PATH: " << root_path_;
    //flush(std::cout);
    int master_ = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(aPort);
    inet_pton(AF_INET, aHost.c_str(), &sa.sin_addr);
    //sa.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(master_, (const sockaddr*)&sa, sizeof(sa));
    listen(master_, SOMAXCONN);
    setnonblocking(master_);

    sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    int client_ = 0;
#define MAX_EVENTS 10
    struct epoll_event ev, events[MAX_EVENTS];
               int epollfd = epoll_create1(0);
               if (epollfd == -1) {
                   perror("epoll_create1");
                   exit(EXIT_FAILURE);
               }

               ev.events = EPOLLIN;
               ev.data.fd = master_;
               if (epoll_ctl(epollfd, EPOLL_CTL_ADD, master_, &ev) == -1) {
                   perror("epoll_ctl: Master");
                   exit(EXIT_FAILURE);
               }

               while  (running()) {
                   int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
                   if (nfds < 0 && errno == EINTR) {
                       continue;
                   }

                   for (int n = 0; n < nfds; ++n) {
                       if (events[n].data.fd == master_) {
                           int conn_sock = accept(master_, (sockaddr *) &addr, &addrlen);
                           if (conn_sock == -1) {
                               continue;
                           }
                           //setnonblocking(conn_sock);
                           push_request(conn_sock);
                           /*for (auto i = clients_.begin(); i != clients_.end();++i) {
                               //sockaddr_in* sa = (sockaddr_in*)&addr;
                               char *ip = inet_ntoa(addr.sin_addr);
                                int sc = 0;
                                if (*i == conn_sock) {
                                    sc = send(*i, "Hello!", 6, MSG_NOSIGNAL);
                                } else {
                                    sc = send(*i, ip, strlen(ip), MSG_NOSIGNAL);
                                }

                                if (sc <= 0) {
                                   shutdown(*i, SHUT_RDWR);
                                   clients_.erase(*i);
                                }
                           }

                           ev.events = EPOLLIN | EPOLLET;
                           ev.data.fd = conn_sock;
                           if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock,
                                       &ev) == -1) {
                               perror("epoll_ctl: conn_sock");
                               exit(EXIT_FAILURE);
                           }*/
                       }
                   }
               }
    /*
    timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
       while(running()) {
           fd_set fds;
           FD_ZERO(&fds);
           FD_SET(master_, &fds);
           if (select(master_ + 1, &fds, NULL, NULL, &tv) > 0) {
               if ((client_ = accept(master_, 0, 0)) > 0) {
                   setnonblocking(client_);
                    push_request(client_);
               }
           }
       }
*/
       clear_all();
       return 0;
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

void CServer::clear_all() {
    while (handlers_.empty() == false) {
        handlers_.begin()->second.join();
        handlers_.erase(handlers_.begin());
    }
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

void CServer::add_handler() {
    static int handler_id = 0;
    //std::cout << "add handler " << handler_id + 1 << std::endl;
    //flush(std::cout);
    std::shared_ptr<CHandler> handler_(new CHandler(*this, ++handler_id));
    workers_.insert(std::make_pair(handler_id, handler_));
    handlers_.insert(std::make_pair(handler_id, std::thread(&CHandler::run, handler_.get())));
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

void CServer::remove_handler(int aId) {
    {

    std::unique_lock<std::mutex> lock(hnd_mutex_);
    //if (workers_.find(aId) != workers_.end()) {
    //std::cout << "join " << aId;
    //handlers_[aId].join();
    //handlers_.erase(aId);
    workers_.erase(aId);
    //std::cout << " erase  " << aId;

    }
    //
    //workers_.erase(aId);
flush(std::cout);
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

void CServer::push_request(int aReq) {
  {
    std::unique_lock<std::mutex> lock(req_mutex());
    tasks_.push_back(aReq);
  }
  condition_.notify_one();
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// sig from handlers
void CServer::on_handler_exit(int aId) {
    remove_handler(aId);
    if (running())
        add_handler();
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

void CServer::on_request_error(int aConnection) {
    //connections_.erase(aConnection);
    close(aConnection);
    //shutdown(aConnection, SHUT_RDWR);
}

void CServer::on_request_end(int aConnection) {
    //connections_.erase(aConnection);
    close(aConnection);
    //shutdown(aConnection, SHUT_RDWR);
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
