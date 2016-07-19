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

    int client_ = 0;
    timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
       while(running()) {
           fd_set fds;
           FD_ZERO(&fds);
           FD_SET(master_, &fds);
           if (select(master_ + 1, &fds, NULL, NULL, &tv) > 0) {
               if ((client_ = accept(master_, 0, 0)) > 0) {
                    push_request(client_);
               }
           }
       }

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
    connections_.erase(aConnection);
    shutdown(aConnection, SHUT_RDWR);
}

void CServer::on_request_end(int aConnection) {
    connections_.erase(aConnection);
    shutdown(aConnection, SHUT_RDWR);
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
