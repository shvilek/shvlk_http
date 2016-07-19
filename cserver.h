#ifndef CSERVER_H
#define CSERVER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

#include <map>
#include <set>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <thread>
#include <memory>
#include <unistd.h>

#include "chandler.h"

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

namespace shvilek {

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

class CServer {
public:
  typedef std::map<int, std::thread> handlers_t;
  typedef handlers_t::const_iterator handlers_ci;
  typedef handlers_t::iterator handlers_i;

  typedef std::deque<int> tasks_t;
  typedef tasks_t::const_iterator tasks_ci;
  typedef tasks_t::iterator tasks_i;

public:
  CServer(int aHandlerCount);
  int run(const std::string& aHost, const std::string& aDir, int port);
  bool running() const { return running_; }
  void stop() {
      {
        std::unique_lock<std::mutex> lock(req_mutex());
        running_ = false;
      }
      condition_.notify_all();
  }
  bool tasks_empty() const { return tasks_.empty(); }

  tasks_t& tasks() { return tasks_; }
  std::condition_variable& condition() { return condition_; }
  std::mutex &req_mutex() { return req_mutex_; }
  const std::string& root_path() const { return root_path_; }
  // sig from handlers
  void on_handler_exit(int aId);
  void on_request_error(int aConnection);
  void on_request_end(int aConnection);

private:
  void clear_all();
  void add_handler();
  void remove_handler(int aId);
  void push_request(int aReq);

private:
  bool running_;
  handlers_t handlers_;
  tasks_t tasks_;
  std::string root_path_;
  std::map<int, std::shared_ptr<CHandler> > workers_;
  std::set<int> connections_;
  // synchronization
  std::mutex req_mutex_;
  std::condition_variable condition_;
};

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //

#endif // CSERVER_H
