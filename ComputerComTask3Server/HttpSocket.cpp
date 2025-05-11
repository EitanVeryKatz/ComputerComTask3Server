#include "HttpSocket.h"
#include <fstream>
#include <algorithm>


int HttpSocket::ParseHttpRequest() {
	//parse request line

	char copyBuffer[MAX_LINE_LENGTH];
	strcpy(copyBuffer, buffer);
	char* requestLine = strtok(copyBuffer, "\r\n");
	if (requestLine == nullptr) {
		return BAD_REQUEST;
	}
	if (sscanf(requestLine, "%s %s", verb, requestUrl) != 2) {
		return BAD_REQUEST; // Return an error if parsing fails
	}
	if (!checkVerbValid(verb)) {
		throw(BAD_REQUEST);
	}
	if (!checkUrlVaild(requestUrl)) {
		throw(BAD_REQUEST);
	}
	//parse headers

	for (char* token = strtok(nullptr, "\r\n"); token; token = strtok(nullptr, "\r\n")){//gets header from buffer

		size_t n = strlen(token) + 1; // finds len
		char* headerLine = new char[n];//allocate mem
		strcpy(headerLine, token);//copies to alocated mem
		headers.push_back(headerLine);//pushback
	}

	char* bodyStart = strstr(buffer, "\r\n\r\n");
	if (bodyStart == nullptr) {
		return BAD_REQUEST; // Return an error if the delimiter is not found
	}
	strcpy(body, bodyStart + 4); //skip headers

	return NOT_FULLY_PROCCESED;
}

