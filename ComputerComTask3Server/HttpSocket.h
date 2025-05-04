#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>

enum HTTP_Actions { Get, Post, Put, Delete, Head, Options, Trace };

class HttpSocket
{
	SOCKET id;			// Socket handle
	HTTP_Actions Action;
	char* requestUrl;
	char* body;
	int	recv;			// Receiving?
	int	send;			// Sending?
	char buffer[255];
	int len;

	string getAction() {
		char* request = buffer;
		return strtok(request, " ");
	}
	string getBody() {
		char* request = buffer;
		string RequestAndHeaders = strtok(request, "\r\n\r\n");
		return buffer + RequestAndHeaders.length() + 4;
	}
	string getLangQueryParam() {
		char* request = buffer;
		char* query = strtok(request, "?");
		query = strtok(NULL, " ");
		if (strncmp(query, "lang=", 5) == 0) {
			if (strncmp(query + 5, "he", 2) == 0 || strncmp(query + 5, "en", 2) == 0 || strncmp(query + 5, "fr", 2) == 0) {

			}
		}
	}
};

