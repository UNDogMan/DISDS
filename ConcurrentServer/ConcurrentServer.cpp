#pragma ones
#include <sstream>
#include <string>
#include <unordered_map>
#include <queue>
#include <iostream>
#include <list>
#include <ctime>
#include <WS2tcpip.h>
#include <fstream>
#include <sys\timeb.h> 

#include "CA.h"

#include "Winsock2.h"
#include "WSAErrors.h"

#pragma comment(lib, "WS2_32.lib") 
#pragma warning(disable:4996)

#define AS_SQUIRT 10
using namespace std;

typedef unordered_map<string, list<u_long>> MAP;

CRITICAL_SECTION scListContact;

volatile long accepted = 0, failed = 0, finished = 0, active = 0;

SOCKET sS;
SOCKET sSUDP;
SOCKET sSTUDP;
SOCKET sSCAUDP;
int serverPort = 2000;
char* sIP;

char dll[50];
char namedPipe[10];
HANDLE(*ts)(char*, LPVOID);
HMODULE st;

HANDLE hAcceptServer, hConsolePipe, hGarbageCleaner, hDispatchServer, hResponseServer, hTimeServer, hNtpSincroServer, hCritticalSectionServer;
HANDLE hClientConnectedEvent = CreateEvent(NULL, FALSE, FALSE, L"ClientConnected");

DWORD WINAPI AcceptServer(LPVOID pPrm);
DWORD WINAPI ConsolePipe(LPVOID pPrm);
DWORD WINAPI GarbageCleaner(LPVOID pPrm);
DWORD WINAPI DispatchServer(LPVOID pPrm);
DWORD WINAPI ResponseServer(LPVOID pPrm);

enum TalkersCommand {
	START, STOP, EXIT, STATISTICS, WAIT, SHUTDOWN, GETCOMMAND
};
volatile TalkersCommand  previousCommand = GETCOMMAND;

struct Contact // элемент списка подключений
{
	enum TE { // состояние сервера подключения
		EMPTY, // пустой элемент списка подключений
		ACCEPT, // подключен (accept), но не обслуживается
		CONTACT // передан обслуживающему серверу
	} type; // тип элемента списка подключений

	enum ST { // состояние обслуживающего сервера
		WORK, // идет обмен данными с клиентом
		ABORT, // обслуживающий сервер завершился не нормально
		TIMEOUT, // обслуживающий сервер завершился по времени
		FINISH // обслуживающий сервер завершился нормально
	} sthread; // состояние обслуживающего сервера (потока)

	SOCKET s; // сокет для обмена данными с клиентом
	SOCKADDR_IN prms; // параметры сокета
	int lprms; // длина prms
	HANDLE hthread; // handle потока (или процесса)
	HANDLE htimer; // handle таймера
	HANDLE serverHThtead;// handle обслуживающего сервера который в последствие может зависнуть

	char msg[50]; // сообщение
	char srvname[15]; // наименование обслуживающего сервера

	Contact(TE t = EMPTY, const char* namesrv = "") // конструктор
	{
		memset(&prms, 0, sizeof(SOCKADDR_IN));
		lprms = sizeof(SOCKADDR_IN);
		type = t;
		strcpy(srvname, namesrv);
		msg[0] = 0;
	};

	void SetST(ST sth, const char* m = "")
	{
		sthread = sth;
		strcpy(msg, m);
	}
};
typedef list<Contact> ListContact; // список подключений 
ListContact contacts;

struct GETSINCRO {
	char cmd[4];
	int curvalue;
};

struct SETSINCRO {
	char cmd[4];
	int correction;
};

struct NTP_packet	//пакет NTP
{
	CHAR head[4];
	DWORD32 RootDelay;
	DWORD32 RootDispersion;
	CHAR ReferenceIdentifier[4];	//идентификатор
	DWORD ReferenceTimestamp[2];	//32 бита  - секунды с 01.01.1900 00:00, 32 бита - доли секунды в 2^-32 единицах
	DWORD64 OriginateTimestamp;
	DWORD32 TransmitTimestamp[2];	//32 бита  - секунды с 01.01.1900 00:00, 32 бита - доли секунды в 2^-32 единицах
	DWORD32 KeyIdentifier;			//optional
	DWORD64 MessageDigest[2];		//optional
};

void UnixTimeToFileTime(time_t t, LPFILETIME pft)
{
	// Note that LONGLONG is a 64-bit value
	LONGLONG ll;

	ll = Int32x32To64(t, 10000000) + 116444736000000000;
	pft->dwLowDateTime = (DWORD)ll;
	pft->dwHighDateTime = ll >> 32;
}

