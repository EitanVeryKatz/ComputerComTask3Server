#include "HttpSocket.h"
#include <fstream>


char* HttpSocket::Get() {
    FILE* file = fopen("he.txt", "rb");
    if (!file) {
        cerr << "Failed to open file: " << "he.txt" << endl;
        return nullptr;
    }

    // Go to end of file to determine length
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);

    // Allocate memory for content (+1 for null terminator)
    char* buffer = new char[length + 1];
    if (fread(buffer, 1, length, file) != (size_t)length) {
        cerr << "Failed to read file: " << "he.txt" << endl;
        fclose(file);
        delete[] buffer;
        return nullptr;
    }

    buffer[length] = '\0'; // Null-terminate
    fclose(file);
    return buffer;
}


