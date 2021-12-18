#ifndef SERVER_H
#define SERVER_H

#define BIND_PORT 27219
#define data_t unsigned char;

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
  data_t VN;
  data_t CD;
  unsigned int DSTPORT;
  std::string DSTIP;
  std::string URL;
  data_t* USERID;
  int IDLEN;
  data_t* DOMAIN_NAME;
  int DOMAIN_LEN;
}sock4_t;

class connection:public std::enable_shared_from_this<connection>{
public: 
  connection(tcp::socket socket_, sock4_t requestedContent)
  :socket(socket_),
  :req(requestedContent),
  acceptor(ioservice, tcp::endpoint(ip::tcp::v4(), 0)){}
private:
  tcp::socket client_sock, server_sock;
  sock4_t req;
  tcp::acceptor acceptor;
  data_t uplink[max_length];
  data_t downlink[max_length];
}

class session:public std::enable_shared_from_this<session>{
public:
    session(tcp::socket socket):socket(std::move(socket)){}
    void start();
private:
    void do_read();
    void request_parser(unsigned char* data, sock4_t &requestedContent);

  tcp::socket socket_;
  tcp::resolver resolver;
  enum { max_length = 1024 };
  char data_[max_length];
};

class server
{
public:
  server(boost::asio::io_context& my_io_context, short port):acceptor_(my_io_context, tcp::endpoint(tcp::v4(), port)){
    do_accept();
  }

private:

  void do_accept();

  tcp::acceptor acceptor_;
};

#endif