void UnixTimeToSystemTime(time_t t, LPSYSTEMTIME pst)
{
	FILETIME ft;

	UnixTimeToFileTime(t, &ft);
	FileTimeToSystemTime(&ft, pst);
}

unsigned long long getTimeFromGlobalServer() {
	int h = CLOCKS_PER_SEC;
	clock_t t = clock();
	int d = 613608 * 3600;
	time_t ttime;
	time(&ttime);

	WSAData wsaData;
	SOCKET s;
	SOCKADDR_IN server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("88.147.254.235");
	server.sin_port = htons(123);

	NTP_packet out_buf, in_buf;
	ZeroMemory(&out_buf, sizeof(out_buf));
	ZeroMemory(&in_buf, sizeof(in_buf));
	out_buf.head[0] = 0x1B;
	out_buf.head[1] = 0x00;
	out_buf.head[2] = 4;
	out_buf.head[3] = 0xEC;

	unsigned long long ms;

	SYSTEMTIME sysTime;

	try
	{
		if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
			throw WSAGetLastError();
		if ((s = socket(AF_INET, SOCK_DGRAM, NULL)) == INVALID_SOCKET)
			throw WSAGetLastError();
		int lenout = 0, lenin = 0, lensockaddr = sizeof(server);

		if ((lenout = sendto(s, (char*)&out_buf, sizeof(out_buf), NULL, (sockaddr*)&server, sizeof(server))) == SOCKET_ERROR)
			throw  WSAGetLastError();
		if ((lenin = recvfrom(s, (char*)&in_buf, sizeof(in_buf), NULL, (sockaddr*)&server, &lensockaddr)) == SOCKET_ERROR)
			throw  WSAGetLastError();

		in_buf.ReferenceTimestamp[0] = ntohl(in_buf.ReferenceTimestamp[0]) - d;
		in_buf.TransmitTimestamp[0] = ntohl(in_buf.TransmitTimestamp[0]) - d;
		in_buf.TransmitTimestamp[1] = ntohl(in_buf.TransmitTimestamp[1]);
		ms = (int)1000.0 * ((double)(in_buf.TransmitTimestamp[1]) / (double)0xffffffff);

		if (closesocket(s) == INVALID_SOCKET)
			throw WSAGetLastError();
		if (WSACleanup() == SOCKET_ERROR)
			throw WSAGetLastError();

	}
	catch (int e)
	{
		std::cout << "getTimeFromGlobalServer error: " << e << std::endl;
	}

	return (unsigned long)in_buf.TransmitTimestamp[0] * 1000 + ms;
}

bool AcceptCycle(int squirt)
{
	bool rc = false;
	Contact c(Contact::ACCEPT, "EchoServer");
	while (squirt-- > 0 && !rc)
	{
		if ((c.s = accept(sS,
			(sockaddr*)&c.prms, &c.lprms)) == INVALID_SOCKET)
		{
			if (WSAGetLastError() != WSAEWOULDBLOCK)
				throw WSAErrors::SetErrorMsgText("accept:", WSAGetLastError());
		}
		else
		{
			rc = true;
			EnterCriticalSection(&scListContact);
			contacts.push_front(c);
			LeaveCriticalSection(&scListContact);
			puts("contact connected");
			InterlockedIncrement(&accepted);
			InterlockedIncrement(&active);
		}
	}
	return rc;
};

void OpenSocket() {
	SOCKADDR_IN serv, clnt;
	u_long nonblk = 1;

	if ((sS = socket(AF_INET, SOCK_STREAM, NULL)) == INVALID_SOCKET)
		throw WSAErrors::SetErrorMsgText("", WSAGetLastError());

	int lclnt = sizeof(clnt);
	serv.sin_family = AF_INET;
	serv.sin_port = htons(serverPort);
	serv.sin_addr.s_addr = INADDR_ANY;

	if (bind(sS, (LPSOCKADDR)&serv, sizeof(serv)) == SOCKET_ERROR)
		throw  WSAErrors::SetErrorMsgText("bind:", WSAGetLastError());
	//cout << "OpenSocket " << inet_ntoa(serv.sin_addr) << ":" << htons(serv.sin_port) << endl;
	if (listen(sS, SOMAXCONN) == SOCKET_ERROR)
		throw  WSAErrors::SetErrorMsgText("listen:", WSAGetLastError());
	if (ioctlsocket(sS, FIONBIO, &nonblk) == SOCKET_ERROR)
		throw WSAErrors::SetErrorMsgText("ioctlsocket:", WSAGetLastError());
}

