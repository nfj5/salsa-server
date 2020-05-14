#pragma once

#include <string>
#include <map>
class NetworkHelper {
public:
    // for holding the parsed HTTP header information
    struct HTTPHeader {
        enum HeaderType {
            REQUEST,
            RESPONSE
        };

        // keep track of request or response
        HeaderType type;

        std::string method; 
        std::string path;
        std::string version = "HTTP/1.1";

        // for response headers
        int code;

        // code to std::string mappings
        std::map<int, std::string> CodeStrings = {
            {200, "OK"},
            {400, "Bad Request"},
            {500, "Internal Server Error"},
            {501, "Not Implemented"}
        };

        // other fields
        std::map<std::string, std::string> fields;

        std::string toString() {
            std::string result;
            if (type == REQUEST)
                result = method + " " + path + " " + version + "\r\n";

            if (type == RESPONSE)
                result = version + " " + std::to_string(code) + " " + CodeStrings[code] + "\r\n";

            std::map<std::string, std::string>::iterator it;
            for (it = fields.begin(); it != fields.end(); ++it) {
                result += it->first + ": " + it->second + "\r\n";
            }

            result += "\r\n";
            return result;
        }
    };

	// receive an HTTP header and body separately
	static void getContent(int socket, std::string &header, std::string &body);

    // parse an HTTP request header into its parts
    static void parseRequestHeader(std::string headerText, HTTPHeader &header);

    // send an HTTP response
    static void sendResponse(int socket, std::string header, std::string body);

    // to handle an error and close socket
    static void ConnectionError(int socket, std::string message);
protected:
	static const int BUFFER_SIZE = 4096;
};