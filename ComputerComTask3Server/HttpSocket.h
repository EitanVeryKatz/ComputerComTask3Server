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

	int ParseHttpRequest(char* request) {
		// שמירת עותק זמני כי strtok משנה את המחרוזת
		char temp[MAX_LINE_LENGTH];
		strncpy(temp, request, MAX_LINE_LENGTH - 1);
		temp[MAX_LINE_LENGTH - 1] = '\0';

		// שורת הבקשה הראשונה
		char* line = strtok(temp, "\r\n");
		if (!line) return BAD_REQUEST;

		char* method = strtok(line, " ");
		char* url = strtok(NULL, " ");
		char* httpVersion = strtok(NULL, " ");

		if (!method || !url || !httpVersion) return BAD_REQUEST;

		if (!checkVerbValid(method)) return BAD_REQUEST;
		if (!checkUrlVaild(url)) return BAD_REQUEST;

		strncpy(verb, method, sizeof(verb));
		strncpy(requestUrl, url, sizeof(requestUrl));


		while (true) {
			char* header = strtok(NULL, "\r\n");
			if (!header || strlen(header) == 0) break;
			headers.push_back(_strdup(header));


			char* bodyStart = strstr(request, "\r\n\r\n");
			if (bodyStart) {
				bodyStart += 4;
				strncpy(body, bodyStart, sizeof(body));
			}
			else {
				body[0] = '\0';
			}

			return NOT_FULLY_PROCCESED;
		}
	}

	bool checkVerbValid(const char* method) {
		return strcmp(method, "GET") == 0 || strcmp(method, "POST") == 0 ||
			strcmp(method, "PUT") == 0 || strcmp(method, "DELETE") == 0 ||
			strcmp(method, "HEAD") == 0|| strcmp(method, "OPTIONS") == 0|| strcmp(method, "TRACE") == 0;
	}

	bool checkUrlVaild(const char* url) {
		return url[0] == '/';

	}

	char* getQueryParamsFromUrl() {
		return strstr(requestUrl, "?");
	}

};