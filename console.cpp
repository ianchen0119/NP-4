#include "console.h"


using boost::asio::ip::tcp;


void controller::start(){
    this->do_resolve();
}

void controller::do_resolve(){
    user_t userData;
    userData = htmlGen::getInstance().userTable[std::stoi(id)];
    auto self(shared_from_this());
    resolver.async_resolve(userData.url, userData.port,
        [this, self](const boost::system::error_code &ec,
            const boost::asio::ip::tcp::resolver::results_type results){
            if(!ec){
                clearBuffer(data_);
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
                    clearBuffer(data_);
                    user_t* userTable = htmlGen::getInstance().userTable;
                    /* open test file as ifstream */
                    std::string path = "./test_case/" + userTable[std::stoi(id)].file;
                    fin.open(path.data());
                    do_read();
                }else{
                    socket_.close();
                }
            });
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