void CloseSocket() {
	if (closesocket(sS) == SOCKET_ERROR)
		throw WSAErrors::SetErrorMsgText("closesocket", WSAGetLastError());
}

void CommandsCycle(TalkersCommand& cmd) {
	int squirt = 0;
	while (cmd != EXIT)
	{
		switch (cmd)
		{
		case START: 
			cmd = GETCOMMAND;
			cout << "START command\n";
			if (previousCommand != START) {
				squirt = AS_SQUIRT;
				OpenSocket();
				previousCommand = START;
			}
			break;
		case STOP: 
			cmd = GETCOMMAND;
			cout << "STOP command\n";
			if (previousCommand != STOP) {
				squirt = 0;
				CloseSocket();
				previousCommand = STOP;
			}
			break;
		case WAIT:
			squirt = 0;
			cout << "WAIT command\n";
			CloseSocket();
			while (contacts.size() != 0);
			cmd = START;
			previousCommand = WAIT;
			break;
		case SHUTDOWN:
			squirt = 0;
			cout << "SHUTDOWN command\n";
			CloseSocket();
			while (contacts.size() != 0);
			cmd = EXIT;
			break;
		};
		if (cmd != STOP) {
			if (AcceptCycle(squirt)) //цикл запрос/подключение (accept)
			{
				cmd = GETCOMMAND;
				SetEvent(hClientConnectedEvent);
			}
			else SleepEx(0, TRUE); // выполнить асинхронные процедуры
		}
	};

};

DWORD WINAPI AcceptServer(LPVOID pPrm)
{
	cout << "AcceptServer Start\n";
	DWORD rc = 0;
	WSADATA wsaData;
	try {
		if(WSAStartup(MAKEWORD(2, 0), &wsaData))
			throw  WSAErrors::SetErrorMsgText("AcceptServer WSAStartup:", WSAGetLastError());

		CommandsCycle(*((TalkersCommand*)pPrm));

		WSACleanup();
	}
	catch (string errorStr) {
		cout << errorStr << "\n";
	}
	cout << "AcceptServer Stop\n";
	ExitThread(rc);
}

void CALLBACK ASWTimer(LPVOID Prm, DWORD, DWORD) {
	Contact* contact = (Contact*)(Prm);
	cout << "ASWTimer is calling " << contact->hthread << endl;
	TerminateThread(contact->serverHThtead, NULL);
	send(contact->s, "TimeOUT", strlen("TimeOUT") + 1, NULL);
	EnterCriticalSection(&scListContact);
	CancelWaitableTimer(contact->htimer);
	contact->type = contact->EMPTY;
	contact->sthread = contact->TIMEOUT;
	LeaveCriticalSection(&scListContact);
}

DWORD WINAPI DispatchServer(LPVOID pPrm) {
	cout << "DispatchServer Start\n";
	
	int rc = 0;

	TalkersCommand& command = *(TalkersCommand*)pPrm;

	while (command != EXIT) {
		if (command != STOP) {
			WaitForSingleObject(hClientConnectedEvent, 5000);
			ResetEvent(hClientConnectedEvent);
			EnterCriticalSection(&scListContact);

			for (auto i = contacts.begin(); i != contacts.end(); ++i) {
				if (i->type == i->ACCEPT) {
					u_long nonblk = 0;
					if (ioctlsocket(i->s, FIONBIO, &nonblk) == SOCKET_ERROR)
						throw WSAErrors::SetErrorMsgText("ioctlsocket:", WSAGetLastError());

					char serviceType[5];
					clock_t start = clock();
					recv(i->s, serviceType, sizeof(serviceType), NULL);
					strcpy(i->msg, serviceType);
					clock_t delta = clock() - start;

					if (delta > 5000) {
						cout << "It's so long\n";
						i->sthread = i->TIMEOUT;
						if ((send(i->s, "TimeOUT", strlen("TimeOUT") + 1, NULL)) == SOCKET_ERROR)
							throw  WSAErrors::SetErrorMsgText("send:", WSAGetLastError());
						if (closesocket(i->s) == SOCKET_ERROR)
							throw  WSAErrors::SetErrorMsgText("closesocket:", WSAGetLastError());
						i->type = i->EMPTY;
					}
					else if (delta <= 5000) {
						if (strcmp(i->msg, "Echo") && strcmp(i->msg, "Time") && strcmp(i->msg, "0001")) {
							if ((send(i->s, "ErrorInquiry", strlen("ErrorInquiry") + 1, NULL)) == SOCKET_ERROR)
								throw  WSAErrors::SetErrorMsgText("send:", WSAGetLastError());
							i->sthread = i->ABORT;
							i->type = i->EMPTY;
							if (closesocket(i->s) == SOCKET_ERROR)
								throw  WSAErrors::SetErrorMsgText("closesocket:", WSAGetLastError());
						}
						else {
							i->type = i->CONTACT;
							i->hthread = hAcceptServer;
							i->serverHThtead = ts(serviceType, (LPVOID) & (*i));
							i->htimer = CreateWaitableTimer(0, FALSE, 0);
							LARGE_INTEGER Li;
							int seconds = 60;
							Li.QuadPart = -(10000000 * seconds);
							SetWaitableTimer(i->htimer, &Li, 0, ASWTimer, (LPVOID) & (*i), FALSE);
							SleepEx(0, TRUE);
						}
					}
				}
			}

			LeaveCriticalSection(&scListContact);
		}
	}
	cout << "DispatchServer Close\n";
	ExitThread(rc);
}

