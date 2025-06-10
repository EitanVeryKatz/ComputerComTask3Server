#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <algorithm>
#include "HttpSocket.h"

// Parses the HTTP request stored in the buffer.
// Extracts the request line, headers, and body.
// Returns an int status code indicating the result of parsing.
// Returns BAD_REQUEST if parsing fails, NOT_FULLY_PROCCESED if successful but incomplete.

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

// Validates the HTTP method (verb) in the request.
// Returns true if the method is valid, false otherwise.
bool HttpSocket::checkVerbValid(const char* method) const {
    return strcmp(method, "GET") == 0 || strcmp(method, "POST") == 0 ||
        strcmp(method, "PUT") == 0 || strcmp(method, "DELETE") == 0 ||
        strcmp(method, "HEAD") == 0 || strcmp(method, "OPTIONS") == 0 || strcmp(method, "TRACE") == 0;
}

// Processes the HTTP request based on the parsed verb.
// Calls the appropriate handler function for the request type.
void HttpSocket::processRequest(){
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

// Handles the HTTP TRACE request.
// Echoes back the received request in the response body.
// Throws NOT_ACCEPTABLE if the request URL is invalid.
void HttpSocket::Trace() {
	// Define the HTTP TRACE response
    if (strcmp(requestUrl, "/")) {
        throw NOT_ACCEPTABLE;
    }
	size_t contentLength = strlen(buffer);
    strcpy(body, buffer);

	size_t headerLength = snprintf(buffer, MAX_LINE_LENGTH, OK_FORMAT_TRACE, contentLength);
    strcat(buffer, body);

    lastContentLength = headerLength + contentLength;
}

// Handles the HTTP OPTIONS request.
// Responds with a predefined message listing supported methods.
void HttpSocket::Options()
{
    // Define the HTTP OPTIONS response
    if (strcmp(requestUrl, "/")) {
        throw NOT_ACCEPTABLE;
    }

	// Copy the response into the buffer
	strcpy(buffer, OPTIONS_MSG);
    lastContentLength = strlen(OPTIONS_MSG);
}

// Handles the HTTP GET request.
// Calls the Head function and builds the HTTP response.
void HttpSocket::Get() {

	Head();

    bool isPNGRequest = (strstr(buffer, "png") != nullptr);
    BuildHttpResponse(body, lastContentLength, isPNGRequest);
}

// Builds the HTTP response for GET requests.
// Constructs the response header and appends the content.
// content - The content to include in the response.
// contentLength - The length of the content.
// isBinary - True if the content is binary (e.g., PNG), false otherwise.
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

// Extracts the file path from the request URL.
// url - The request URL.
// Returns the file path corresponding to the URL.
string HttpSocket::getFilePathFromUrl(const char* url) const {
    const char* baseUrl = "/files/";
    if (strncmp(url, baseUrl, strlen(baseUrl)) != 0) {
        return "";
    }

    string filename = url + strlen(baseUrl);
    string filePath = "C:/temp/" + filename;

    //Replace forward slashes with backslashes for Windows compatibility
    replace(filePath.begin(), filePath.end(), '/', '\\');

    return filePath;
}

// Handles the HTTP POST request.
// Validates headers and body, and processes the request.
// Throws NOT_ACCEPTABLE if url is invalid or if the body is empty.
// Throws BAD_REQUEST if the Content-Type header is invalid.
void HttpSocket::Post() {
    if (strcmp(requestUrl, "/")) {
        throw NOT_ACCEPTABLE;
    }

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
	cout << "POST Body Content:" << endl << body << endl;

	//First, print the post message, then check for easter egg
	if (strcmp(body, TEAPOT) == 0) //Easter egg
		throw(IM_A_TEAPOT);

    strcpy(buffer, OK_EMPTY_MSG);
    lastContentLength = strlen(OK_EMPTY_MSG);
}

// Checks if the Content-Type header is valid for HTML requests.
// Returns true if the Content-Type is valid, false otherwise.
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

// Handles the HTTP PUT request.
// Validates headers and body, and processes the request.
// Throws NOT_ACCEPTABLE if the Content-Type header is invalid or if the body is empty.
// Throws BAD_REQUEST if the URL is invalid.
// Throws NOT_FOUND if the file cannot be opened or written to.
void HttpSocket::Put() {

	if (!htmlRequestChecker()) //Case: not a text/html request
        throw(NOT_ACCEPTABLE);

    if (strlen(body) == 0) { //Case: empty body
        throw(NOT_ACCEPTABLE);
    }

    string fullPath = createFullPath();

	
    // Check if the file already exists
    ifstream fileOpen(fullPath);
    bool fileExists = fileOpen.good();
    fileOpen.close();

    //Attempt to open the file and write to it
    ofstream file(fullPath);
    if (!file) //Case: no permissions
        throw(NOT_FOUND);

    file << body;
    file.close();

    cout << "File " << fullPath << " successfully " << (fileExists ? "updated" : "created") << endl;

    if (fileExists)
    {
        strcpy(buffer, OK_EMPTY_MSG);
        lastContentLength = strlen(OK_EMPTY_MSG);
    }
	else //Case: file was created
    {
        strcpy(buffer, CREATED_EMPTY_MSG);
        lastContentLength = strlen(CREATED_EMPTY_MSG);
    }
}

// Handles the HTTP DELETE request.
// Validates the request URL and deletes the specified file.
// Throws BAD_REQUEST if the URL is invalid.
// Throws NOT_FOUND if the file cannot be found or deleted.
void HttpSocket::Delete() {

    string fullPath = createFullPath();

    //Attempt to delete the file
    if (remove(fullPath.c_str())) {
		throw(NOT_FOUND); //Case: file wasn't found or no permissions to delete
    }

    cout << "File " << fullPath << " successfully deleted" << endl;

	strcpy(buffer, OK_EMPTY_MSG);
    lastContentLength = strlen(OK_EMPTY_MSG);
}

// Extracts the query parameters from the request URL.
// checks for validity of the query parameters.
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

// Extracts the query parameters from the request URL.
//tries to open the file according to the query parameters.
//fills the body with the file content if successful.
//throws NOT_FOUND if the file cannot be opened.
//throws BAD_REQUEST if the URL is invalid.
void HttpSocket::Head() {

	fill(body, body + sizeof(body), '\0'); //Clearing the body buffer

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
    string path = requestUrl;
    size_t queryPos = path.find('?');
    if (queryPos != string::npos) {
        path = path.substr(0, queryPos);  // Remove query parameters
    }

    // Get the file path using the existing function
    string filePath = getFilePathFromUrl(path.c_str());

    if (path.find(".png") != string::npos) {
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
        string fileName = (lastSlash != string::npos) ? filePath.substr(lastSlash + 1) : filePath;
        string dirPath = filePath.substr(0, lastSlash);
        string fullPath = dirPath + "\\" + string(lang) + "\\" + fileName + ".html";

        FILE* filePtr = fopen(fullPath.c_str(), "r");
        if (filePtr)
        {
            lastContentLength = fileSize(filePtr);
            fclose(filePtr);
        }

        // Open the file
        ifstream file(fullPath);
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

// Frees the memory allocated for headers.
// Clears the headers vector.
void HttpSocket::freeHeaders() {
	for (char* header : headers) {
		delete[] header;
	}
	headers.clear();
}

// Calculates the size of a file.
// f - Pointer to the file.
// Returns the size of the file in bytes.
long int HttpSocket::fileSize(FILE* f) {

    long int res;
    long int saver = ftell(f);

    fseek(f, 0, SEEK_END);
    res = ftell(f);

    fseek(f, saver, SEEK_SET);

    return res;
}

string HttpSocket::createFullPath() {

    string filePath = getFilePathFromUrl(requestUrl);
    if (filePath.empty())
        throw(BAD_REQUEST); //Case: Invalid URL

    // Parse query
    char* query = getQueryParamsFromUrl();
    const char* supportedLangs[] = { "he", "en", "fr" };
    const int numLangs = 3;
    char lang[3] = "he";  // Default language

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
    string path = filePath;
    size_t queryPos = path.find('?');
    if (queryPos != string::npos) {
        path = path.substr(0, queryPos);  // Remove query parameters
    }

    // Construct the path by inserting the language folder
    size_t lastSlash = path.find_last_of("\\");
    string fileName = (lastSlash != string::npos) ? path.substr(lastSlash + 1) : path;
    string dirPath = path.substr(0, lastSlash);
    string fullPath = dirPath + "\\" + string(lang) + "\\" + fileName;

	return fullPath;
}

bool HttpSocket::isMessageStuck() {
	// Check if the last request was made more than 2 minutes ago
	time_t currentTime = time(nullptr);
	return (difftime(currentTime, lastRequestTime) > TWO_MINUTES);
}