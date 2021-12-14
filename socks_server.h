#ifndef SERVER_H
#define SERVER_H

#include <utility>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

using boost::asio::ip::tcp;

typedef struct{
  unsigned char VN;
  unsigned char CD;
  unsigned int DSTPORT;
  std::string DSTIP;
  unsigned char* USERID;
}sock4_t;

class session:public std::enable_shared_from_this<session>{
public:
    session(tcp::socket socket):socket_(std::move(socket)){}
    void start();
private:
    void do_read();
    void request_parser(std::string requestContent);

  tcp::socket socket_;
  tcp::resolver resolver;
  enum { max_length = 1024 };
  char data_[max_length];
  std::vector< std::pair<std::string, std::string> > envVector;
};

class server
{
public:
  server(boost::asio::io_context& io_context, short port):acceptor_(io_context, tcp::endpoint(tcp::v4(), port)){
    do_accept();
  }

private:

  void do_accept();

  tcp::acceptor acceptor_;
};

#endif