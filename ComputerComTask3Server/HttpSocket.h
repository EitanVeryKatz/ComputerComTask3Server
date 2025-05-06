#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <string.h>
#include <vector>

#define BAD_REQUEST 400
#define NOT_FULLY_PROCCESED 0


#define MAX_LINE_LENGTH 1024
#define MAX_HEADERS 50

class HttpSocket
{
public:
	SOCKET id;
	char verb[16];
	char requestUrl[512];
	vector<char*> headers;
	char body[MAX_LINE_LENGTH];
	int	recv;
	int	send;
	char buffer[2048];
	int len;
	int statusCode;

	int ParseHttpRequest() {
		char* request = buffer;
		char temp[MAX_LINE_LENGTH];
		strncpy(temp, request, MAX_LINE_LENGTH - 1);
		temp[MAX_LINE_LENGTH - 1] = '\0';


		char* line = strtok(temp, "\r\n");//get request line
		if (!line)
			return BAD_REQUEST;

		char* method = strtok(line, " ");
		char* url = strtok(NULL, " ");
		char* httpVersion = strtok(NULL, " ");

		if (!method || !url || !httpVersion) return BAD_REQUEST;

		if (!checkVerbValid(method)) return BAD_REQUEST;
		if (!checkUrlVaild(url)) return BAD_REQUEST;

		strncpy(verb, method, sizeof(verb));
		strncpy(requestUrl, url, sizeof(requestUrl));//fill all fields if valid request



		char* headersStr = strtok(NULL, "\r\n\r\n");
		char* header = strtok(headersStr, "\r\n");
		while (header != NULL) {
			headers.push_back(header);
			header = strtok(NULL, "\r\n");
		}
		//get all headers seperatly in vector

		char* bodyStart = strstr(request, "\r\n\r\n");
		if (bodyStart) {
			bodyStart += 4;
			strncpy(body, bodyStart, sizeof(body));
		}
		else {
			return BAD_REQUEST;
			//no appearance of "\r\n\r\n"
		}

		return NOT_FULLY_PROCCESED;
	}

	char* processRequest() {
		if (verb == "GET") {
			return Get();
		}
		else if (verb == "POST") {
			return Post();
		}
		else if (verb == "DELETE") {
			return Delete();
		}
		else if (verb == "PUT") {
			return Put();
		}
		else if (verb == "OPTIONS") {
			return Options();
		}
		else if (verb == "HEAD") {
			return Head();
		}
		else if (verb == "TRACE") {
			return Trace();
		}
		else {
			throw("HTTP method not supported");
		}
	}

private:

	bool checkVerbValid(const char* method) {
		return strcmp(method, "GET") == 0 || strcmp(method, "POST") == 0 ||
			strcmp(method, "PUT") == 0 || strcmp(method, "DELETE") == 0 ||
			strcmp(method, "HEAD") == 0 || strcmp(method, "OPTIONS") == 0 || strcmp(method, "TRACE") == 0;
	}

	bool checkUrlVaild(const char* url) {
		return url[0] == '/';

	}

	char* getQueryParamsFromUrl() {
		return strstr(requestUrl, "?");
	}

	

	char* Get();
	char* Post();
	char* Delete();
	char* Put();
	char* Options();
	char* Head();
	char* Trace();
};