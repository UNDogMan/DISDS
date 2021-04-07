#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include "Winsock2.h" //заголовок WS2_32.dll
#pragma comment(lib, "WS2_32.lib") //экспорт WS2_32.dll
#include <ctime>
#include <fstream>
#include <sys/timeb.h>

#define TIMESERVER_TYPE 2
#define CLOCK_TYPE 1

using namespace std;

SOCKET cC;
SOCKET cCUDP;

SOCKADDR_IN server;
int ls;

int Tc = 0;
int timeOut = 1000;
int messageCount = 10;
char cmd[4];
int type;

HANDLE hTimeSync;
bool endWork = false;


string itos(int i) {
	stringstream s;
	s << i;
	return s.str();
}

string GetErrorMsgText(int code) {
	string msgText;
	switch (code) {
	case WSAEINTR: msgText = "WSAEINTR";  break;
	case WSAEACCES: msgText = "WSAEACCES";  break;
	case WSAEFAULT: msgText = "WSAEFAULT";  break;
	case WSAEINVAL: msgText = "WSAEINVAL";  break;
	case WSAEMFILE: msgText = "WSAEMFILE";  break;
	case WSAEWOULDBLOCK: msgText = "WSAEWOULDBLOCK";  break;
	case WSAEINPROGRESS: msgText = "WSAEINPROGRESS";  break;
	case WSAEALREADY: msgText = "WSAEALREADY";  break;
	case WSAENOTSOCK: msgText = "WSAENOTSOCK";  break;
	case WSAEDESTADDRREQ: msgText = "WSAEDESTADDRREQ";  break;
	case WSAEMSGSIZE: msgText = "WSAEMSGSIZE";  break;
	case WSAEPROTOTYPE: msgText = "WSAEPROTOTYPE";  break;
	case WSAENOPROTOOPT: msgText = "WSAENOPROTOOPT";  break;
	case WSAEPROTONOSUPPORT: msgText = "WSAEPROTONOSUPPORT";  break;
	case WSAESOCKTNOSUPPORT: msgText = "WSAESOCKTNOSUPPORT";  break;
	case WSAEOPNOTSUPP: msgText = "WSAEOPNOTSUPP";  break;
	case WSAEPFNOSUPPORT: msgText = "WSAEPFNOSUPPORT";  break;
	case WSAEAFNOSUPPORT: msgText = "WSAEAFNOSUPPORT";  break;
	case WSAEADDRINUSE: msgText = "WSAEADDRINUSE";  break;
	case WSAEADDRNOTAVAIL: msgText = "WSAEADDRNOTAVAIL";  break;
	case WSAENETDOWN: msgText = "WSAENETDOWN";  break;
	case WSAENETUNREACH: msgText = "WSAENETUNREACH";  break;
	case WSAENETRESET: msgText = "WSAENETRESET";  break;
	case WSAECONNABORTED: msgText = "WSAECONNABORTED";  break;
	case WSAECONNRESET: msgText = "WSAECONNRESET";  break;
	case WSAENOBUFS: msgText = "WSAENOBUFS";  break;
	case WSAEISCONN: msgText = "WSAEISCONN";  break;
	case WSAENOTCONN: msgText = "WSAENOTCONN";  break;
	case WSAESHUTDOWN: msgText = "WSAESHUTDOWN";  break;
	case WSAETIMEDOUT: msgText = "WSAETIMEDOUT";  break;
	case WSAECONNREFUSED: msgText = "WSAECONNREFUSED";  break;
	case WSAEHOSTDOWN: msgText = "WSAEHOSTDOWN";  break;
	case WSAEHOSTUNREACH: msgText = "WSAEHOSTUNREACH";  break;
	case WSAEPROCLIM: msgText = "WSAEPROCLIM";  break;
	case WSASYSNOTREADY: msgText = "WSASYSNOTREADY";  break;
	case WSAVERNOTSUPPORTED: msgText = "WSAVERNOTSUPPORTED";  break;
	case WSANOTINITIALISED: msgText = "WSANOTINITIALISED";  break;
	case WSAEDISCON: msgText = "WSAEDISCON";  break;
	case WSATYPE_NOT_FOUND: msgText = "WSATYPE_NOT_FOUND";  break;
	case WSAHOST_NOT_FOUND: msgText = "WSAHOST_NOT_FOUND";  break;
	case WSATRY_AGAIN: msgText = "WSATRY_AGAIN";  break;
	case WSANO_RECOVERY: msgText = "WSANO_RECOVERY";  break;
	case WSANO_DATA: msgText = "WSANO_DATA";  break;
	case WSA_INVALID_HANDLE: msgText = "WSA_INVALID_HANDLE";  break;
	case WSA_INVALID_PARAMETER: msgText = "WSA_INVALID_PARAMETER";  break;
	case WSA_IO_INCOMPLETE: msgText = "WSA_IO_INCOMPLETE";  break;
	case WSA_IO_PENDING: msgText = "WSA_IO_PENDING";  break;
	case WSA_NOT_ENOUGH_MEMORY: msgText = "WSA_NOT_ENOUGH_MEMORY";  break;
	case WSA_OPERATION_ABORTED: msgText = "WSA_OPERATION_ABORTED";  break;
		/*case WSAINVALIDPROCTABLE: msgText = "WSAINVALIDPROCTABLE";  break;
		case WSAINVALIDPROVIDER: msgText = "WSAINVALIDPROVIDER";  break;
		case WSAPROVIDERFAILEDINIT: msgText = "WSAPROVIDERFAILEDINIT";  break;*/
	case WSASYSCALLFAILURE: msgText = "WSASYSCALLFAILURE";  break;
	default: msgText = "***ERROR***" + itos(code);  break;
	}
	return msgText;
}

