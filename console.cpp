#include "console.h"


using boost::asio::ip::tcp;

void controller::setRequest(int id_, data_t* req){
    user_t userData = htmlGen::getInstance().userTable[id_];
    req[0] = 0x04;
    req[1] = 0x01;
    req[2] = std::stoi(userData.port)/256;
    req[3] = std::stoi(userData.port)%256;
    req[4] = 0x00;
    req[5] = 0x00;
    req[6] = 0x00;
    req[7] = 0x01;
    req[8] = 0x00;
    for (int i = 0; i < (int)userData.url.length(); i++){
        req[9 + i] = (unsigned char)htmlGen::getInstance().SOCKS_IP[i];
    }
    req[9 + userData.url.length()] = 0x00;
}

void controller::start(){
    this->do_resolve();
}

void controller::do_resolve(){
    auto self(shared_from_this());
    resolver.async_resolve(htmlGen::getInstance().SOCKS_IP, htmlGen::getInstance().SOCKS_PORT,
        [this, self](const boost::system::error_code &ec,
            const boost::asio::ip::tcp::resolver::results_type results){
            if(!ec){
                endpoints = results;
                do_connect();
            }else{
                socket_.close();
            }
        }
    );
}

void controller::do_connect(){
    auto self(shared_from_this());
            boost::asio::async_connect(socket_, endpoints,
                [this, self](const boost::system::error_code &ec, tcp::endpoint ed){
                if(!ec){
                    setRequest(std::stoi(id), sock_buf);
                    sendRequest();
                }else{
                    socket_.close();
                }
            });
}

void controller::sendRequest(){
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(sock_buf, sizeof(data_t)*(10 + htmlGen::getInstance().SOCKS_IP.length())),
        [this, self](boost::system::error_code ec, std::size_t /*length*/){
        if(!ec){
            read_reply();
        }else{
            socket_.close();
        }
    });
}

void controller::read_reply(){
    auto self(shared_from_this());
    clearBuffer(sock_buf);
    socket_.async_read_some(boost::asio::buffer(sock_buf, max_length),
                [this, self](boost::system::error_code ec, std::size_t length){
                    if (!ec){
                        user_t userData = htmlGen::getInstance().userTable[std::stoi(id)];
                        if (sock_buf[0] == 0x00 && sock_buf[1] == 0x5a){
                            std::string path = "./test_case/" + userData.file;
                            fin.open(path.data());
                            do_read();
                        } else {
                            socket_.close();
                        }
                    } else {
                        socket_.close();
                    }
                }
            );
        }

void controller::do_write(std::string command){
    auto self(shared_from_this());
            const char *msg = command.c_str();
            /* send command to the shell */
            boost::asio::async_write(socket_, boost::asio::buffer(msg, sizeof(char)*command.length()),
                [this, self, command](boost::system::error_code ec, std::size_t /*length*/){
                if (!ec){
                    std::string strCmp ("exit\n");
                    if(command.compare(strCmp) == 0){
                        socket_.close();
                    }else{
                        do_read();
                    }
                }
            });
}

void controller::do_read(){
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
    [this, self](boost::system::error_code ec, std::size_t length){
        if(!ec){
            data2Msg(data_, msg, length);
            clearBuffer(data_);
            htmlGen::getInstance().sendMsg(this->id, msg, false);
            if(length != 0){
                /* has prompt */
                if(msg.find('%', 0) != std::string::npos){
                    std::string command;
                    getline(fin, command);
                    command += "\n";
                    htmlGen::getInstance().sendMsg(this->id, command, true);
                    do_write(command);
                }else{
                    do_read();
                }
            }
        }else{
            socket_.close();
        }
    });
}


int main(int argc, char* argv[]){
    htmlGen::getInstance().do_parseString();
    htmlGen::getInstance().sendHtmlFrame();
    try{
        boost::asio::io_context io_context;
        for(int i = 0; i < USER_NUM; i++){
            user_t* userTable = htmlGen::getInstance().userTable;
            if(userTable[i].url == ""){
                break;
            }else{
                htmlGen::getInstance().sendHtmlTable(std::to_string(i), (userTable[i].url + ":" + userTable[i].port));
                std::make_shared<controller>(io_context, std::to_string(i))->start();
            }
        }

        io_context.run();
    }
    catch(std::exception& e){
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}