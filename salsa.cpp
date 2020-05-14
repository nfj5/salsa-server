/**
 * Nicholas Jones
 * CPSC5510 - SQ20
 * Professor Lillethun
 * Project1 - HTTP Server
 */

#include <iostream>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <time.h>
#include <unistd.h>
#include <cstdio>
#include <array>
#include <memory>
#include "NetworkHelper.h"
using namespace std;

map<string, string> MimeType = {
	{".txt", "text/plain"},
	{".html", "text/html"},
	{".htm", "text/htm"},
    {".css", "text/css"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".png", "image/png"},
    {".php", "text/html"}
};

// for handling the network methods


string date_string(tm* t) {
    char buffer[80];
    strftime(buffer, 80, "%a, %d %h %G %T %Z", t);
    string temp = buffer;
    return temp;
}

bool get_file(string &path, string &content) {
    struct stat file_info;
    string temp_path;
    if (stat(path.c_str(), &file_info) != 0)
        return false;

    if (S_ISDIR(file_info.st_mode)){
        if (path.substr(path.length()-1, path.length()-2) != "/")
            path += "/";
        path += "index.html";

        return get_file(path, content);
    }

    ifstream file(path, ios::binary);
    content.assign(istreambuf_iterator<char>(file), istreambuf_iterator<char>());
    file.close();

    return true;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        cerr << "Usage: salsa [port]" << endl;
        return 1;
    }

    int port = strtol(argv[1], NULL, 0);

    // create the socket
    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "Unable to create socket" << endl;
        return 1;
    }

    // bind the port
    struct sockaddr_in srv_addr;
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) < 0) {
        cerr << "Unable to bind to the specified port. Exiting..." << endl;
        exit(1);
    }

    // set the queue length
    listen(sock, 5);

    cout << "SalsaServer listening on port " << port << endl;

    // handle incoming connections
    struct sockaddr_in cli_addr;
    int client_sock;
    socklen_t client_len;
    
    while (true) {
        client_len = sizeof(cli_addr);
        client_sock = accept(sock, (struct sockaddr *) &cli_addr, &client_len);

        // read the request
        string requestHeader, requestBody;
        NetworkHelper::getContent(client_sock, requestHeader, requestBody);

        // parse the request header into an object
        NetworkHelper::HTTPHeader request;
        NetworkHelper::parseRequestHeader(requestHeader, request);

        cout << request.method << " " << request.path << endl;
        string body = "";

        // get the GMT date as a string
        time_t now = time(0);
        tm* gmt = gmtime(&now);
        string date = date_string(gmt);

        // create the default response object
        NetworkHelper::HTTPHeader response;
        response.type = NetworkHelper::HTTPHeader::HeaderType::RESPONSE;
        response.code = 200;
        response.fields.insert({"Connection", "close"});
        response.fields.insert({"Date", date});

        string local_path = "./web_root" + request.path;

        // get the file content, but also indicate whether it exists or not
        string file_content;
        bool fileExists = get_file(local_path, file_content);

        // only allow GET requests
        if (request.method != "GET")
            response.code = 501;

        // do not allow directory manipulation
        else if (request.path.find("/../") != string::npos)
            response.code = 400;

        else if (!fileExists) {
            response.code = 404;
        }

        else {
            int typeIndex = local_path.rfind(".");
            string fileType = local_path.substr(typeIndex);

            if (MimeType.find(fileType) == MimeType.end())
                response.code = 501;
            else {
                response.fields.insert({"Content-Type", MimeType[fileType]});
                body = file_content;

                if (fileType == ".php") {
                    body = "";
                    array<char, 128> buffer;
                    string cmd = "php-cgi -q " + local_path;
                    unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);

                    while(fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                        body += buffer.data();
                    }
                }
            }
        }

        // construct the response
        response.fields.insert({"Content-Length", to_string(body.length())});

        // send the response
        NetworkHelper::sendResponse(client_sock, response.toString(), body);

        close(client_sock);
    }

    return 0;
}
