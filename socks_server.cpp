#include "socks_server.h"

using boost::asio::ip::tcp;

void connection::start(){
    fw_config_setter();
    if(req.CD == 0x01){
        /* connect */
        do_resolve();
    }else if(req.CD == 0x02){
        /* bind */
        bool solved = false;
        while (!solved){
            try{
                acceptor = std::move(tcp::acceptor(io_context, tcp::endpoint(tcp::v4(), this->port)));
                solved = true;
            }catch (std::exception& e){
                this->port++;
            }
        }
        set_req(true);
        do_reply(false);
        do_accept();
    }
}

void connection::do_resolve(){
    auto self(shared_from_this());
    resolver.async_resolve(req.URL, std::to_string(req.DSTPORT),
        [this, self](const boost::system::error_code &ec,
            const boost::asio::ip::tcp::resolver::results_type results){
            if(!ec){
                endpoint = results;
                do_connect();
            }else{
                server_sock.close();
                client_sock.close();
            }
        }
    );
}

void connection::do_connect(){
    auto self(shared_from_this());
        boost::asio::async_connect(server_sock, endpoint,
            [this, self](const boost::system::error_code &ec, tcp::endpoint ed){
                if (!ec){
                    req.URL = server_sock.remote_endpoint().address().to_string();
                    set_req(true);
                    do_reply(true);
                } else {
                    set_req(false);
                    do_reply(false);
                    server_sock.close();
                    client_sock.close();
                }
            }
        );
}

void connection::do_accept(){
    auto self(shared_from_this());
    acceptor.async_accept(
        [this, self](boost::system::error_code ec, tcp::socket socket){
            if (!ec){
                server_sock = std::move(socket);
                (this->acceptor).close();
                do_reply(true);
            } else {
                client_sock.close();
            }
        }
    );
}

void connection::do_write_up(std::size_t length){
    auto self(shared_from_this());
    boost::asio::async_write(server_sock, boost::asio::buffer(uplink, length),
        [this, self](boost::system::error_code ec, std::size_t /*length*/){
            if (!ec){
                do_read_up();
            } else {
                client_sock.close();
                server_sock.close();
            }
        }
    );
}

void connection::do_write_dl(std::size_t length){
    auto self(shared_from_this());
    boost::asio::async_write(client_sock, boost::asio::buffer(downlink, length),
        [this, self](boost::system::error_code ec, std::size_t /*length*/){
            if (!ec){
                do_read_dl();
            } else {
                client_sock.close();
                server_sock.close();
            }
        }
    );
}

void connection::do_read_up(){
    auto self(shared_from_this());
    buffer_clear(uplink);
    client_sock.async_read_some(boost::asio::buffer(uplink, max_length),
        [this, self](boost::system::error_code ec, std::size_t length){
            if (!ec){
                do_write_up(length);
            } else {
                client_sock.close();
                server_sock.close();
            }
        }
    );
}

void connection::do_read_dl(){
    auto self(shared_from_this());
    buffer_clear(downlink);
    server_sock.async_read_some(boost::asio::buffer(downlink, max_length),
        [this, self](boost::system::error_code ec, std::size_t length){
            if (!ec){
                do_write_dl(length);
            } else {
                client_sock.close();
                server_sock.close();
            }
        }
    );
}

void connection::fw_config_setter(){
    std::string line;
    std::ifstream config;
    config.open("./socks.conf");
    while(getline(config, line)){
        fw_config_t new_rule;
        /* mode setting */
        if(line[7] == 'b'){
            new_rule.mode = 0x02;
        }else if(line[7] == 'c'){
            new_rule.mode = 0x01;
        }
        /* parse the ip filter */
        new_rule.rule = "";
        std::string temp_ip = line.substr(9);
        for (int i = 0; i < (int)temp_ip.length(); i++){
            switch (temp_ip[i]){
            case '*':
                new_rule.rule += "[0-9]+";
                break;
            case '.':
                new_rule.rule += "\\.";
                break;
            default:
                new_rule.rule += temp_ip[i];
                break;
            }
        }
        this->config_vector.push_back(new_rule);
    }
}

bool connection::fw_config_getter(data_t CD, std::string URL){
    bool result = false;
    for(fw_config_t item: this->config_vector){
        if(CD == item.mode){
            std::regex new_rule(item.rule);
            if(std::regex_match(URL, new_rule)){
                result = true;
                break;
            } 
        }
    }
    return result;
}

void connection::set_req(bool isAccept){

    std::cout << "<S_IP>: " << client_sock.local_endpoint().address().to_string() << std::endl;
    std::cout << "<S_PORT>: " << std::to_string(htons(client_sock.local_endpoint().port())) << std::endl;
    std::cout << "<D_IP>: " << req.URL << std::endl;
    std::cout << "<D_PORT>: " << req.DSTPORT << std::endl;

    buffer_clear(downlink);
    /* downlink buffer init */
    for(int i = 0; i < 8; i++){
        downlink[i] = 0x00;
    }
    if(isAccept && fw_config_getter(req.CD, req.URL)){
        downlink[1] = 0x5a;
    }else{
        downlink[1] = 0x5b;
    }
    downlink[2] = port/256;
    downlink[3] = port%256;

    if(req.CD == 0x01){
        std::cout << "<Command>: CONNECT" << std::endl;
    }else if(req.CD == 0x02){
        std::cout << "<Command>: BIND" << std::endl;
    }else{
        std::cout << "<Command>: UNKNOWN -- CD = " << (int)req.CD << std::endl;
    }

    std::string reply = (downlink[1] == 0x5a)?("Accept"):("Reject");
    std::cout << "<Reply>: " << reply << std::endl;
    std::cout << "-------------------------------------" << std::endl;
}

void connection::do_reply(bool cond){
    auto self(shared_from_this());
    boost::asio::async_write(client_sock, boost::asio::buffer(downlink, sizeof(data_t) * 8),
        [this, self, cond](boost::system::error_code ec, std::size_t /*length*/){
            if(!ec){
                if((this->downlink)[1] == 0x5a){
                    if(cond){
                        do_read_up();
                        do_read_dl();
                    }
                }else{
                    client_sock.close();
                    server_sock.close();
                }
            }else{
                client_sock.close();
                server_sock.close();
            }
        }
    );
}

void session::start(){
    this->do_read();
}

void session::do_read(){
    auto self(shared_from_this());
    buffer_clear(data_);
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
    [this, self](boost::system::error_code ec, std::size_t length){
        if(!ec){
            sock4_t requestedContent;
            request_parser(data_, requestedContent);
            std::make_shared<connection>(io_context, std::move(socket_), requestedContent)->start();
            io_context.run();
        }else{
            socket_.close();
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
            req.DSTIP += ".";
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
        req.URL = req.DSTIP;
    }
}

void server::do_accept(){
    acceptor_.async_accept(socket,
    [this](boost::system::error_code ec){
        if(!ec){
            /* if no error occur, server trans the ownership of socket to session */
            // ref: https://www.boost.org/doc/libs/1_47_0/doc/html/boost_asio/reference/io_context/notify_fork.html
            io_context.notify_fork(boost::asio::io_context::fork_prepare);
            if(fork() == 0){
                /* Child Process */
                io_context.notify_fork(boost::asio::io_context::fork_child);
                acceptor_.close();
                std::make_shared<session>(std::move(socket))->start();
            }else{
                io_context.notify_fork(boost::asio::io_context::fork_parent);
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
        server s(io_context, std::atoi(argv[1]));
        io_context.run();
    }
    catch(std::exception& e){
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}