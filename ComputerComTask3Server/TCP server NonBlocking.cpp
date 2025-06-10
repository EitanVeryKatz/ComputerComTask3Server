#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include "HttpSocket.h"
using namespace std;

const int HTTP_PORT = 8080;
const int MAX_SOCKETS = 60;
const int EMPTY = 0;
const int LISTEN  = 1;
const int RECEIVE = 2;
const int IDLE = 3;
const int SEND = 4;
const int SEND_TIME = 1;
const int SEND_SECONDS = 2;

bool addSocket(SOCKET id, int what);
void removeSocket(int index);
void acceptConnection(int index);
void receiveMessage(int index);
void sendMessage(int index);

HttpSocket sockets[MAX_SOCKETS]={0};
int socketsCount = 0;


void main() 
{
    // Initialize Winsock (Windows Sockets).
	WSAData wsaData; 
 
	if (NO_ERROR != WSAStartup(MAKEWORD(2,2), &wsaData))
	{
        cout<<"Http Server: Error at WSAStartup()\n";
		return;
	}

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Check for errors to ensure that the socket is a valid socket.
	if (INVALID_SOCKET == listenSocket)
	{
        cout<<"Http Server: Error at socket(): "<<WSAGetLastError()<<endl;
        WSACleanup();
        return;
	}

    // Create a sockaddr_in object called serverService. 
	sockaddr_in serverService;
    serverService.sin_family = AF_INET; 
	
	// The IP address is INADDR_ANY to accept connections on all interfaces.
	serverService.sin_addr.s_addr = INADDR_ANY;
	serverService.sin_port = htons(HTTP_PORT);
    if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR *) &serverService, sizeof(serverService))) 
	{
		cout<<"Http Server: Error at bind(): "<<WSAGetLastError()<<endl;
        closesocket(listenSocket);
		WSACleanup();
        return;
    }

    // Listen on the Socket for incoming connections.
    if (SOCKET_ERROR == listen(listenSocket, 5))
	{
		cout << "Http Server: Error at listen(): " << WSAGetLastError() << endl;
        closesocket(listenSocket);
		WSACleanup();
        return;
	}
	addSocket(listenSocket, LISTEN);

	cout << "Http server listening on port " << HTTP_PORT << "..." << endl;

    // Accept connections and handles them one by one.
	while (true)
	{
		fd_set waitRecv;
		FD_ZERO(&waitRecv);

		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE))
				FD_SET(sockets[i].id, &waitRecv);
		}

		fd_set waitSend;
		FD_ZERO(&waitSend);

		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].send == SEND)
				FD_SET(sockets[i].id, &waitSend);
		}

		int nfd;
		nfd = select(0, &waitRecv, &waitSend, NULL, NULL);
		if (nfd == SOCKET_ERROR)
		{
			cout <<"Http Server: Error at select(): " << WSAGetLastError() << endl;
			WSACleanup();
			return;
		}

		//handle incoming requests
		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			
			if (FD_ISSET(sockets[i].id, &waitRecv))
			{
				nfd--;
				switch (sockets[i].recv)
				{
				case LISTEN:
					acceptConnection(i);
					break;

				case RECEIVE:
					receiveMessage(i);
					break;
				}
			}
			if (sockets[i].gotMessage == true && sockets[i].isMessageStuck()) {
				closesocket(sockets[i].id);
				removeSocket(i);
			}

		}

		//process requests, prepeare and send responses
		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitSend))
			{
				nfd--;
				switch (sockets[i].send)
				{
				case SEND:
					sendMessage(i);
					break;
				}
			}
		}
	}

	// Closing connections and Winsock.
	cout << "Http Server: Closing Connection.\n";
	closesocket(listenSocket);
	WSACleanup();
}

bool addSocket(SOCKET id, int what)
{
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			sockets[i].id = id;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			sockets[i].len = 0;
			socketsCount++;
			return (true);
		}
	}
	return (false);
}

void removeSocket(int index)
{
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	sockets[index].gotMessage = false;
	socketsCount--;
}

