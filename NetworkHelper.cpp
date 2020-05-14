#include <iostream>
#include <string>
#include <sys/socket.h>
#include <cstdio>
#include <sstream>
#include <vector>
#include <cstring>
#include "NetworkHelper.h"
using namespace std;

void NetworkHelper::getContent(int socket, string &header, string &body) {
    char buffer[BUFFER_SIZE];
    string headerDelimiter = "\r\n\r\n";

    bool receivedHeader = false;
    int headerEnd, contentLength = 1, currentBytes;

    header = "";
    body = "";

	// until we've read the entire specified content length
    while ((int) body.length() < contentLength) {
    	if ((currentBytes = recv(socket, (char*) &buffer, BUFFER_SIZE, 0)) < 0) {
    		cerr << "Error when reading from socket" << endl;
    		exit(1);
    	}

	    for (int i=0; i < currentBytes; ++i) {
        	// append to the header until we have the whole thing
        	if (!receivedHeader) header += buffer[i];

        	// otherwise append to the body
        	else body += buffer[i];

			// check for the header end delimiter
        	if ((headerEnd = header.find(headerDelimiter)) != -1) {
        		receivedHeader = true;

        		// remove delimiter from header
        		header = header.substr(0, headerEnd);

        		// get the content length, or throw an error if it's not there
        		int contentLengthPos;
        		if ((contentLengthPos = header.find("Content-Length: ")) == -1 && header.rfind("GET", 0) != 0) {
        			cerr << "Sites not returning the Content-Length header are not yet supported" << endl;
        			exit(1);
        		}

        		string length = header.substr(contentLengthPos+16, header.find("\r\n", contentLengthPos));
        		contentLength = (int) strtol(length.c_str(), NULL, 0);
        	}
        }

        // clear the buffer
        memset(buffer, 0, BUFFER_SIZE);
    }
}

void NetworkHelper::parseRequestHeader(string headerText, HTTPHeader &header) {
    istringstream parser(headerText);
    string line;

    // read the basic fields from the first line
    parser >> header.method;
    parser >> header.path;
    parser >> header.version;
    getline(parser, line);

    // loop through the fields
    string field, val;
    while(getline(parser, line)) {
        istringstream line_parser(line);
        getline(line_parser, field, ':');
        line_parser >> val;
        header.fields.insert({field, val});
    }
}

void NetworkHelper::sendResponse(int socket, string header, string body) {
    string message = header + body;
    vector<char> raw_msg(message.begin(), message.end());

    int bytes_sent = 0;
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));

    // bytes left to copy for this block
    for (int i=0; i<BUFFER_SIZE && bytes_sent+i<(int)raw_msg.size(); ++i) {
        buffer[i] = raw_msg[bytes_sent + i];
    }

    int currentBytes;
    while (bytes_sent < (int) raw_msg.size()) {
    	if ((currentBytes = send(socket, (char*) &buffer, sizeof(buffer), 0)) < 0) {
    		cerr << "Unable to send data over socket" << endl;
    		exit(1);
    	}

        bytes_sent += currentBytes;

        // determine how many bytes are left to copy for the next block, then wipe and write
        memset(buffer, 0, sizeof(buffer));
        for (int i=0; i<BUFFER_SIZE && bytes_sent+i<(int)raw_msg.size(); ++i) {
            buffer[i] = raw_msg[bytes_sent + i];
        }
    }
}