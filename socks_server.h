#ifndef SERVER_H
#define SERVER_H

#define BIND_PORT 27219
#define data_t unsigned char
#define buffer_clear(target) bzero(target, sizeof(target))

#include <utility>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <regex>

using boost::asio::ip::tcp;

boost::asio::io_context io_context;

typedef struct{
  data_t mode;
  std::string rule;
}fw_config_t;

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
  connection(boost::asio::io_context& io_context,
  tcp::socket socket_, sock4_t requestedContent)
  :client_sock(std::move(socket_)),
  server_sock(io_context),
  req(requestedContent),
  io_context(io_context),
  resolver(io_context),
  acceptor(io_context, tcp::endpoint(tcp::v4(), 0)){}
  void start();
private:
  std::vector<fw_config_t> config_vector;
  void fw_config_setter();
  bool fw_config_getter(data_t CD, std::string URL);
  void do_resolve();
  void do_connect();
  void do_accept();
  void set_req(bool isAccept);
  void do_write_up(std::size_t length);
  void do_read_up();
  void do_write_dl(std::size_t length);
  void do_read_dl();
  void do_reply(bool cond);

  tcp::socket client_sock, server_sock;
  sock4_t req;
  boost::asio::io_context& io_context;
  tcp::resolver resolver;
  tcp::acceptor acceptor;
  tcp::resolver::results_type endpoint;
  enum { max_length = 1024 };
  data_t uplink[max_length];
  data_t downlink[max_length];
  short port = 27219;
};

class session:public std::enable_shared_from_this<session>{
public:
    session(tcp::socket socket):socket_(std::move(socket)){}
    void start();
private:
    void do_read();
    void request_parser(unsigned char* data, sock4_t &requestedContent);

  tcp::socket socket_;
  enum { max_length = 1024 };
  data_t data_[max_length];
};

class server{
private:
  void do_accept();
  tcp::acceptor acceptor_;
  tcp::socket socket;
public:
  server(boost::asio::io_context& io_context, short port):acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
  socket(io_context){
    do_accept();
  }
};

#endif