#ifndef SERVER_H
#define SERVER_H

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

#define USER_NUM 5
#define data_t unsigned char
#define clearBuffer(data) bzero(data, sizeof(data))
#define data2Msg(sou, des, size) \
sou[size] = '\0'; \
std::string des = sou

using boost::asio::ip::tcp;

typedef struct user{
  std::string url;
  std::string port;
  std::string file;
}user_t;

class controller:public std::enable_shared_from_this<controller>{
public:
    controller(boost::asio::io_context& io_context,
            std::string id)
            :resolver(io_context), 
            socket_(io_context), 
            io_context(io_context), 
            id(id){}
    void start();
private:
    void do_read();
    void do_write(std::string command);
    void do_resolve();
    void do_connect();
    void read_reply();
    void setRequest(int id, data_t* req);
    void sendRequest();

    tcp::resolver resolver;
    tcp::socket socket_;
    boost::asio::io_context& io_context;
    std::string id;
    std::ifstream fin;
    boost::asio::ip::tcp::resolver::results_type endpoints;
    enum { max_length = 4096 };
    char data_[max_length];
    data_t sock_buf[max_length];
};

class htmlGen{
  public:
    std::string SOCKS_IP, SOCKS_PORT;
    /* parse QUERY_STRING into userData */
    void do_parseString();
    std::string getQueryString();
    static htmlGen& getInstance();
    /* content-type + head + body */
    void sendHtmlFrame();
    void sendHtmlTable(std::string index, std::string msg);
    /* prompt or command? */
    void sendMsg(std::string index, std::string msg, bool isCommand);
    user_t userTable[USER_NUM];
  private:
    htmlGen() = default;
    ~htmlGen() = default;
};

#endif