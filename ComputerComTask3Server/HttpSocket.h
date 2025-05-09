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
#define OK 200
#define NOT_FOUND 404
#define CREATED 201

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
	
	bool checkValidQuery(char* query);

	void BuildHttpResponse(const char* content);

	int ParseHttpRequest();

	void processRequest() {
		if (strcmp(verb, "GET") == 0) {
			Get();
		}
		else if (strcmp(verb, "POST") == 0) {
			Post();
		}
		else if (strcmp(verb, "DELETE") == 0) {
			Delete();
		}
		else if (strcmp(verb, "PUT") == 0) {
			Put();
		}
		else if (strcmp(verb, "OPTIONS") == 0) {
			Options();
		}
		else if (strcmp(verb, "HEAD") == 0) {
			Head();
		}
		else if (strcmp(verb, "TRACE") == 0) {
			Trace();
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

	std::string getFilePathFromUrl(const char* url);

	void Get();
	void Post();
	void Delete();
	void Put();
	void Options();
	void Head();
	void Trace();
};