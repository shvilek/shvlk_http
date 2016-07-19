#include <vector>
#include <string>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <iostream>

#include "chandler.h"
#include "cserver.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <ctime>

namespace shvilek {
CHandler::CHandler(CServer& aOwner, int aId) : owner_(aOwner), id_(aId) {

}
void CHandler::run() {

  while (true) {
      try {
        int connection_ = 0;
        {
          std::unique_lock<std::mutex> lock(owner_.req_mutex());

          while (owner_.running() && owner_.tasks_empty()) {
            owner_.condition().wait(lock);
          }

          if (owner_.running() == false) {
              owner_.on_handler_exit(id_);
              break;//return;
          }

          // get the task from the queue
          connection_ = owner_.tasks().front();
          owner_.tasks().pop_front();

        } // release lock
        process_request(connection_);

        } catch(const std::exception &exc) {
          owner_.on_handler_exit(id_);
          break;
      }
      }

}

int CHandler::send_data(int aConnection, const char* aData, int aSize) {
    int r = send(aConnection, aData, aSize, MSG_NOSIGNAL);
    if (r <= 0) {
        owner_.on_request_error(aConnection);
        return -1;
    } else {
        return r;
    }
}

void CHandler::process_request(int aConnection) {
    char data;
    char read_data[2048];
    std::stringstream ss;

    request_state state_ = rs_initial_line_method;
    std::string method_;
    std::string path_;
    std::string version_;
    std::vector<std::string> headers_;
    std::string head_line_;
    int count = 0;
    while (state_ != rs_body && (count = recv(aConnection, &data, 1, MSG_NOSIGNAL)) > 0) {
        //std::cout << data;
        if (state_ == rs_body || data == '\r') continue;
        switch(state_) {
        case rs_initial_line_method:
            if (data != ' ')
                method_ += data;
            else {
                std::cout << "METHOD: " << method_ << std::endl;
                state_ = rs_initial_line_path;
            }
            break;
        case rs_initial_line_path:
            if (data != ' ')
                path_ += data;
            else {
                std::cout << "PATH: " << path_ << std::endl;
                state_ = rs_initial_line_version;
            }
            break;
        case rs_initial_line_version:

            if (data != '\n')
                version_ += data;
            else {
                std::cout << "VERSION: " << version_ << std::endl;
                state_ = rs_headers;
            }
            break;
        case rs_headers:
            if (data != '\n')
                head_line_ += data;
            else {
                if (head_line_.empty())
                    state_ = rs_body;
                else {
                    headers_.push_back(head_line_);
                }
                head_line_.clear();
            }
            break;
        case rs_body:
            //if (data == '\n') {
            //}
            break;
        default:
            break;
        }
    }

                path_ = owner_.root_path() + path_;
                std::ifstream file_(path_, std::ios::binary);
                //file_.open(path_, std::ios::binary);

                if (file_.good()) {
                    int r = 0;
                    file_.seekg (0, std::ios::end);
                    size_t size_ = file_.tellg();
                    file_.seekg (0, std::ios::beg);

                    auto now = std::chrono::system_clock::now();
                    auto now_c = std::chrono::system_clock::to_time_t(now);

                    ss << "HTTP/1.0 200 OK\r\n";
                    ss << "Date: " << std::put_time(std::localtime(&now_c), "%c") << "\r\n";
                    ss << "Content-Type: text/html\r\n";
                    ss << "Content-Length: " << size_  << "\r\n" << "\r\n";
                    r = send_data(aConnection, ss.str().data(), ss.str().size());
                    if (r == -1) {
                        std::cout << "ERROR!";
                        flush(std::cout);
                        return;
                    }

                    ss.str("");

                    file_.read(read_data, sizeof(read_data));
                    while (file_.gcount() > 0) {
                        std::cout << std::string(&read_data[0], file_.gcount()) << std::endl;
                        flush(std::cout);
                        r = send_data(aConnection, read_data, file_.gcount());
                        if (r == -1)
                            return;
                        file_.read(read_data, sizeof(read_data));
                    }
                    owner_.on_request_end(aConnection);
                } else {
                    int r = send_data(aConnection, "HTTP/1.0 404 OK", sizeof("HTTP/1.0 404 OK"));
                    if (r == -1)
                        return;
                    owner_.on_request_end(aConnection);
                }

        flush(std::cout);
    //}

    if (count <= 0) {
        owner_.on_request_error(aConnection);
    }
}
}
