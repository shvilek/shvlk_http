#ifndef CHANDLER_H
#define CHANDLER_H
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

  CHandler(CServer& aOwner, int aId);
  void run();
  void process_request(int aConnection);

private:
    int send_data(int aConnection, const char* aData, int aSize);
    CServer& owner_;
  int id_;
};

}

#endif // CHANDLER_H