void HttpSocket::Trace() {
	// Define the HTTP TRACE response
	const char* response =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: message/http\r\n"
		"Content-Length: %zu\r\n"
		"\r\n%s";

	size_t contentLength = strlen(buffer);
	size_t headerLength = snprintf(nullptr, 0, response, contentLength, buffer);
	char* fullResponse = new char[headerLength + 1];

	snprintf(fullResponse, headerLength + 1, response, contentLength, buffer);
	strcpy(buffer, fullResponse);
	delete[] fullResponse;

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



std::string HttpSocket::getFilePathFromUrl(const char* url) {
    const char* baseUrl = "/files/";
    if (strncmp(url, baseUrl, strlen(baseUrl)) != 0) {
        return "";
    }

    std::string filename = url + strlen(baseUrl);
    std::string filePath = "C:/temp/" + filename;

    //Replace forward slashes with backslashes for Windows compatibility
    std::replace(filePath.begin(), filePath.end(), '/', '\\');

    return filePath;
}

void HttpSocket::Post() {

    bool validContentType = false;

    //Check for valid headers
    for (const auto& header : headers) {
        if (strstr(header, "Content-Type:") != nullptr) {
            if (strstr(header, "text/plain") != nullptr || strstr(header, "text/html") != nullptr) {
                validContentType = true;
            }
        }
    }

    if (!validContentType) {
        const char badRequest[] = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
        strncpy(buffer, badRequest, sizeof(badRequest));
        buffer[sizeof(badRequest)] = '\0'; //add the null-terminating to make it a string
        return;
    }

    //Print the body to the console
    std::cout << "POST Body Content:" << std::endl << body << std::endl;

    const char ok[] = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    strncpy(buffer, ok, sizeof(ok));
    buffer[sizeof(ok)] = '\0'; //add the null-terminating to make it a string
}

bool HttpSocket::htmlRequestChecker() {

    bool validContentType = false;


    for (const auto& header : headers) {

        if (strstr(header, "Content-Type:") != nullptr) {
            if (strstr(header, "text/html") != nullptr) {
                validContentType = true;
            }
        }
    }

    if (!validContentType) {
        strncpy(buffer, BAD_REQUEST_MSG, sizeof(BAD_REQUEST_MSG));
        buffer[sizeof(BAD_REQUEST_MSG)] = '\0'; //add the null-terminating to make it a string
    }
	return validContentType;
}

void HttpSocket::Put() {

	htmlRequestChecker();

    std::string filePath = getFilePathFromUrl(requestUrl);
    if (filePath.empty()) {
        strncpy(buffer, BAD_REQUEST_MSG, sizeof(BAD_REQUEST_MSG));
        buffer[sizeof(BAD_REQUEST_MSG)] = '\0'; //add the null-terminating to make it a string
        return;
    }

    if (strlen(body) == 0) {
        const char noContent[] = "HTTP/1.1 204 No Content\r\nContent-Length: 0\r\n\r\n";
        strncpy(buffer, noContent, sizeof(noContent));
        buffer[sizeof(noContent)] = '\0'; //add the null-terminating to make it a string
        return;
    }

    //Attempt to open the file and write to it
    std::ofstream file(filePath);
    if (!file) { //Case: file wasn't found or no permissions to write
        const char notFound[] = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        strncpy(buffer, notFound, sizeof(notFound));
        buffer[sizeof(notFound)] = '\0'; //add the null-terminating to make it a string
        return;
    }

    file << body;
    file.close();

    std::cout << "File " << filePath << " successfully updated";

    const char ok[] = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    strncpy(buffer, ok, sizeof(ok));
    buffer[sizeof(ok)] = '\0'; //add the null-terminating to make it a string
}

void HttpSocket::Delete() {

    std::string filePath = getFilePathFromUrl(requestUrl);
    if (filePath.empty()) {
        const char badRequest[] = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
        strncpy(buffer, badRequest, sizeof(badRequest));
        buffer[sizeof(badRequest)] = '\0'; //add the null-terminating to make it a string
        return;
    }

    //Attempt to delete the file
    if (remove(filePath.c_str())) {
        const char notFound[] = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        strncpy(buffer, notFound, sizeof(notFound));
        buffer[sizeof(notFound)] = '\0'; //add the null-terminating to make it a string
        return;
    }

    std::cout << "File " << filePath << " successfully deleted";

    const char ok[] = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    strncpy(buffer, ok, sizeof(ok));
    buffer[sizeof(ok)] = '\0'; //add the null-terminating to make it a string
}

bool HttpSocket::checkValidQuery(char* query) {
	if (query == nullptr) {
		return false;
	}
	char temp[32];
	strcpy(temp, query);
	char* key = strtok(temp, "=");
	char* value = strtok(nullptr, "\0");
	if (key == nullptr || value == nullptr) {
		return false;
	}
	if (strcmp(key, "?lang") != 0) {
		return false;
	}
	if (strcmp(value, "he") != 0 && strcmp(value, "en") != 0 && strcmp(value, "fr") != 0) {
		return false;
	}
	return true;

}

void HttpSocket::Head() {

	//parse query
	char* query = getQueryParamsFromUrl();
	
	char htmlFilesPath[32] = {};
	strcpy(htmlFilesPath, "C:\\temp\\");//path to files
	if (checkValidQuery(query)) {
		//parse query
		char* key = strtok(query, "=");
		char* lang = strtok(nullptr, "\0");
		
		
		

		if (strcmp(lang, "he") == 0) {
			strcat(htmlFilesPath, "he.html");
		}
		else if (strcmp(lang, "en") == 0) {
			strcat(htmlFilesPath, "en.html");
		}
		else if (strcmp(lang, "fr") == 0) {
			strcat(htmlFilesPath, "fr.html");
		}
		else
			throw(BAD_REQUEST);
	}
	else {
		strcat(htmlFilesPath, "he.html");
	}
	//open file
	std::ifstream carmion(htmlFilesPath);
	/*std::ifstream carmion("C:\\temp\\en.html");*/
	if (!carmion.is_open()) {
		throw(NOT_FOUND);
		
	}
	carmion.read(body, sizeof(body));
	if (carmion.gcount() == 0) {
		throw(NOT_FOUND);
	}
	// Define the HTTP HEAD response
	const char* header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %zu\r\n\r\n";
	size_t contentLength = strlen(body);
	size_t headerLength = snprintf(nullptr, 0, header, contentLength);
	char* response = new char[headerLength + contentLength + 1];
	snprintf(response, headerLength + 1, header, contentLength);
	strcpy(buffer, response);
}

void HttpSocket::freeHeaders() {
	for (char* header : headers) {
		delete[] header;
	}
	headers.clear();
}