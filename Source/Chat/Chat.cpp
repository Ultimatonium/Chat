#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif 

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>

#pragma comment(lib, "Ws2_32.lib")

constexpr int DEFAULT_BUFFER_LENGTH = 256;
constexpr auto DEFAULT_IP = "127.0.0.1";
constexpr auto DEFAULT_PORT = "55555";
constexpr auto DEFAULT_USERNAME = "THE NOOB";

int doSending(PCSTR ip, PCSTR port, std::string username);
int doReceiving(PCSTR ip, PCSTR port);

int main(int argc, char *argv[]) {
	std::string input;
	char function;
	std::string username;
	std::string ip;
	do {
		printf("(s)end or (r)eceive? \n");
		std::getline(std::cin, input);
		function = input.at(0);
	} while (function != 's' && function != 'r');

	printf("TargetIP? \n");
	std::getline(std::cin, ip);
	if (ip == "") {
		ip = DEFAULT_IP;
	}

	switch (function)
	{
	case 's':
		printf("Username? \n");
		std::getline(std::cin, username);
		if (username == "") {
			username = DEFAULT_USERNAME;
		}
		return doSending(ip.c_str(), DEFAULT_PORT, username);
	case 'r':
		return doReceiving(ip.c_str(), DEFAULT_PORT);
	default:
		printf("Invalid Input %c \n", function);
		return function;
	}
}

int doSending(PCSTR ip, PCSTR port, std::string username) {
	int retCd;

	//init winsocket
	WSADATA wsaData;
	retCd = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (retCd != 0) {
		printf("Error on WSAStartup: %d \n", retCd);
		return retCd;
	}

	//get addr info
	addrinfo hints;
	memset(&hints, 0, sizeof(hints)); //cleanup garbage in struc
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	addrinfo *addrInfo;
	retCd = getaddrinfo(ip, port, &hints, &addrInfo);
	if (retCd != 0) {
		printf("Error on getAddrInfo: %d \n", retCd);
		WSACleanup();
		return retCd;
	}

	//create socket
	SOCKET sendSocket;
	sendSocket = socket(addrInfo->ai_family, addrInfo->ai_socktype, addrInfo->ai_protocol);
	if (sendSocket == INVALID_SOCKET) {
		retCd = WSAGetLastError();
		printf("Error in socket: %d \n", retCd);
		freeaddrinfo(addrInfo);
		WSACleanup();
		return retCd;
	}

	//connect socket
	retCd = connect(sendSocket, addrInfo->ai_addr, addrInfo->ai_addrlen);
	if (retCd == SOCKET_ERROR) {
		retCd = WSAGetLastError();
		printf("Error in connect %d \n", retCd);
		closesocket(sendSocket);
		freeaddrinfo(addrInfo);
		WSACleanup();
		return retCd;
	}

	//early freeaddrinfo
	freeaddrinfo(addrInfo);

	//shutdown receive
	retCd = shutdown(sendSocket, SD_RECEIVE);
	if (retCd == SOCKET_ERROR) {
		retCd = WSAGetLastError();
		printf("Error in shutdown %d \n", retCd);
		closesocket(sendSocket);
		WSACleanup();
		return retCd;
	}

	//send
	printf("Ready to send to %s \n", ip);
	std::string input = "";
	do {
		std::getline(std::cin, input);
		if (input == "\\x") {
			break;
		}
		input = username + ": " + input;
		retCd = send(sendSocket, input.c_str(), strlen(input.c_str()), 0);
		if (retCd == SOCKET_ERROR) {
			retCd = WSAGetLastError();
			printf("Error in send %d \n", retCd);
			closesocket(sendSocket);
			WSACleanup();
			return retCd;
		}
	} while (true);

	//shutdown
	retCd = shutdown(sendSocket, SD_BOTH);
	if (retCd == SOCKET_ERROR) {
		retCd = WSAGetLastError();
		printf("Error in shutdown %d \n", retCd);
		closesocket(sendSocket);
		WSACleanup();
		return retCd;
	}

	//cleanup
	closesocket(sendSocket);
	WSACleanup();

	//all ok
	return 0;
}

