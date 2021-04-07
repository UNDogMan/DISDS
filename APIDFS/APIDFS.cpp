#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include <ctime>
#include <fstream>
#include <sys/timeb.h>


#include "Winsock2.h" //заголовок WS2_32.dll
#pragma comment(lib, "WS2_32.lib") //экспорт WS2_32.dll
#include "CA.h"
#include "DFS.h"
#include "WSAErrors.h"

using namespace std;

SOCKADDR_IN server;
int ls;

bool GetServer(char* call, short port, SOCKADDR_IN* from, int* flen) {
	SOCKET sS;
	if ((sS = socket(AF_INET, SOCK_DGRAM, NULL)) == INVALID_SOCKET)
		throw WSAErrors::SetErrorMsgText("GRFC socket:", WSAGetLastError());

	int opt = 1;
	if (setsockopt(sS, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(int)) == SOCKET_ERROR)
		throw WSAErrors::SetErrorMsgText("setsockopt:", WSAGetLastError());

	SOCKADDR_IN all;
	all.sin_family = AF_INET;
	all.sin_port = htons(port);
	all.sin_addr.s_addr = INADDR_BROADCAST;

	SOCKADDR_IN serv;
	int ls = sizeof(serv);

	int lobuf;
	if ((lobuf = sendto(sS, call, 32, NULL, (sockaddr*)&all, sizeof(all))) == SOCKET_ERROR)
		throw WSAErrors::SetErrorMsgText("sendto:", WSAGetLastError());
	int libuf;
	char bfrom[10000];
	if ((libuf = recvfrom(sS, bfrom, sizeof(bfrom), NULL, (sockaddr*)&serv, &ls)) == SOCKET_ERROR) {
		if (WSAGetLastError() == WSAETIMEDOUT) {
			memset(from, 0, sizeof(*from));
			return false;
		}
		throw WSAErrors::SetErrorMsgText("GRFC recvfrom:", WSAGetLastError());
	}
	if (strcmp(bfrom, "OK") == 0) {
		*from = serv;
		*flen = ls;
		if (closesocket(sS) == SOCKET_ERROR)
			throw WSAErrors::SetErrorMsgText("GRFC socket:", WSAGetLastError());
		return true;
	}
	if (closesocket(sS) == SOCKET_ERROR)
		throw WSAErrors::SetErrorMsgText("GRFC socket:", WSAGetLastError());
	return false;
}

void main() {
	WSADATA wsaData;
	setlocale(0, "rus");
	try {
		if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
			throw  WSAErrors::SetErrorMsgText("Startup:", WSAGetLastError());

		char text[100];
		char hostname[20];
		gethostname(hostname, 20);
		char buf[2048];
		int i = 0;
		time_t rawtime;
		struct tm* timeinfo;
		DFS::HDFS hdfs = DFS::OpenDFSFile((char *)"\\\\DESKTOP-FEESGC8\\Release\\all.txt");
		for (int i = 0; i < 10; ++i) {
			ZeroMemory(text, 100);
			time(&rawtime);
			timeinfo = localtime(&rawtime);
			strftime(text, 100, "%c", timeinfo);
			strcat(text, " ");
			strcat(text, hostname);
			strcat(text, "\n");
			DFS::WriteDFSFile(hdfs, text, strlen(text) + 1);
			Sleep(300);
		}
		cout << "Butes readed: " << (i = DFS::ReadDFSFile(hdfs, (void*)buf, 2048)) << endl;
		for (int j = 0; j < i; ++j)
			cout << buf[j];
		cout << endl;
		system("pause");
		if (WSACleanup() == SOCKET_ERROR)
			throw  WSAErrors::SetErrorMsgText("Cleanup:", WSAGetLastError());
	}
	catch (string errorMsgText)
	{
		cout << endl << errorMsgText;
	}

}