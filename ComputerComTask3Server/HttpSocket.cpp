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

bool HttpSocket::checkVerbValid(const char* method) const {
    return strcmp(method, "GET") == 0 || strcmp(method, "POST") == 0 ||
        strcmp(method, "PUT") == 0 || strcmp(method, "DELETE") == 0 ||
        strcmp(method, "HEAD") == 0 || strcmp(method, "OPTIONS") == 0 || strcmp(method, "TRACE") == 0;
}

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

void HttpSocket::Options()
{
	// Copy the response into the buffer
	strcpy(buffer, OPTIONS_MSG);
    lastContentLength = strlen(OPTIONS_MSG);
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
        throw(NOT_ACCEPTABLE); //Case: if not text/html, not acceptable

    if (strlen(body) == 0) {
        throw(NOT_ACCEPTABLE); //Case: empty body is not acceptable
    }

    string filePath = getFilePathFromUrl(requestUrl);
    if (filePath.empty())
        throw(BAD_REQUEST); //Case: Invalid URL

    // Parse query
    char* query = getQueryParamsFromUrl();
    const char* supportedLangs[] = { "he", "en", "fr" };
    const int numLangs = 3;
    char lang[8] = "he";  // Default language

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

    //Attempt to open the file and write to it
    ofstream file(fullPath);
    if (!file) //Case: file wasn't found or no permissions to write
		throw(NOT_FOUND);
    
    file << body;
    file.close();

    cout << "File " << fullPath << " successfully updated" << endl;

	strcpy(buffer, OK_EMPTY_MSG);
    lastContentLength = strlen(OK_EMPTY_MSG);
}

void HttpSocket::Delete() {

    string filePath = getFilePathFromUrl(requestUrl);
    if (filePath.empty())
        throw(BAD_REQUEST); //Case: Invalid URL

    // Parse query
    char* query = getQueryParamsFromUrl();
    const char* supportedLangs[] = { "he", "en", "fr" };
    const int numLangs = 3;
    char lang[8] = "he";  // Default language

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

    //Attempt to delete the file
    if (remove(fullPath.c_str())) {
		throw(NOT_FOUND); //Case: file wasn't found or no permissions to delete
    }

    cout << "File " << fullPath << " successfully deleted" << endl;

	strcpy(buffer, OK_EMPTY_MSG);
    lastContentLength = strlen(OK_EMPTY_MSG);
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