DWORD WINAPI GarbageCleaner(LPVOID pPrm)
{
	cout << "GarbageCleaner Start\n";

	DWORD rc = 0;
	while (*((TalkersCommand*)pPrm) != EXIT) {
		int listSize = 0;
		int howMuchClean = 0;
		if (contacts.size() != 0) {
			for (auto i = contacts.begin(); i != contacts.end();) {
				if (i->type == i->EMPTY) {
					EnterCriticalSection(&scListContact);
					if (i->sthread == i->FINISH)
						InterlockedIncrement(&finished);
					if (i->sthread == i->ABORT || i->sthread == i->TIMEOUT)
						InterlockedIncrement(&failed);
					i = contacts.erase(i);
					howMuchClean++;
					listSize = contacts.size();
					LeaveCriticalSection(&scListContact);
					Sleep(2000);
				}
				else ++i;
			}
		}
	}
	cout << "GarbageCleaner stop\n";
	ExitThread(rc);
}

TalkersCommand set_param(char* param) {
	if (!strcmp(param, "start")) return START;
	if (!strcmp(param, "stop")) return STOP;
	if (!strcmp(param, "exit")) return EXIT;
	if (!strcmp(param, "wait")) return WAIT;
	if (!strcmp(param, "shutdown")) return SHUTDOWN;
	if (!strcmp(param, "statistics")) return STATISTICS;
	if (!strcmp(param, "getcommand")) return GETCOMMAND;

}

DWORD WINAPI ConsolePipe(LPVOID pPrm)
{
	cout << "ConsolePipe started\n";

	DWORD rc = 0;
	char rbuf[100];
	DWORD dwRead, dwWrite;
	HANDLE hPipe;
	try
	{
		char NPname[50];
		sprintf(NPname, "\\\\.\\pipe\\%s", namedPipe);
		if ((hPipe = CreateNamedPipeA(NPname, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_WAIT, 1, NULL, NULL, INFINITE, NULL)) == INVALID_HANDLE_VALUE)
			throw WSAErrors::SetErrorMsgText("create:", GetLastError());
		if (!ConnectNamedPipe(hPipe, NULL))
			throw WSAErrors::SetErrorMsgText("connect:", GetLastError());
		TalkersCommand& param = *((TalkersCommand*)pPrm);
		while (param != EXIT) {
			cout << "--Connecting to Named Pipe Client--\n";
			ConnectNamedPipe(hPipe, NULL);
			while (ReadFile(hPipe, rbuf, sizeof(rbuf), &dwRead, NULL))
			{
				cout << "Client message:  " << rbuf << endl;
				param = set_param(rbuf);
				if (param == STATISTICS)
				{
					char stat[200];
					sprintf(stat, "\n----STATISTICS----\n"\
						"Connections count:    %d\n" \
						"Breaked:        %d\n" \
						"Complete successfully:             %d\n" \
						"Active connections: %d\n" \
						"-------------------------------------", accepted, failed, finished, contacts.size());
					WriteFile(hPipe, stat, sizeof(stat), &dwWrite, NULL);
				}
				if (param != STATISTICS)
					WriteFile(hPipe, rbuf, strlen(rbuf) + 1, &dwWrite, NULL);
			}
			cout << "--------------Pipe Closed-----------------\n";
			DisconnectNamedPipe(hPipe);
		}
		CloseHandle(hPipe);
		cout << "ConsolePipe breaked\n" << endl;
	}
	catch (string ErrorPipeText)
	{
		cout << endl << ErrorPipeText << endl;
	}
	ExitThread(rc);
}