string SetErrorMsgText(string msgText, int code) {
	return msgText + GetErrorMsgText(code);
}

struct GETSINCRO {
	char cmd[4];
	int curvalue;
};

struct SETSINCRO {
	char cmd[4];
	int correction;
};

char* get_message(int msg)
{
	char echo[] = "Echo";
	char time[] = "Time";
	char rand[] = "0001";
	char r[] = "";
	switch (msg)
	{
	case 1: 	return echo;
	case 2: 	return  time;
	case 3: 	return  rand;
	default:
		return r;
	}
}

bool GetServer(char* call, short port, SOCKADDR_IN* from, int* flen) {
	SOCKET sS;
	if ((sS = socket(AF_INET, SOCK_DGRAM, NULL)) == INVALID_SOCKET)
		throw SetErrorMsgText("GRFC socket:", WSAGetLastError());

	int opt = 1;
	if (setsockopt(sS, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(int)) == SOCKET_ERROR)
		throw SetErrorMsgText("setsockopt:", WSAGetLastError());

	SOCKADDR_IN all;
	all.sin_family = AF_INET;
	all.sin_port = htons(port);
	all.sin_addr.s_addr = INADDR_BROADCAST;

	SOCKADDR_IN serv;
	int ls = sizeof(serv);

	int lobuf;
	if ((lobuf = sendto(sS, call, 32, NULL, (sockaddr*)&all, sizeof(all))) == SOCKET_ERROR)
		throw SetErrorMsgText("sendto:", WSAGetLastError());
	int libuf;
	char bfrom[10000];
	if ((libuf = recvfrom(sS, bfrom, sizeof(bfrom), NULL, (sockaddr*)&serv, &ls)) == SOCKET_ERROR) {
		if (WSAGetLastError() == WSAETIMEDOUT) {
			memset(from, 0, sizeof(*from));
			return false;
		}
		throw SetErrorMsgText("GRFC recvfrom:", WSAGetLastError());
	}
	if (strcmp(bfrom, "OK") == 0) {
		*from = serv;
		*flen = ls;
		if (closesocket(sS) == SOCKET_ERROR)
			throw SetErrorMsgText("GRFC socket:", WSAGetLastError());
		return true;
	}
	if (closesocket(sS) == SOCKET_ERROR)
		throw SetErrorMsgText("GRFC socket:", WSAGetLastError());
	return false;
}

DWORD WINAPI TimeSync(LPVOID msgCount) {
	cout << "TimeSync Start" << endl;
	if ((cCUDP = socket(AF_INET, SOCK_DGRAM, NULL)) == INVALID_SOCKET)
		throw SetErrorMsgText("GRFC socket:", WSAGetLastError());

	int port = ntohs(server.sin_port);
	SOCKADDR_IN servTime;
	servTime.sin_family = AF_INET;
	servTime.sin_port = htons(port + 1);
	servTime.sin_addr = server.sin_addr;
	cout << "\TimeSync Server " << inet_ntoa(servTime.sin_addr) << ":" << htons(servTime.sin_port) << endl;
	SOCKADDR_IN serv;
	int ls = sizeof(serv);

	SOCKADDR_IN from;
	int lf;
	int i = 0;
	int maxCorrection = INT_MIN;
	int minCorrection = INT_MAX;
	int sum = 0;
	int supSum = 0;

	timeb localTime;

	int lobuf;
	int libuf;
	struct timeb t;

	SYSTEMTIME systemTime;
	ftime(&t);
	if (type == 2)
		Tc = (1000 * t.time + t.millitm);
	GetSystemTime(&systemTime);

	while ((i < *(int *)msgCount || *(int *)msgCount < 0) && !endWork) {
		GETSINCRO gsinc;
		memset(&gsinc, 0, sizeof(gsinc));

		SETSINCRO ssinc;
		memset(&ssinc, 0, sizeof(ssinc));

		gsinc.curvalue = Tc;
		strcpy(gsinc.cmd, cmd);

		SYSTEMTIME systemTime;
		ftime(&t);
		GetSystemTime(&systemTime);

		if ((lobuf = sendto(cCUDP, (char*)&gsinc, sizeof(gsinc), NULL, (sockaddr*)&servTime, sizeof(servTime))) == SOCKET_ERROR)
			throw SetErrorMsgText("sendto:", WSAGetLastError());

		SOCKADDR_IN from;
		memset(&from, 0, sizeof(from));
		int lc = sizeof(from);

		if (libuf = recvfrom(cCUDP, (char*)&ssinc, sizeof(ssinc), NULL, (sockaddr*)&from, &lc) == SOCKET_ERROR)
			throw SetErrorMsgText("recvfrom:", WSAGetLastError());
		sum += ssinc.correction;
		
		if (maxCorrection < ssinc.correction)
			maxCorrection = ssinc.correction;

		if (minCorrection > ssinc.correction)
			minCorrection = ssinc.correction;

		Tc = Tc + ssinc.correction + timeOut;
		supSum += Tc - (1000 * t.time + t.millitm);

		std::cout << i++ << ") Correction: " << ssinc.correction << " Min: "
			<< minCorrection << " Max: " << maxCorrection << endl;

		systemTime.wMilliseconds += ssinc.correction;
		SetSystemTime(&systemTime);

		std::cout << " DataTime " << systemTime.wMonth
			<< "." << systemTime.wDay
			<< ".2021" << endl << " Hours "
			<< systemTime.wHour + 3
			<< ":" << systemTime.wMinute
			<< ":" << systemTime.wSecond
			<< "." << systemTime.wMilliseconds << " " << endl << endl;

		Sleep(timeOut);
	}
	std::cout << "---------------------------------" << endl;
	std::cout << "AVG: " << sum / i << " Min: " << minCorrection << " Max: " << maxCorrection << endl << endl;
	if(type == 2)
		std::cout << "AVG: Cc - OSTime: " << supSum / i << endl;
	if (closesocket(cCUDP) == SOCKET_ERROR)
		throw SetErrorMsgText("GRFC socket:", WSAGetLastError());
	return 0;
}