int doReceiving(PCSTR ip, PCSTR port) {
	int retCd;

	//init winsocket
	WSADATA wsaData;
	retCd = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (retCd != 0) {
		printf("Error on WSAStartup: %d \n", retCd);
		return retCd;
	}

	//get addr info
	addrinfo hints;
	memset(&hints, 0, sizeof(hints)); //cleanup garbage in struc
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	addrinfo *addrInfo;
	retCd = getaddrinfo(nullptr, port, &hints, &addrInfo);
	if (retCd != 0) {
		printf("Error on getAddrInfo: %d \n", retCd);
		WSACleanup();
		return retCd;
	}

	//create socket
	SOCKET listenSocket = INVALID_SOCKET;
	listenSocket = socket(addrInfo->ai_family, addrInfo->ai_socktype, addrInfo->ai_protocol);
	if (listenSocket == INVALID_SOCKET) {
		retCd = WSAGetLastError();
		printf("Error in socket: %ld \n", retCd);
		freeaddrinfo(addrInfo);
		WSACleanup();
		return retCd;
	}

	//bind
	retCd = bind(listenSocket, addrInfo->ai_addr, addrInfo->ai_addrlen);
	if (retCd == SOCKET_ERROR) {
		retCd = WSAGetLastError();
		printf("Error in bind %d \n", retCd);
		closesocket(listenSocket);
		freeaddrinfo(addrInfo);
		WSACleanup();
		return retCd;
	}

	//early free addrinfo
	freeaddrinfo(addrInfo);

	//listen
	retCd = listen(listenSocket, 2);
	if (retCd == SOCKET_ERROR) {
		retCd = WSAGetLastError();
		printf("Error in listen %d \n", retCd);
		closesocket(listenSocket);
		WSACleanup();
		return retCd;
	}

	//accept connection
	SOCKET clientSocket;
	clientSocket = accept(listenSocket, nullptr, nullptr);
	if (retCd == INVALID_SOCKET) {
		retCd = WSAGetLastError();
		printf("Error in accept: %d \n", retCd);
		closesocket(listenSocket);
		WSACleanup();
		return retCd;
	}

	//recv
	printf("Ready to listen \n");
	char receiveBuffer[DEFAULT_BUFFER_LENGTH];
	std::string input;
	do
	{
		memset(&receiveBuffer, 0, sizeof(hints));
		retCd = recv(clientSocket, receiveBuffer, DEFAULT_BUFFER_LENGTH, 0);
		if (retCd == SOCKET_ERROR) {
			retCd = WSAGetLastError();
			printf("Error in recv %d \n", retCd);
			closesocket(clientSocket);
			WSACleanup();
			return retCd;
		}
		if (retCd > 0) {
			input = receiveBuffer;
			std::cout << input.substr(0, retCd) << std::endl;
		}
	} while (retCd > 0);

	//shutdown
	retCd = shutdown(clientSocket, SD_BOTH);
	if (retCd == SOCKET_ERROR) {
		retCd = WSAGetLastError();
		printf("Error in shutdown client %d \n", retCd);
		closesocket(clientSocket);
		WSACleanup();
		return retCd;
	}

	retCd = shutdown(listenSocket, SD_BOTH);
	if (retCd == SOCKET_ERROR) {
		retCd = WSAGetLastError();
		switch (retCd)
		{
		case 10057:
			printf("Listen socket already closed");
			retCd = 0;
			break;
		default:
			printf("Error in shutdown listen %d \n", retCd);
			break;
		}
		closesocket(listenSocket);
		WSACleanup();
		return retCd;
	}

	//cleanup
	closesocket(listenSocket);
	WSACleanup();

	//allok
	return 0;
}