bool PutAnswerToClient(char* name, sockaddr* to, int* lto) {

	char msg[] = "OK";
	if ((sendto(sSUDP, msg, sizeof(msg) + 1, NULL, to, *lto)) == SOCKET_ERROR)
		throw  WSAErrors::SetErrorMsgText("sendto:", WSAGetLastError());
	return false;
}

bool  GetRequestFromClient(char* name, short port, SOCKADDR_IN* from, int* flen)
{
	SOCKADDR_IN clnt;
	int lc = sizeof(clnt);
	ZeroMemory(&clnt, lc);
	char ibuf[500];
	int  lb = 0;
	int optval = 1;
	int TimeOut = 1000;
	setsockopt(sSUDP, SOL_SOCKET, SO_BROADCAST, (char*)&optval, sizeof(int));
	setsockopt(sSUDP, SOL_SOCKET, SO_RCVTIMEO, (char*)&TimeOut, sizeof(TimeOut));
	while (true) {
		if ((lb = recvfrom(sSUDP, ibuf, sizeof(ibuf), NULL, (sockaddr*)&clnt, &lc)) == SOCKET_ERROR) return false;
		if (!strcmp(name, ibuf)) {
			*from = clnt;
			*flen = lc;
			return true;
		}
		cout << "\nBad name\n";
	}
	return false;
}

DWORD WINAPI ResponseServer(LPVOID pPrm)
{
	cout << "ResponseServer started\n";

	DWORD rc = 0;
	WSADATA wsaData;
	SOCKADDR_IN serv;
	int TimeOut = 1000;
	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
		throw  WSAErrors::SetErrorMsgText("Startup:", WSAGetLastError());
	if ((sSUDP = socket(AF_INET, SOCK_DGRAM, NULL)) == INVALID_SOCKET)
		throw  WSAErrors::SetErrorMsgText("socket:", WSAGetLastError());
	serv.sin_family = AF_INET;
	serv.sin_port = htons(serverPort);
	serv.sin_addr.s_addr = INADDR_ANY;

	if (bind(sSUDP, (LPSOCKADDR)&serv, sizeof(serv)) == SOCKET_ERROR)
		throw  WSAErrors::SetErrorMsgText("bind:", WSAGetLastError());
	//cout << "ResponseServer " << inet_ntoa(serv.sin_addr) << ":" << htons(serv.sin_port) << endl;
	setsockopt(sSTUDP, SOL_SOCKET, SO_RCVTIMEO, (char*)&TimeOut, sizeof(TimeOut));

	SOCKADDR_IN someServer;
	int serverSize = sizeof(someServer);

	SOCKADDR_IN from;
	int lc = sizeof(from);
	ZeroMemory(&from, lc);
	int numberOfClients = 0;
	char* name = (char*)"Hello";

	while (*(TalkersCommand*)pPrm != EXIT)
	{
		try
		{
			while (*(TalkersCommand*)pPrm != STOP && *(TalkersCommand*)pPrm != EXIT) {
				if (GetRequestFromClient(name, serverPort, &from, &lc)) {
					printf("\nConnected Client: №%d, port: %d, ip: %s\n", ++numberOfClients, htons(from.sin_port), inet_ntoa(from.sin_addr));
					PutAnswerToClient(name, (sockaddr*)&from, &lc);
				}
			}
		}
		catch (string errorMsgText)
		{
			cout << endl << errorMsgText;
		}
	}
	if (closesocket(sSUDP) == SOCKET_ERROR)
		throw  WSAErrors::SetErrorMsgText("closesocket:", WSAGetLastError());
	if (WSACleanup() == SOCKET_ERROR)
		throw  WSAErrors::SetErrorMsgText("Cleanup:", WSAGetLastError());
	cout << "ResponseServer stop\n";
	ExitThread(rc);
}

