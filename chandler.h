#ifndef CHANDLER_H
#define CHANDLER_H
#include <fstream>
#include <sstream>

namespace  shvilek {

class CServer;

class CHandler {
public:
    enum request_state {
        rs_initial_line_method,
        rs_initial_line_path,
        rs_initial_line_version,
        rs_headers,
        rs_body
    };
  ~CHandler() {log_.close();}
  CHandler(CServer& aOwner, int aId);
  void run();
  void process_request(int aConnection);

private:
  void write_log(const std::string& aStr);
    int send_data(int aConnection, const char* aData, int aSize);
    CServer& owner_;
  int id_;
  std::ofstream log_;
};

}

#endif // CHANDLER_H
