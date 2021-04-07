#pragma once
#include "CA.h"

namespace DFS {
	struct HDFS {
		LPWSTR filename;
		CA::CA ca;
	};

	HDFS OpenDFSFile(char* FileName);
	int ReadDFSFile(HDFS hdfs, void* buf, int bufsize);
	int WriteDFSFile(HDFS hdfs, void* buf, int bufsize);
	bool GetServer(char* call, short port, SOCKADDR_IN* from, int* flen);
}

bool DFS::GetServer(char* call, short port, SOCKADDR_IN* from, int* flen) {
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

DFS::HDFS DFS::OpenDFSFile(char* FileName)
{
	SOCKADDR_IN server;
	int ls;
	DFS::HDFS hdfs;
	wchar_t* wtext = new wchar_t[20];
	mbstowcs(wtext, FileName, strlen(FileName) + 1);//Plus null
	LPWSTR ptr = wtext;
	hdfs.filename = wtext;
	WSADATA wsaData;
	try {
		if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
			throw  WSAErrors::SetErrorMsgText("Startup:", WSAGetLastError());
		if (!DFS::GetServer((char*)"Hello", 2000, &server, &ls)) {
			throw  WSAErrors::SetErrorMsgText("GetServer:", WSAGetLastError());
		}
		hdfs.ca = CA::InitCA(inet_ntoa(server.sin_addr), FileName);
		if (WSACleanup() == SOCKET_ERROR)
			throw  WSAErrors::SetErrorMsgText("Cleanup:", WSAGetLastError());
	}
	catch (string errorMsgText)
	{
		cout << endl << errorMsgText;
	}
	return hdfs;
}

int DFS::ReadDFSFile(HDFS hdfs, void* buf, int bufsize)
{
	DWORD read;
	CA::EnterCA(hdfs.ca);
	HANDLE file = CreateFile(hdfs.filename, GENERIC_WRITE | GENERIC_READ, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	BOOL ans = ReadFile(file, buf, bufsize, &read, NULL);
	CloseHandle(file);
	CA::LeaveCA(hdfs.ca);
	return ans ? read : -1;
}

int DFS::WriteDFSFile(HDFS hdfs, void* buf, int bufsize)
{
	DWORD read;
	CA::EnterCA(hdfs.ca);
	HANDLE file = CreateFile(hdfs.filename, GENERIC_WRITE | GENERIC_READ, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		std::cout << "WriteDFSFile:CreateFile::"  << GetLastError();
	}
	SetFilePointer(file, 0, 0, FILE_END);
	BOOL ans = WriteFile(file, buf, bufsize, &read, NULL);
	CloseHandle(file);
	CA::LeaveCA(hdfs.ca);
	return ans ? read : -1;
}