DWORD WINAPI NtpSincroServer(LPVOID pPrm) {
	cout << "NtpSincroServer started\n";

	DWORD rc = 0;
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
		throw  WSAErrors::SetErrorMsgText("Startup:", WSAGetLastError());

	SOCKET s;
	SOCKADDR_IN server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("88.147.254.235");
	server.sin_port = htons(123);


	int h = CLOCKS_PER_SEC;
	int d = 613608 * 3600;

	if ((s = socket(AF_INET, SOCK_DGRAM, NULL)) == INVALID_SOCKET)
		throw WSAGetLastError();
	while (*(TalkersCommand*)pPrm != EXIT)
	{
		while (*(TalkersCommand*)pPrm != STOP && *(TalkersCommand*)pPrm != EXIT) {
			try
			{
				NTP_packet out_buf, in_buf;
				ZeroMemory(&out_buf, sizeof(out_buf));
				ZeroMemory(&in_buf, sizeof(in_buf));
				out_buf.head[0] = 0x1B;
				out_buf.head[1] = 0x00;
				out_buf.head[2] = 4;
				out_buf.head[3] = 0xEC;
				int lenout = 0, lenin = 0, lensockaddr = sizeof(server);

				if ((lenout = sendto(s, (char*)&out_buf, sizeof(out_buf), NULL, (sockaddr*)&server, sizeof(server))) == SOCKET_ERROR)
					throw  WSAGetLastError();
				if ((lenin = recvfrom(s, (char*)&in_buf, sizeof(in_buf), NULL, (sockaddr*)&server, &lensockaddr)) == SOCKET_ERROR)
					throw  WSAGetLastError();

				in_buf.ReferenceTimestamp[0] = ntohl(in_buf.ReferenceTimestamp[0]) - d;
				in_buf.TransmitTimestamp[0] = ntohl(in_buf.TransmitTimestamp[0]) - d;
				in_buf.TransmitTimestamp[1] = ntohl(in_buf.TransmitTimestamp[1]);
				int ms = (int)1000.0 * ((double)(in_buf.TransmitTimestamp[1]) / (double)0xffffffff);

				SYSTEMTIME sysTime;
				UnixTimeToSystemTime(in_buf.TransmitTimestamp[0], &sysTime);
				sysTime.wMilliseconds = ms;

				if (SetSystemTime(&sysTime)) {
					cout << "Time Is Successfully synced!" << endl;
				}
				else {
					cout << GetLastError();
				}

				Sleep(10000);
			}
			catch (string errorMsgText)
			{
				cout << endl << errorMsgText;
			}
		}
	}


	if (WSACleanup() == SOCKET_ERROR)
		throw  WSAErrors::SetErrorMsgText("Cleanup:", WSAGetLastError());
	cout << "NtpSincroServer stop\n";
	ExitThread(rc);
}

DWORD WINAPI TimeServer(LPVOID pPrm) {
	cout << "TimeServer started\n";

	DWORD rc = 0;
	WSADATA wsaData;
	SOCKADDR_IN serv;

	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
		throw  WSAErrors::SetErrorMsgText("Startup:", WSAGetLastError());
	if ((sSTUDP = socket(AF_INET, SOCK_DGRAM, NULL)) == INVALID_SOCKET)
		throw  WSAErrors::SetErrorMsgText("socket:", WSAGetLastError());

	serv.sin_family = AF_INET;
	serv.sin_port = htons(serverPort + 1);
	serv.sin_addr.s_addr = INADDR_ANY;

	if (bind(sSTUDP, (LPSOCKADDR)&serv, sizeof(serv)) == SOCKET_ERROR)
		throw  WSAErrors::SetErrorMsgText("bind:", WSAGetLastError());

	int TimeOut = 1000;
	setsockopt(sSUDP, SOL_SOCKET, SO_RCVTIMEO, (char*)&TimeOut, sizeof(TimeOut));

	int lf = 0;

	int maxCorrection = INT_MIN;
	int minCorrection = INT_MAX;
	int sum = 0;
	int i = 0;

	while (*(TalkersCommand*)pPrm != EXIT)
	{
		while (*(TalkersCommand*)pPrm != STOP && *(TalkersCommand*)pPrm != EXIT) {
			try
			{
				SOCKADDR_IN from;
				memset(&from, 0, sizeof(from));
				int lc = sizeof(from);

				GETSINCRO gsinc;
				memset(&gsinc, 0, sizeof(gsinc));

				SETSINCRO ssinc;
				memset(&ssinc, 0, sizeof(ssinc));

				int lobuf;
				int libuf;

				struct timeb t;
				SYSTEMTIME systemTime;
				ftime(&t);
				GetSystemTime(&systemTime);

				if ((libuf = recvfrom(sSTUDP, (char*)&gsinc, sizeof(gsinc), NULL, (sockaddr*)&from, &lc)) == SOCKET_ERROR) {
					if( WSAGetLastError() != WSAETIMEDOUT)
						throw WSAErrors::SetErrorMsgText("recvfrom:", WSAGetLastError());
				}
				else {
					cout << "Read command: " << gsinc.cmd << endl;
					if (gsinc.cmd[0] == 'C') {
						ssinc.correction = clock() - gsinc.curvalue;
						cout << "Income command: CLC\n";
					}
					else {
						ssinc.correction = (1000 * t.time + t.millitm) - gsinc.curvalue;
						cout << "Income command: NTP\n";
					}

					if ((lobuf = sendto(sSTUDP, (char*)&ssinc, sizeof(ssinc), NULL, (sockaddr*)&from, sizeof(from))) == SOCKET_ERROR)
						throw WSAErrors::SetErrorMsgText("sendto:", WSAGetLastError());
					if (maxCorrection < ssinc.correction)
						maxCorrection = ssinc.correction;

					if (minCorrection > ssinc.correction)
						minCorrection = ssinc.correction;
					sum += ssinc.correction;
					cout << i << ") Correction: " << ssinc.correction << " "
						<< "AVG: " << (double)sum / (double)(++i) << endl;
					cout << "DataTime "
						<< systemTime.wMonth << "."
						<< systemTime.wDay << ".2021" << " " << endl
						<< systemTime.wHour + 3 << ":"
						<< systemTime.wMinute << ":"
						<< systemTime.wSecond << "."
						<< systemTime.wMilliseconds << endl << endl;
				}
			}
			catch (string errorMsgText)
			{
				cout << endl << errorMsgText;
			}
		}
	}

	cout << "---------------------------------" << endl;
	cout << "AVG: " << sum / i << " Min: " << minCorrection << " Max: " << maxCorrection << endl << endl;

	if (closesocket(sSTUDP) == SOCKET_ERROR)
		throw  WSAErrors::SetErrorMsgText("closesocket:", WSAGetLastError());
	if (WSACleanup() == SOCKET_ERROR)
		throw  WSAErrors::SetErrorMsgText("Cleanup:", WSAGetLastError());
	cout << "TimeServer stop\n";
	ExitThread(rc);
}