int main()
{
	WSADATA wsaData;
	setlocale(0, "rus");
	try
	{
		if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
			throw  SetErrorMsgText("Startup:", WSAGetLastError());

		if ((cC = socket(AF_INET, SOCK_STREAM, NULL)) == INVALID_SOCKET)
			throw  SetErrorMsgText("socket:", WSAGetLastError());
		if (!GetServer((char*)"Hello", 2000, &server, &ls)) {
			throw  SetErrorMsgText("GetServer:", WSAGetLastError());
		}

		char message[50];
		int  libuf = 0, lobuf = 0, service;
		cout << "Enter timeout\n";
		scanf("%d", &timeOut);
		cout << "Choose\n1 - Clock\n2 - NTP\n";
		scanf("%d", &service);

		if (service == 1) {
			strcpy(cmd, "CLC");
			type = 1;
		}
		else {
			strcpy(cmd, "NTP");
			type = 2;
		}

		cout << "\Find Server " << inet_ntoa(server.sin_addr) << ":" << htons(server.sin_port) << endl;
		hTimeSync = CreateThread(NULL, NULL, TimeSync, (LPVOID)&messageCount, NULL, NULL);

		cout << "Choose\n1 - Echo\n2 - Time\n3 - Random\n";
		scanf("%d", &service);

		char* outMessage = new char[5];
		strcpy(outMessage, get_message(service));

		if ((connect(cC, (sockaddr*)&server, sizeof(server))) == SOCKET_ERROR)
			throw  SetErrorMsgText("connect:", WSAGetLastError());

		if ((lobuf = send(cC, outMessage, strlen(outMessage) + 1, NULL)) == SOCKET_ERROR)
			throw  SetErrorMsgText("send:", WSAGetLastError());

		printf("send: %s\n", outMessage);

		if ((libuf = recv(cC, message, sizeof(message), NULL)) == SOCKET_ERROR)
			throw  SetErrorMsgText("recv:", WSAGetLastError());

		if (!strcmp(message, "TimeOUT")) {
			cout << "Time out\n";
			return -1;
		}
		if (service == 1)
		{
			for (int j = 10; j >= 0; --j) {
				Sleep(1000);
				sprintf(outMessage, "%d", j);
				if ((lobuf = send(cC, outMessage, strlen(outMessage) + 1, NULL)) == SOCKET_ERROR)
					throw  SetErrorMsgText("send:", WSAGetLastError());

				printf("send: %s\n", outMessage);

				if ((libuf = recv(cC, message, sizeof(message), NULL)) == SOCKET_ERROR)
					throw  SetErrorMsgText("recv:", WSAGetLastError());

				printf("receive: %s\n", message);
			}
		}
		else if (service == 2 || service == 3) {
			printf("receive: %s\n", message);
		}
		else {
			printf("receive: %s\n", message);
		}
		if (closesocket(cC) == SOCKET_ERROR)
			throw  SetErrorMsgText("closesocket:", WSAGetLastError());
		endWork = true;
		WaitForSingleObject(hTimeSync, INFINITE);
		CloseHandle(hTimeSync);

		if (WSACleanup() == SOCKET_ERROR)
			throw  SetErrorMsgText("Cleanup:", WSAGetLastError());
	}
	catch (string errorMsgText)
	{
		cout << endl << errorMsgText;
	}
	system("pause");
	return 0;
}