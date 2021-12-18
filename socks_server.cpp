#include "socks_server.h"

using boost::asio::ip::tcp;

boost::asio::io_context my_io_context;

void session::start(){
    this->do_read();
}

void session::do_read(){
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
    [this, self](boost::system::error_code ec, std::size_t length){
        if(!ec){
            sock4_t requestedContent;
            request_parser(data_, requestedContent);
            std::make_shared<connection>(std::move(socket_), requestedContent)->start();
        }else{
            socket.close();
        }
    });
}

void session::request_parser(data_t* data, sock4_t &req){
    req.VN = data[0];
    req.CD = data[1];
    req.DSTPORT = ntohs(*(short *)(data + 2));
    int i = 4;
    for(; i < 8; i++){
        req.DSTIP += std::to_string((int)data[i]);
        if(i != 7){
            req.DSTIP += "."
        }
    }

    req.USERID = data + 8;

    i = 0;
    while (data[8 + i] != 0x00){
        i++;
    }
    req.IDLEN = i;

    if(req.DSTIP == "0.0.0.1"){
        req.DOMAIN_NAME = data + 9 + req.IDLEN;
        i = 0;
        while(data[9 + req.IDLEN + i] != 0x00){
            i++;
        }
        req.DOMAIN_LEN = i;
        req.URL = (char*) req.DOMAIN_NAME;
    }else{
        req.URL = req.DSTTP;
    }
}

void server::do_accept(){
    acceptor_.async_accept(
    [this](boost::system::error_code ec, tcp::socket socket){
        if(!ec){
            /* if no error occur, server trans the ownership of socket to session */
            // ref: https://www.boost.org/doc/libs/1_47_0/doc/html/boost_asio/reference/io_context/notify_fork.html
            my_io_context.notify_fork(boost::asio::io_context::fork_prepare);
            if(fork() == 0){
                /* Child Process */
                my_io_context.notify_fork(boost::asio::io_context::fork_child);
                acceptor_.close();
                std::make_shared<session>(std::move(socket))->start();
            }else{
                my_io_context.notify_fork(boost::asio::io_context::fork_parent);
                socket.close();
                do_accept();
            }
        }else{
            do_accept();
        }        
    });
  }

int main(int argc, char* argv[]){
    
    try{
        if(argc != 2){
            std::cerr << "Usage: socks_server <port>\n";
            return 1;
        }
        // boost::asio::io_context io_context;
        server s(my_io_context, std::atoi(argv[1]));
        my_io_context.run();
    }
    catch(std::exception& e){
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}