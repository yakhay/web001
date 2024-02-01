/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   getMethod.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yakhay <yakhay@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/12/26 18:40:51 by wbouwach          #+#    #+#             */
/*   Updated: 2024/02/01 14:03:06 by yakhay           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "getMethod.hpp"
#include <fstream>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
bool isDirectory(const std::string& path) {
    DIR* dir = opendir(path.c_str());
    if (dir != nullptr) {
        closedir(dir);
        return true;
    }
    return false;
}

// #include "one.hpp"
// #include "../config_file/server.hpp"

// void send_not_found_respons(int newsockfd) {
//     char resp[] = "HTTP/1.0 404 Not Found\r\n"
//                   "Server: webserver-c\r\n"
//                   "Content-Length: 25\r\n"
//                   "Content-type: text/html\r\n\r\n"
//                   "<html><h1>404 Not Found</h1></html>\r\n"; 
//     int valwrite = send(newsockfd, resp, strlen(resp), 0);

//     if (valwrite < 0) {
//         perror("webserver (write) failed");
//     }
// }

std::string determine_content(const std::string& fileExtension)
{
    return (mimetype.get(fileExtension, 1));
}

size_t  max = 0;
size_t size = 0;
int bytesRead = 1;

void send_forbidden_response(int newsockfd, cl& client) {
    const char resp[] = "HTTP/1.1 403 Forbidden\r\n"
                        "Server: webserver-c\r\n"
                        "Content-Length: 25\r\n"
                        "Content-type: text/html\r\n\r\n"
                        "<html><h1> 403 Forbidden </h1></html>\r\n";
    send(newsockfd, resp, strlen(resp), 0);
    client.end_send = 1;
}

void send_not_found_response(int newsockfd, cl& client) {
    char resp[] = "HTTP/1.0 404 Not Found\r\n"
                  "Server: webserver-c\r\n"
                  "Content-Length: 25\r\n"
                  "Content-type: text/html\r\n\r\n"
                  "<html><h1> 404 Not Found </h1></html>\r\n"; 
    int valwrite = send(newsockfd, resp, strlen(resp), 0);

    if (valwrite < 0) {
        perror("webserver (write) failed");
    }
    client.end_send = 1;
}


void send_response_200(const std::string &filename, const std::string &contentType, int newsockfd, cl& client) {
    if (client.test2 == 0)
    {

        client.fileStream.open(filename, std::ios::binary);
        if (!client.fileStream.is_open()) {
            send_not_found_response(newsockfd, client);
            return;
        }

        struct stat statbuf;
        if (stat(filename.c_str(), &statbuf) == -1) {
            std::cerr << "Error getting file information." << std::endl;
            send_not_found_response(newsockfd, client);
            return;
        }

        size_t size = statbuf.st_size;
        std::cout << "path: " << filename << std::endl;

        std::string response = "HTTP/1.1 200 OK\r\nServer: webserver-c\r\nContent-Length: " + std::to_string(size) + "\r\nContent-type: " + contentType + "\r\n\r\n";
        std::cout << response << std::endl;
        int dd = send(newsockfd, response.c_str(), response.size(), 0);

        if (dd < 0) {
            perror("webserver (write) failed");
            client.fileStream.close();
            return;
        }
        client.test2 = 1;
        std::cout<<"send headers"<<std::endl;
    }
    else
    {
        std::cout<<"read from file"<<std::endl;
        const size_t buffer_size = 1024;
        std::vector<char> buffer(buffer_size, 0);
        client.fileStream.read(buffer.data(), buffer_size);
        int valwrite = send(newsockfd, buffer.data(), client.fileStream.gcount(), 0);
        if (valwrite < 0) {
            perror("webserver (write) failed");
            client.fileStream.close();
            return;
        }
    }
    if (client.fileStream.eof())
    {
        std::cout<<"hannnnannnana"<<std::endl;
        client.end_send = 1;
        if (filename == "/tmp/listing.html")
            std::remove("/tmp/listing.html");
    }
}

void get_method(request &req, manyServer *server, int newsockfd, int idx, cl& client) {
    std::string new_path;
    std::string contentType;
    Location    location;
    location = server->_name_server[idx].get_location(req.get_path().substr(0, req.get_path().find("?")));
    if (find(location._allow_methods.cbegin(), location._allow_methods.cend(), "GET") == location._allow_methods.cend())
        throw (405);
    new_path = server->_name_server[idx].get_path(req.get_path().substr(0, req.get_path().find("?")));

    if (location._return) {
        if (location.cgi_status == 0)
        {
            std::string ret = location._return;
            contentType = determine_content(ret.substr(ret.find_last_of(".") + 1));
            send_response_200(ret, contentType, newsockfd, client);
            return;
        }
        else
        {
            if (client.test3 == 0)
            {
                std::cout<<"RUN CGI FROM GET1"<<std::endl;
                new_path = location._return;
                if ((new_path.find_last_of('.') != std::string::npos) && !(location.get_cgi(new_path.substr(new_path.find_last_of('.'))).empty()))
                {
                    std::cout<<"RUN CGI FROM GET2"<<std::endl;
                    std::string msg;
                
                    client.req.set_cgi_script(new_path);
                    msg = client.cgi.cgi_run(server->_name_server[idx], client, "");
                    std::cout<<"mst ::::::"<<msg<<"$\n";
                    write(newsockfd, msg.c_str(), msg.length());
                }
                else
                {
                    contentType = determine_content(new_path.substr(new_path.find_last_of(".") + 1));
                    send_response_200(new_path, contentType, newsockfd, client);
                }
                client.test3 = 1;
                return ;
            }
            std::string ret = "temp_output.html";
            contentType = determine_content(ret.substr(ret.find_last_of(".") + 1));
            send_response_200("temp_output.html", contentType, newsockfd, client);
        }
    }

    if (!isDirectory(new_path)) {
        if(location.cgi_status == 0)
        {
            contentType = determine_content(new_path.substr(new_path.find_last_of(".") + 1));
            std::cout <<"zzzzzzzf:::" <<contentType << std::endl;
            send_response_200(new_path, contentType, newsockfd, client);
        }
        else
        {
            std::cout<<"ffffffffff:"<<location.get_cgi(new_path.substr(new_path.find_last_of('.')))<<"$"<<std::endl;
            std::cout<<"new_path::"<<new_path<<"\n";
            if ((new_path.find_last_of('.') != std::string::npos) && !(location.get_cgi(new_path.substr(new_path.find_last_of('.'))).empty()))
            {
                std::cout<<"RUN CGI FROM GET2"<<std::endl;
                std::string msg;
            
                client.req.set_cgi_script(new_path);
                msg = client.cgi.cgi_run(server->_name_server[idx], client, "");
                std::cout<<"mst ::::::"<<msg<<"$\n";
                write(newsockfd, msg.c_str(), msg.length());
            }
            else
            {
                contentType = determine_content(new_path.substr(new_path.find_last_of(".") + 1));
                send_response_200(new_path, contentType, newsockfd, client);
            }
            // return ;
            // ##run cgi; new_path
        }
         }
        else if (location.index) {
            std::string index = (location.index);
            new_path = new_path + "/" + index;
                if (location.cgi_status == 1)
                {
                    std::cout<<"RUN CGI FROM GET3"<<std::endl;
                    if ((new_path.find_last_of('.') != std::string::npos) && !(location.get_cgi(new_path.substr(new_path.find_last_of('.'))).empty()))
                {
                    std::cout<<"RUN CGI FROM GET2"<<std::endl;
                    std::string msg;
                
                    client.req.set_cgi_script(new_path);
                    msg = client.cgi.cgi_run(server->_name_server[idx], client, "");
                    std::cout<<"mst ::::::"<<msg<<"$\n";
                    write(newsockfd, msg.c_str(), msg.length());
                }
                else
                {
                    contentType = determine_content(new_path.substr(new_path.find_last_of(".") + 1));
                    send_response_200(new_path, contentType, newsockfd, client);
                }
                return ;
                //##run cgi new_path +"/" + index
            } else {
                contentType = determine_content(index.substr(index.find_last_of(".") + 1));
                send_response_200(new_path +"/" + index, contentType, newsockfd, client);
        }
    }
    else if (location._autoindex == 1) {
        if (client.test3 == 0) {
            
            std::fstream outputFile("/tmp/listing.html", std::ios::in | std::ios::out | std::ios::app);
            outputFile.seekp(0, std::ios::beg);
            outputFile << "";
            if (!outputFile.is_open()) {
                std::cerr << "Error opening the file: " << std::endl;
                return;
            }
            client.test3 = 1;

            std::string buffer = "<html><head>\n<title>Directory Listing</title></head><body>\n<h1>Directory Listing</h1><ul>\n";
            DIR *dir = opendir(new_path.c_str());

            if (dir == nullptr) {
                std::cerr << "Error opening directory." << std::endl;
                return;
            }
            struct dirent *entry;
            while ((entry = readdir(dir)) != nullptr) {
                if (entry->d_type == DT_REG) {
                    std::string filename = entry->d_name;
                    if (buffer.find(entry->d_name) == std::string::npos)
                        buffer = buffer + "<li> <a href=\"" + req.get_path() + "/" + filename + "\">" + filename + "</a></li> \n";
                    else
                        break;
                }
            }
            outputFile << buffer + "</ul></body></html>\r\n";
            outputFile.close();
            closedir(dir);
            contentType = "text/html"; 
        }
        send_response_200("/tmp/listing.html", contentType, newsockfd, client);
    } 
    else {
            char resp[] = "HTTP/1.1 403 forbidden\r\n"
                          "Server: webserver-c\r\n"
                          "Content-Length: 25\r\n"
                          "Content-type: text/html\r\n\r\n"
                          "<html><h1> 403 forbidden </h1></html>\r\n";
            int valwrite = send(newsockfd, resp, strlen(resp), 0);

            if (valwrite < 0) {
                perror("webserver (write) failed");
            }
            client.end_send = 1;
    }
}
