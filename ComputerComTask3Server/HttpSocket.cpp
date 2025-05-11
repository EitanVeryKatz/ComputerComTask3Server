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
	size_t contentLength = strlen(buffer);
	size_t headerLength = snprintf(nullptr, 0, OK_FORMAT_MSG, contentLength, buffer);
	char* fullResponse = new char[headerLength + 1];

	snprintf(fullResponse, headerLength + 1, OK_FORMAT_MSG, contentLength, buffer);
	strcpy(buffer, fullResponse);
	delete[] fullResponse;
}

void HttpSocket::Options()
{
	// Copy the response into the buffer
	strcpy(buffer, OPTIONS_MSG);
}

void HttpSocket::Get() {
	Head();
	BuildHttpResponse(body);
}

//build http get response
void HttpSocket::BuildHttpResponse(const char* content) {
	
	size_t contentLength = strlen(content);
	size_t headerLength = snprintf(nullptr, 0, OK_FORMAT_MSG, contentLength);
	char* response = new char[headerLength + contentLength + 1];

	snprintf(response, headerLength + 1, OK_FORMAT_MSG, contentLength);
	strcat(response, content);
	
	strcpy(buffer,response);
	delete[] response;
}

std::string HttpSocket::getFilePathFromUrl(const char* url) const {
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

	if (body[0] == '\0') //Case: no body was sent
		throw(NOT_ACCEPTABLE);

    bool validContentType = false;

    //Check for valid headers
    for (const auto& header : headers) {
        if (strstr(header, "Content-Type:") != nullptr) {
            if (strstr(header, "text/plain") != nullptr || strstr(header, "text/html") != nullptr) {
                validContentType = true;
            }
        }
    }

    if (!validContentType) //Case: Invalid Content-Type header
		throw(BAD_REQUEST);

	//Print the body to the console
	std::cout << "POST Body Content:" << std::endl << body << std::endl;

	//First, print the post message, then check for easter egg
	if (strcmp(body, TEAPOT) == 0) //Easter egg
		throw(IM_A_TEAPOT);

    strcpy(buffer, OK_EMPTY_MSG);
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

	if (!htmlRequestChecker()) //Case: put is not text/html
		return;

    std::string filePath = getFilePathFromUrl(requestUrl);
    if (filePath.empty()) {
        strncpy(buffer, BAD_REQUEST_MSG, sizeof(BAD_REQUEST_MSG));
        buffer[sizeof(BAD_REQUEST_MSG)] = '\0'; //add the null-terminating to make it a string
        return;
    }

    if (strlen(body) == 0) {
		throw(NOT_ACCEPTABLE); //Case: empty body is not acceptable
    }

    //Attempt to open the file and write to it
    std::ofstream file(filePath);
    if (!file) //Case: file wasn't found or no permissions to write
		throw(NOT_FOUND);
    
    file << body;
    file.close();

    std::cout << "File " << filePath << " successfully updated" << std::endl;

	strcpy(buffer, OK_EMPTY_MSG);
}

void HttpSocket::Delete() {

    std::string filePath = getFilePathFromUrl(requestUrl);
    if (filePath.empty()) 
        throw(BAD_REQUEST); //Case: Invalid URL

    //Attempt to delete the file
    if (remove(filePath.c_str())) {
		throw(NOT_FOUND); //Case: file wasn't found or no permissions to delete
    }

    std::cout << "File " << filePath << " successfully deleted" << std::endl;

	strcpy(buffer, OK_EMPTY_MSG);
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
		strtok(query, "="); //No need for the returned value
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
	if (!carmion.is_open()) {
		throw(NOT_FOUND);
		
	}
	carmion.read(body, sizeof(body));
	if (carmion.gcount() == 0) {
		throw(NOT_FOUND);
	}
	// Define the HTTP HEAD response
	
	size_t contentLength = strlen(body);
	size_t headerLength = snprintf(nullptr, 0, OK_FORMAT_MSG, contentLength);
	char* response = new char[headerLength + contentLength + 1];
	snprintf(response, headerLength + 1, OK_FORMAT_MSG, contentLength);
	strcpy(buffer, response);
	delete[] response; //Added to prevent memory leak
}

void HttpSocket::freeHeaders() {
	for (char* header : headers) {
		delete[] header;
	}
	headers.clear();
}