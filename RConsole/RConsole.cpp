#include <sstream>
#include <string>
#include <iostream>
#include <list>
#include <ctime>
#include "Winsock2.h"

#pragma comment(lib, "WS2_32.lib") 
#pragma warning(disable:4996)

#define AS_SQUIRT 10

using namespace std;

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

string SetPipeError(string msgText, int code) {
	return msgText + GetErrorMsgText(code);
}

int main()
{
	setlocale(LC_ALL, "Russian");

	char rbuf[200];
	DWORD dwRead, dwWrite;
	HANDLE hPipe;
	int n;

	try
	{
		if ((hPipe = CreateFile(L"\\\\.\\pipe\\NP", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL)) == INVALID_HANDLE_VALUE)
			throw  SetPipeError("createfile:", GetLastError());

		char wbuf[40] = "start";
		int command = 0;
		do {
			cout << "\nInput server command" << endl;
			cout << "1 - start" << endl;
			cout << "2 - stop" << endl;
			cout << "3 - statistics" << endl;
			cout << "4 - shutdown" << endl;
			cout << "5 - exit" << endl;
			cin >> command;

			switch (command) {
			case 1:
				strcpy_s(wbuf, "start");
				system("cls");
				break;
			case 2:
				strcpy_s(wbuf, "stop");
				system("cls");
				break;
			case 3:
				strcpy_s(wbuf, "statistics");
				system("cls");
				break;
			case 4:
				strcpy_s(wbuf, "shutdown");
				system("cls");
				break;
			case 5:
				strcpy_s(wbuf, "exit");
				system("cls");
				break;
			}

			if (!WriteFile(hPipe, wbuf, sizeof(wbuf), &dwWrite, NULL))
				throw  SetPipeError("writefile:", GetLastError());

			if (ReadFile(hPipe, rbuf, sizeof(rbuf), &dwRead, NULL))
				cout << "receive:  " << rbuf << endl;
		} while (command != 6);

	}
	catch (string ErrorPipeText)
	{
		cout << endl << ErrorPipeText << endl;
	}

	CloseHandle(hPipe);
	system("pause");
	return 0;
}
