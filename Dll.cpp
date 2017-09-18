#include <BWAPI.h>

#ifdef WIN32 // Windows

#include <Windows.h>
#define DLLEXPORT __declspec(dllexport)

#else // Not windows

#define DLLEXPORT

#endif

#include "Neohuman.h"

#include <iostream>

extern "C" DLLEXPORT void gameInit(BWAPI::Game* game) { BWAPI::BroodwarPtr = game; }

#ifdef WIN32
BOOL APIENTRY DllMain( HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved ) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		std::cout <<	" ad88888ba                                     88     888b      88               88\n"
						"d8\"     \"8b                                    88     8888b     88               88\n"
						"Y8,                                            88     88 `8b    88               88\n"
						"`Y8aaaaa,     ,adPPYba,  8b,dPPYba,    ,adPPYb,88     88  `8b   88  88       88  88   ,d8   ,adPPYba,  ,adPPYba,\n"
						"  `\"\"\"\"\"8b,  a8P_____88  88P'   `\"8a  a8\"    `Y88     88   `8b  88  88       88  88 ,a8\"   a8P_____88  I8[    \"\"\n"
						"        `8b  8PP\"\"\"\"\"\"\"  88       88  8b       88     88    `8b 88  88       88  8888[     8PP\"\"\"\"\"\"\"   `\"Y8ba,\n"
						"Y8a     a8P  \"8b,   ,aa  88       88  \"8a,   ,d88     88     `8888  \"8a,   ,a88  88`\"Yba,  \"8b,   ,aa  aa    ]8I\n"
						" \"Y88888P\"    `\"Ybbd8\"'  88       88   `\"8bbdP\"Y8     88      `888   `\"YbbdP'Y8  88   `Y8a  `\"Ybbd8\"'  `\"YbbdP\"'" << std::endl;
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
#endif

extern "C" DLLEXPORT BWAPI::AIModule* newAIModule() {
	return neoInstance = new Neohuman();
}