DWORD WINAPI CritticalSectionServer(LPVOID pPrm) {
	cout << "CritticalSectionServer started\n";

	DWORD rc = 0;
	WSADATA wsaData;
	SOCKADDR_IN serv;
	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
		throw  WSAErrors::SetErrorMsgText("Startup:", WSAGetLastError());
	if ((sSCAUDP = socket(AF_INET, SOCK_DGRAM, NULL)) == INVALID_SOCKET)
		throw  WSAErrors::SetErrorMsgText("socket:", WSAGetLastError());

	serv.sin_family = AF_INET;
	serv.sin_port = htons(serverPort + 2);
	serv.sin_addr.s_addr = INADDR_ANY;

	if (bind(sSCAUDP, (LPSOCKADDR)&serv, sizeof(serv)) == SOCKET_ERROR)
		throw  WSAErrors::SetErrorMsgText("bind:", WSAGetLastError());

	int TimeOut = 2000;
	setsockopt(sSCAUDP, SOL_SOCKET, SO_RCVTIMEO, (char*)&TimeOut, sizeof(TimeOut));

	MAP map;

	while (*(TalkersCommand*)pPrm != EXIT)
	{
		while (*(TalkersCommand*)pPrm != STOP && *(TalkersCommand*)pPrm != EXIT) {
			try
			{
				SOCKADDR_IN from;
				memset(&from, 0, sizeof(from));
				int lc = sizeof(from);

				CA::CA caIN;
				memset(&caIN, 0, sizeof(caIN));

				CA::CA caOUT;
				memset(&caOUT, 0, sizeof(caOUT));

				int lobuf;
				int libuf;

				if ((libuf = recvfrom(sSCAUDP, (char*)&caIN, sizeof(caIN), NULL, (sockaddr*)&from, &lc)) == SOCKET_ERROR) {
					if (WSAGetLastError() != WSAETIMEDOUT)
						throw WSAErrors::SetErrorMsgText("recvfrom:", WSAGetLastError());
				}
				else {
					cout << "Recvfrom ";
					CA::print(caIN);
					caOUT = caIN;
					switch (caIN.status)
					{
					case CA::ENTER:
						if (map[caIN.resurce].empty()) {
							map[caIN.resurce].push_back(from.sin_addr.s_addr);
							caOUT.status = CA::ENTER;
							break;
						}
						if (!(std::find(map[caIN.resurce].begin(), map[caIN.resurce].end(), from.sin_addr.s_addr) != map[caIN.resurce].end())) {
							map[caIN.resurce].push_back(from.sin_addr.s_addr);
						}
						caOUT.status = CA::WAIT;
						break;
					case CA::LEAVE:
						if (map[caIN.resurce].front() == from.sin_addr.s_addr) {
							map[caIN.resurce].pop_front();
							caOUT.status = CA::LEAVE;
							break;
						}
						caOUT.status = CA::WAIT;
						break;
					default:
						break;
					}
					cout << "Sendto ";
					CA::print(caIN);
					if ((lobuf = sendto(sSCAUDP, (char*)&caOUT, sizeof(caOUT), NULL, (sockaddr*)&from, sizeof(from))) == SOCKET_ERROR)
						throw WSAErrors::SetErrorMsgText("sendto:", WSAGetLastError());
				}
			}
			catch (string errorMsgText)
			{
				cout << endl << errorMsgText;
			}
		}
	}

	if (closesocket(sSCAUDP) == SOCKET_ERROR)
		throw  WSAErrors::SetErrorMsgText("closesocket:", WSAGetLastError());
	if (WSACleanup() == SOCKET_ERROR)
		throw  WSAErrors::SetErrorMsgText("Cleanup:", WSAGetLastError());
	cout << "CritticalSectionServer stop\n";
	ExitThread(rc);
}