void acceptConnection(int index)
{
	SOCKET id = sockets[index].id;
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(id, (struct sockaddr *)&from, &fromLen);
	if (INVALID_SOCKET == msgSocket)
	{ 
		 cout << "Http Server: Error at accept(): " << WSAGetLastError() << endl; 		 
		 return;
	}
	cout << "Http Server: Client "<<inet_ntoa(from.sin_addr)<<":"<<ntohs(from.sin_port)<<" is connected." << endl;

	//
	// Set the socket to be in non-blocking mode.
	//
	unsigned long flag=1;
	if (ioctlsocket(msgSocket, FIONBIO, &flag) != 0)
	{
		cout<<"Http Server: Error at ioctlsocket(): "<<WSAGetLastError()<<endl;
	}

	if (addSocket(msgSocket, RECEIVE) == false)
	{
		cout<<"\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
	return;
}

void receiveMessage(int index)
{
	SOCKET msgSocket = sockets[index].id;

	int len = sockets[index].len;
	int bytesRecv = recv(msgSocket, &sockets[index].buffer[len], sizeof(sockets[index].buffer) - len, 0);
	
	if (SOCKET_ERROR == bytesRecv)
	{
		cout << "Http Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);			
		removeSocket(index);
		return;
	}
	if (bytesRecv == 0)
	{
		closesocket(msgSocket);			
		removeSocket(index);
		return;
	}
	else
	{
		sockets[index].buffer[len + bytesRecv] = '\0'; //add the null-terminating to make it a string
		
		sockets[index].len += bytesRecv;

		//Parse HTTP request
		sockets[index].statusCode = sockets[index].ParseHttpRequest();
		
		if (sockets[index].statusCode == BAD_REQUEST) //Case: The HTTP Request was bad
		{
			cout << "Http Server: Bad HTTP request received." << endl;
			string msg = BAD_REQUEST_MSG;
			strncpy(sockets[index].buffer, msg.c_str(), msg.size());
			(sockets[index].buffer)[msg.size()] = '\0'; //add the null-terminating to make it a string
			sockets[index].len = msg.size();
		}
		sockets[index].lastRequestTime = time(NULL);
		sockets[index].gotMessage = true;
		sockets[index].send = SEND;
	}
}

void sendMessage(int index)
{
	int bytesSent = 0;
	char* sendBuff;

	SOCKET msgSocket = sockets[index].id;
	
	try {
		sockets[index].processRequest();
		
	}
	catch (const int statusCode) {
		
		if (statusCode==NOT_FOUND) {
			cout << "Http Server: Error: " << NOT_FOUND_MSG << endl;
			strcpy(sockets[index].buffer, NOT_FOUND_MSG);
			sockets[index].lastContentLength = strlen(NOT_FOUND_MSG);
		}
		else if (statusCode == NOT_ACCEPTABLE) {
			cout << "Http Server: Error: " << NOT_ACCEPTABLE_MSG << endl;
			strcpy(sockets[index].buffer, NOT_ACCEPTABLE_MSG);
			sockets[index].lastContentLength = strlen(NOT_ACCEPTABLE_MSG);
		}
		else if (statusCode == IM_A_TEAPOT) {
			cout << "Http Server: Error: " << IM_A_TEAPOT_MSG << endl;
			strcpy(sockets[index].buffer, IM_A_TEAPOT_MSG);
			sockets[index].lastContentLength = strlen(IM_A_TEAPOT_MSG);
		}
		else {
			cout << "Http Server: Error: " << BAD_REQUEST_MSG << endl;
			strcpy(sockets[index].buffer, BAD_REQUEST_MSG);
			sockets[index].lastContentLength = strlen(BAD_REQUEST_MSG);
		}
	}
	sendBuff = sockets[index].buffer;

	bytesSent = send(msgSocket, sendBuff, (int)sockets[index].lastContentLength, 0);
	if (SOCKET_ERROR == bytesSent)
	{
		cout << "Http Server: Error at send(): " << WSAGetLastError() << endl;	
		return;
	}

	sockets[index].lastContentLength = 0;
	sockets[index].freeHeaders();
	sockets[index].len = 0;
	sockets[index].send = IDLE;
}