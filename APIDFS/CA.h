#pragma once
#pragma warning(disable:4996)

#include <string>
#include <sstream>
#include <iostream>
#include <map>

#include <ctime>
#include <fstream>
#include "Winsock2.h" //заголовок WS2_32.dll
#pragma comment(lib, "WS2_32.lib") //экспорт WS2_32.dll

#include "WSAErrors.h"

#define CAPORT 2002
#define CATIMEOUT 2000
#define WAITTIME 100

using namespace std;

namespace CA {
    enum STATUS {
        NOINIT,
        INIT,
        ENTER,
        LEAVE,
        WAIT
    };

    struct CA {
        char ipaddr[15];
        char resurce[20];

        STATUS status;
    };

    std::ostream& operator<<(std::ostream& out, const STATUS value) {
        static std::map<STATUS, std::string> strings;
        if (strings.size() == 0) {
#define INSERT_ELEMENT(p) strings[p] = #p
            INSERT_ELEMENT(NOINIT);
            INSERT_ELEMENT(INIT);
            INSERT_ELEMENT(ENTER);
            INSERT_ELEMENT(LEAVE);
            INSERT_ELEMENT(WAIT);
#undef INSERT_ELEMENT
        }
        return out << strings[value];
    }

    CA InitCA(
        char ipaddr[15],
        char resurce[20]
    );

    bool EnterCA(
        CA& ca
    );

    bool LeaveCA(
        CA& ca
    );

    bool CloseCA(
        CA& ca
    );


    bool SendAndRecvCA(CA& ca) {
        bool rc = false;
        CA caR;
        SOCKET sS;

        if ((sS = socket(AF_INET, SOCK_DGRAM, NULL)) == INVALID_SOCKET)
            throw WSAErrors::SetErrorMsgText("GRFC socket:", WSAGetLastError());

        SOCKADDR_IN CAServ;
        CAServ.sin_family = AF_INET;
        CAServ.sin_port = htons(CAPORT);
        CAServ.sin_addr.s_addr = inet_addr(ca.ipaddr);

        int lobuf;
        int libuf;

        do {
            if ((lobuf = sendto(sS, (char*)&ca, sizeof(ca), NULL, (sockaddr*)&CAServ, sizeof(CAServ))) == SOCKET_ERROR)
                throw WSAErrors::SetErrorMsgText("sendto:", WSAGetLastError());

            SOCKADDR_IN from;
            memset(&from, 0, sizeof(from));
            int lc = sizeof(from);
            memset(&caR, 0, sizeof(caR));

            if (libuf = recvfrom(sS, (char*)&caR, sizeof(caR), NULL, (sockaddr*)&from, &lc) == SOCKET_ERROR)
                throw WSAErrors::SetErrorMsgText("recvfrom:", WSAGetLastError());
            if (caR.status == WAIT)
                Sleep(WAITTIME);
        } while (caR.status != ca.status);

        if (closesocket(sS) == SOCKET_ERROR)
            throw WSAErrors::SetErrorMsgText("GRFC socket:", WSAGetLastError());
        return rc;
    };

    CA InitCA(char ipaddr[15], char resurce[20])
    {
        CA ca;
        strncpy(ca.ipaddr, ipaddr, sizeof(ca.ipaddr));
        strncpy(ca.resurce, resurce, sizeof(ca.resurce));
        ca.status = INIT;

        return ca;
    };

    bool EnterCA(CA& ca)
    {
        ca.status = ENTER;
        SendAndRecvCA(ca);
        return false;
    };

    bool LeaveCA(CA& ca)
    {
        ca.status = LEAVE;
        SendAndRecvCA(ca);
        return false;
    };

    bool CloseCA(CA& ca)
    {
        ca.status = NOINIT;
        return false;
    };

    void print(CA& ca) {
        std::cout << "Client " << ca.ipaddr << " to " << ca.resurce << " with " << ca.status << std::endl;
    }
}