int main(int args, char* argv[])
{
	if (args >= 2)
		serverPort = atoi(argv[1]);
	if (args >= 3)
		strcpy(dll, argv[2]);
	if (args >= 4)
		strcpy(namedPipe, argv[3]);
	if (args < 2) {
		serverPort = 2000;
		strcpy(dll, "Win32Project1.dll");
		strcpy(namedPipe, "NP");
	}
	
	cout << "ServerPort -" << serverPort << endl;
	cout << "NamedPipe -" << namedPipe << endl;
	st = LoadLibraryA(dll);
	ts = (HANDLE(*)(char*, LPVOID))GetProcAddress(st, "SSS");
	if (st == NULL)
		cout << "Load DLL ERROR" << endl;
	else
		cout << "DLL - " << dll << endl << endl;

	InitializeCriticalSection(&scListContact);
	volatile TalkersCommand cmd = START;

	hAcceptServer = CreateThread(NULL, NULL, AcceptServer, (LPVOID)&cmd, NULL, NULL);
	hDispatchServer = CreateThread(NULL, NULL, DispatchServer, (LPVOID)&cmd, NULL, NULL);
	hGarbageCleaner = CreateThread(NULL, NULL, GarbageCleaner, (LPVOID)&cmd, NULL, NULL);
	hConsolePipe = CreateThread(NULL, NULL, ConsolePipe, (LPVOID)&cmd, NULL, NULL);
	hResponseServer = CreateThread(NULL, NULL, ResponseServer, (LPVOID)&cmd, NULL, NULL);
	hTimeServer = CreateThread(NULL, NULL, TimeServer, (LPVOID)&cmd, NULL, NULL);
	hNtpSincroServer = CreateThread(NULL, NULL, NtpSincroServer, (LPVOID)&cmd, NULL, NULL);
	hCritticalSectionServer = CreateThread(NULL, NULL, CritticalSectionServer, (LPVOID)&cmd, NULL, NULL);

	SetThreadPriority(hAcceptServer, THREAD_PRIORITY_HIGHEST);
	SetThreadPriority(hResponseServer, THREAD_PRIORITY_ABOVE_NORMAL);
	SetThreadPriority(hDispatchServer, THREAD_PRIORITY_NORMAL);
	SetThreadPriority(hConsolePipe, THREAD_PRIORITY_NORMAL);
	SetThreadPriority(hGarbageCleaner, THREAD_PRIORITY_BELOW_NORMAL);
	SetThreadPriority(hTimeServer, THREAD_PRIORITY_ABOVE_NORMAL);
	SetThreadPriority(hNtpSincroServer, THREAD_PRIORITY_ABOVE_NORMAL);
	SetThreadPriority(hCritticalSectionServer, THREAD_PRIORITY_ABOVE_NORMAL);

	WaitForSingleObject(hAcceptServer, INFINITE);
	WaitForSingleObject(hDispatchServer, INFINITE);
	WaitForSingleObject(hGarbageCleaner, INFINITE);
	WaitForSingleObject(hConsolePipe, INFINITE);
	WaitForSingleObject(hResponseServer, INFINITE);
	WaitForSingleObject(hTimeServer, INFINITE);
	WaitForSingleObject(hNtpSincroServer, INFINITE);
	WaitForSingleObject(hCritticalSectionServer, INFINITE);
	
	CloseHandle(hAcceptServer);
	CloseHandle(hDispatchServer);
	CloseHandle(hGarbageCleaner);
	CloseHandle(hConsolePipe);
	CloseHandle(hResponseServer);
	CloseHandle(hTimeServer);
	CloseHandle(hNtpSincroServer);
	CloseHandle(hCritticalSectionServer);

	DeleteCriticalSection(&scListContact);
	FreeLibrary(st);
	return 0;
}