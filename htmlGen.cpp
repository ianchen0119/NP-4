#include "console.h"

htmlGen& htmlGen::getInstance(){
    static htmlGen instance;
    return instance;
}

std::string htmlGen::getQueryString(){
    return getenv("QUERY_STRING");
}

void htmlGen::do_parseString(){
    std::string envVal = this->getQueryString() + "&";
    std::string reqBlock;
    int start, end, index;
    start = 0;
    index = 0;
    while((end = envVal.find('&', start)) != -1){
        reqBlock = envVal.substr(start, end - start);
        if(reqBlock.length() == 3){
            index++;
            continue;
        }

        if(index == 15){
            this->SOCKS_IP = reqBlock.substr(3);
        }else if(index == 16){
            this->SOCKS_PORT = reqBlock.substr(3);
        }else{
            if(index % 3 == 0){
            userTable[index / 3].url = reqBlock.substr(3);
            }else if (index % 3 == 1){
                userTable[index / 3].port = reqBlock.substr(3);
            }else{
                userTable[index / 3].file = reqBlock.substr(3);
            }
        }
        index++;
        start = end + 1;
    }
}

void htmlGen::sendHtmlTable(std::string index, std::string msg){
    std::cout << "<script>document.querySelector('#table_head').innerHTML += '";
    std::cout << "<th scope=\\\"col\\\">" + msg + "</th>" << "';</script>" << std::flush;
    msg = "<td><pre id=\\\"user_" + index + "\\\" class=\\\"mb-0\\\"></pre></td>";
    std::cout << "<script>document.querySelector('#table_body').innerHTML += '" << msg << "';</script>" << std::flush;
}

void htmlGen::sendMsg(std::string index, std::string msg, bool isCommand){
    /* trans shell's output to html */
    std::string parsedContent;
    for(int i = 0; i < (int) msg.length(); i++){
        switch (msg[i]){
        case '\n':
            parsedContent += "<br>";
            break;
        case '\r':
            parsedContent += "";
            break;
        case '\'':
            parsedContent += "\\'";
            break;
        case '<':
            parsedContent += "&lt;";
            break;
        case '>':
            parsedContent += "&gt;";
            break;
        case '&':
            parsedContent += "&amp;";
            break;
        default:
            parsedContent += msg[i];
            break;
        }
    }
    
    if(isCommand){
        /* font family: bold */
        std::cout << "<script>document.querySelector('#user_" + index + "').innerHTML += '<b>" << parsedContent << "</b>';</script>" << std::flush;
    }else{
        /* normal output */
        std::cout << "<script>document.querySelector('#user_" + index + "').innerHTML += '" << parsedContent << "';</script>" << std::flush;
    }
}

void htmlGen::sendHtmlFrame(){
    std::cout << "Content-type: text/html\r\n\r\n";
    std::cout << "\
    <!DOCTYPE html>\
    <html lang=\"en\">\
        <head>\
            <meta charset=\"UTF-8\" />\
            <title>NP Project 3 Console</title>\
            <link\
                rel=\"stylesheet\"\
                href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css\"\
                integrity=\"sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\"\
                crossorigin=\"anonymous\"\
            />\
            <link\
                href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\
                rel=\"stylesheet\"\
            />\
            <link\
                rel=\"icon\"\
                type=\"image/png\"\
                href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\
            />\
            <style>\
            * {\
                font-family: 'Source Code Pro', monospace;\
                font-size: 1rem !important;\
            }\
            body {\
                background-color: #212529;\
            }\
            pre {\
                color: #cccccc;\
            }\
            b {\
                color: #01b468;\
            }\
            </style>\
        </head>\
        <body>\
            <table class=\"table table-dark table-bordered\">\
                <thead>\
                    <tr id=\"table_head\">\
                    </tr>\
                </thead>\
                <tbody>\
                    <tr id=\"table_body\">\
                    </tr>\
                </tbody>\
            </table>\
        </body>\
    </html>" << std::flush;
}