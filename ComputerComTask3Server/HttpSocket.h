#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
using namespace std;
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <vector>
#include <string>


#define BAD_REQUEST 400
#define NOT_FULLY_PROCCESED 0
#define OK 200
#define CREATED 201
#define NOT_FOUND 404
#define NOT_ACCEPTABLE 406
#define IM_A_TEAPOT 418

#define TEAPOT "I'm a teapot"

#define MAX_BODY_SIZE 1400000
#define MAX_LINE_LENGTH 8192
#define MAX_HEADERS 50
#define TWO_MINUTES 180
#define OK_EMPTY_MSG "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n"
#define OK_FORMAT_MSG "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %zu\r\n\r\n"
#define OK_FORMAT_TRACE "HTTP/1.1 200 OK\r\nContent-Type: message/http\r\nContent-Length: %zu\r\n\r\n"
#define OK_FORMAT_MSG_IMG "HTTP/1.1 200 OK\r\nContent-Type: image/png\r\nContent-Length: %zu\r\n\r\n"
#define OPTIONS_MSG "HTTP/1.1 200 OK\r\nAllow: GET, POST, PUT, DELETE, OPTIONS, HEAD, TRACE\r\nContent-Length: 0\r\n\r\n"
#define CREATED_EMPTY_MSG "HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\r\n"
#define BAD_REQUEST_MSG "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n"
#define NOT_FOUND_MSG "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n"
#define NOT_ACCEPTABLE_MSG "HTTP/1.1 406 Not Acceptable\r\nContent-Type: text/plain\r\nContent-Length: 20\r\n\r\n406 Not Acceptable\r\n"
#define IM_A_TEAPOT_MSG "HTTP/1.1 418 I'm a teapot\r\nContent-Type: text/plain\r\nContent-Length: 15\r\n\r\nI'm a teapot.\r\n"


#define DATA_TYPE_HTML true
#define DATA_TYPE_PNG false

class HttpSocket
{
public:
	SOCKET id;
	time_t lastRequestTime;
	char verb[16];
	char requestUrl[16384];
	vector<char*> headers;
	char body[MAX_BODY_SIZE];
	int	recv;
	int	send;
	char buffer[MAX_BODY_SIZE];
	int len;
	int statusCode;
	size_t lastContentLength = 0;
	bool gotMessage = false;
	
	bool isMessageStuck();

	void freeHeaders();

	bool checkValidQuery(char* query);

	void BuildHttpResponse(const char* content, size_t contentLength, bool isBinary);

	int ParseHttpRequest();

	void processRequest();

private:

	string createFullPath();

	bool checkVerbValid(const char* method) const;

	bool checkUrlVaild(const char* url) const {
		return url[0] == '/';
	}

	char* getQueryParamsFromUrl() {
		return strstr(requestUrl, "?");
	}
	
	bool htmlRequestChecker();

	string getFilePathFromUrl(const char* url) const;

	void Get();
	void Post();
	void Delete();
	void Put();
	void Options();
	void Head();
	void Trace();

	long int fileSize(FILE* f);
};