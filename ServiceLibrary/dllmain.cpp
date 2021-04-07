// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "Windows.h"
#include "DefineTableService.h"
#include "EchoServer.h"
#include "RandServer.h"
#include "TimeServer.h"

BEGIN_TABLESERVICE              // таблица 
ENTRYSERVICE("Echo", EchoServer),
ENTRYSERVICE("Time", TimeServer),
ENTRYSERVICE("Rand", RandServer)
END_TABLESERVICE;

extern "C" __declspec(dllexport) HANDLE  SSS(char* id, LPVOID prm)
{
	//std::cout << "allo" << std::endl;
	HANDLE rc = NULL;
	int  i = 0;
	while (i < SIZETS && strcmp(TABLESERVICE_ID(i), id) != 0)i++;
	if (i < SIZETS)
		rc = CreateThread(NULL, NULL, TABLESERVICE_FN(i), prm, NULL, NULL);
	return rc;
};

BOOL APIENTRY DllMain(HANDLE hinst, DWORD  rcall, LPVOID wres)
{
	return TRUE;
}
