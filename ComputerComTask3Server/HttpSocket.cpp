#include "HttpSocket.h"
#include <fstream>


int HttpSocket::ParseHttpRequest() {
	//parse request line
	char* requestLine = strtok(buffer, "\r\n");
	if (requestLine == nullptr) {
		return NOT_FULLY_PROCCESED;
	}
	sscanf(requestLine, "%s %s", verb, requestUrl);
	if (!checkVerbValid(verb)) {
		throw(BAD_REQUEST);
	}
	if (!checkUrlVaild(requestUrl)) {
		throw(BAD_REQUEST);
	}
	strcpy(body, strstr(buffer, "\r\n\r\n") + 4); //skip headers

	return NOT_FULLY_PROCCESED;
}

void HttpSocket::Options()
{
	// Define the HTTP OPTIONS response
	const char* response =
		"HTTP/1.1 200 OK\r\n"
		"Allow: OPTIONS, GET, POST, PUT, DELETE, HEAD\r\n"
		"Content-Length: 0\r\n"
		"\r\n";

	// Copy the response into the buffer
	strcpy(buffer, response);
}



void HttpSocket::Get() {
	Head();
	BuildHttpResponse(body);
}

//build http get response
void HttpSocket::BuildHttpResponse(const char* content) {
	const char* header = "HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: %zu\r\n"
		"\r\n";
	size_t contentLength = strlen(content);
	size_t headerLength = snprintf(nullptr, 0, header, contentLength);
	char* response = new char[headerLength + contentLength + 1];

	snprintf(response, headerLength + 1, header, contentLength);
	strcat(response, content);
	
	strcpy(buffer,response);
	delete[] response;
}

void HttpSocket::Head() {
	// Define the HTTP HEAD response
	char* query = getQueryParamsFromUrl();
	char* htmlFilePath = new char;
	if (checkValidQuery(query)) {
		//parse query
		char* key = strtok(query, "=");
		char* value = strtok(nullptr, "&");
		char* lang = value;

		if (strcmp(lang, "he") == 0) {
			strcpy(htmlFilePath, "he.html");
		}
		else if (strcmp(lang, "en") == 0) {
			strcpy(htmlFilePath, "en.html");
		}
		else if (strcmp(lang, "fr") == 0) {
			strcpy(htmlFilePath, "fr.html");
		}
		else
			throw (BAD_REQUEST);
	}
	else {
		strcpy(htmlFilePath, "he.html");
	}
	//open file
	std::ifstream carmion(htmlFilePath);
	carmion.read(body, sizeof(body));
	if (carmion.gcount() == 0) {
		throw (NOT_FOUND);
	}
	// Define the HTTP HEAD response
	const char* header = "HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: %zu\r\n"
		"\r\n";
	strcpy(buffer, header);
}

