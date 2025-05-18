#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <vector>

#define BAD_REQUEST 400
#define NOT_FULLY_PROCCESED 0
#define OK 200
#define NOT_FOUND 404
#define NOT_ACCEPTABLE 406
#define IM_A_TEAPOT 418

#define TEAPOT "I'm a teapot"

#define MAX_BODY_SIZE 1400000
#define MAX_LINE_LENGTH 8192
#define MAX_HEADERS 50
#define NOT_FOUND_MSG "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n"
#define BAD_REQUEST_MSG "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n"
#define OK_EMPTY_MSG "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n"
#define OK_FORMAT_MSG "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %zu\r\n\r\n"
#define OK_FORMAT_MSG_IMG "HTTP/1.1 200 OK\r\nContent-Type: image/png\r\nContent-Length: %zu\r\n\r\n"
#define OPTIONS_MSG "HTTP/1.1 200 OK\r\nAllow: GET, POST, PUT, DELETE, OPTIONS, HEAD, TRACE\r\nContent-Length: 0\r\n\r\n"
#define NOT_ACCEPTABLE_MSG "HTTP/1.1 406 Not Acceptable\r\nContent-Type: text/plain\r\nContent-Length: 20\r\n\r\n406 Not Acceptable\r\n"
#define IM_A_TEAPOT_MSG "HTTP/1.1 418 I'm a teapot\r\nContent-Type: text/plain\r\nContent-Length: 15\r\n\r\nI'm a teapot.\r\n"


#define DATA_TYPE_HTML true
#define DATA_TYPE_PNG false




class HttpSocket
{
public:
	SOCKET id;
	char verb[16];
	char requestUrl[16384];
	vector<char*> headers;
	char body[MAX_BODY_SIZE];
	int	recv;
	int	send;
	char buffer[MAX_BODY_SIZE];
	int len;
	int statusCode;
	size_t lastContentLength = 0; // Add this line
	
	void freeHeaders();

	bool checkValidQuery(char* query);

	void BuildHttpResponse(const char* content, size_t contentLength, bool isBinary);

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
			size_t headerLength = snprintf(nullptr, 0, OK_FORMAT_MSG, lastContentLength);
			lastContentLength = headerLength;
		}
		else if (strcmp(verb, "TRACE") == 0) {
			Trace();
		}
		else {
			throw(BAD_REQUEST);
		}
	}

private:

	bool checkVerbValid(const char* method) const {
		return strcmp(method, "GET") == 0 || strcmp(method, "POST") == 0 ||
			strcmp(method, "PUT") == 0 || strcmp(method, "DELETE") == 0 ||
			strcmp(method, "HEAD") == 0 || strcmp(method, "OPTIONS") == 0 || strcmp(method, "TRACE") == 0;
	}

	bool checkUrlVaild(const char* url) const {
		return url[0] == '/';
	}

	char* getQueryParamsFromUrl() {
		return strstr(requestUrl, "?");
	}
	
	bool htmlRequestChecker();

	std::string getFilePathFromUrl(const char* url) const;

	void Get();
	void Post();
	void Delete();
	void Put();
	void Options();
	void Head();
	void Trace();

	long int fileSize(FILE* f);
};