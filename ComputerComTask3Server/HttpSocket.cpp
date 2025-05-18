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

    bool isPNGRequest = (strstr(buffer, "png") != nullptr);
    BuildHttpResponse(body, lastContentLength, isPNGRequest);
}

//build http get response
void HttpSocket::BuildHttpResponse(const char* content, size_t contentLength, bool isBinary) {

    size_t headerLength;

    if (isBinary) {
        headerLength = snprintf(nullptr, 0, OK_FORMAT_MSG_IMG, contentLength);
    }
    else {
        headerLength = snprintf(nullptr, 0, OK_FORMAT_MSG, contentLength);
    }

    // Write header to buffer
    if (isBinary) {
        snprintf(buffer, headerLength + 1, OK_FORMAT_MSG_IMG, contentLength);
        // Copy binary content after header
        memcpy(buffer + headerLength, content, contentLength);
        // Do NOT null-terminate for binary data
    }
    else {
        snprintf(buffer, headerLength + 1, OK_FORMAT_MSG, contentLength);
        memcpy(buffer + headerLength, content, contentLength);
        buffer[headerLength + contentLength] = '\0'; // Null-terminate for text
    }

    lastContentLength = headerLength + contentLength;
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

	std::fill(body, body + sizeof(body), '\0'); //Clearing the body buffer

    // Parse query
    char* query = getQueryParamsFromUrl();
    const char* supportedLangs[] = { "he", "en", "fr" };
    const int numLangs = 3;
    char lang[8] = "he";  // Default language
    bool dataType = DATA_TYPE_HTML;
    

    if (checkValidQuery(query)) {
        strtok(query, "="); // No need for the returned value
        char* paramValue = strtok(nullptr, "\0");

        // Validate the language parameter
        bool isValidLang = false;
        for (int i = 0; i < numLangs; i++) {
            if (strcmp(paramValue, supportedLangs[i]) == 0) {
                strcpy(lang, paramValue);
                isValidLang = true;
                break;
            }
        }
        if (!isValidLang) {
            throw(BAD_REQUEST);
        }
    }

    // Extract path component from URL (before '?')
    std::string path = requestUrl;
    size_t queryPos = path.find('?');
    if (queryPos != std::string::npos) {
        path = path.substr(0, queryPos);  // Remove query parameters
    }

    // Get the file path using the existing function
    std::string filePath = getFilePathFromUrl(path.c_str());

    if (path.find(".png") != std::string::npos) {
        string dirPath = "C:\\temp\\";
        string pngFileName = path.substr(path.find_last_of("/") + 1); // filePath now Holds Path to png
        filePath = dirPath + pngFileName;
		dataType = DATA_TYPE_PNG;
    }
	else if (filePath.empty()) {
        throw(NOT_FOUND);
	}

    char* response;
    size_t contentLength;
    size_t headerLength;

    if (dataType == DATA_TYPE_HTML) {
        // Construct the path by inserting the language folder
        size_t lastSlash = filePath.find_last_of("\\");
        std::string fileName = (lastSlash != std::string::npos) ? filePath.substr(lastSlash + 1) : filePath;
        std::string dirPath = filePath.substr(0, lastSlash);
        std::string fullPath = dirPath + "\\" + std::string(lang) + "\\" + fileName + ".html";

        FILE* filePtr = fopen(fullPath.c_str(), "r");
        if (filePtr)
        {
            lastContentLength = fileSize(filePtr);
            fclose(filePtr);
        }

        // Open the file
        std::ifstream file(fullPath);
        if (!file.is_open()) {
            throw(NOT_FOUND);
        }

        file.read(body, lastContentLength);
        body[lastContentLength] = '\0';
        if (file.gcount() == 0) {
            throw(NOT_FOUND);
        }

        // Construct the HTTP HEAD response
        contentLength = strlen(body);
        headerLength = snprintf(nullptr, 0, OK_FORMAT_MSG, contentLength);
        response = new char[headerLength + 1];
        snprintf(response, headerLength + 1, OK_FORMAT_MSG, contentLength);

    }
    else { //Case: reading a png file
        FILE* pngFile = fopen(filePath.c_str(), "rb");

        if (!pngFile)
            throw(NOT_FOUND);

        long PNGfileSize = fileSize(pngFile);

        if (PNGfileSize <= 0 || PNGfileSize > (long)sizeof(body))
        {
            fclose(pngFile);
			throw(NOT_FOUND);
        }

        size_t bytesRead = fread(body, 1, PNGfileSize, pngFile);
        fclose(pngFile);

        if (bytesRead != (size_t)PNGfileSize)
            throw(NOT_FOUND);

        lastContentLength = bytesRead;

        // Construct the HTTP HEAD response
        contentLength = lastContentLength;
        headerLength = snprintf(nullptr, 0, OK_FORMAT_MSG_IMG, contentLength);
        response = new char[headerLength + 1];
        snprintf(response, headerLength + 1, OK_FORMAT_MSG_IMG, contentLength);
    }
    strcpy(buffer, response);
    delete[] response;
}


void HttpSocket::freeHeaders() {
	for (char* header : headers) {
		delete[] header;
	}
	headers.clear();
}

long int HttpSocket::fileSize(FILE* f) {

    long int res;
    long int saver = ftell(f);

    fseek(f, 0, SEEK_END);
    res = ftell(f);

    fseek(f, saver, SEEK_SET);

    return res;
}