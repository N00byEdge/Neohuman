#include <BWAPI.h>

#ifdef WIN32 // Windows

#include <Windows.h>
#define DLLEXPORT __declspec(dllexport)

#else // Not windows

#define DLLEXPORT

#endif

#include "Neohuman.h"

extern "C" DLLEXPORT void gameInit(BWAPI::Game* game) { BWAPI::BroodwarPtr = game; }

#ifdef WIN32
BOOL APIENTRY DllMain( HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved ) {
  switch (ul_reason_for_call) {
  case DLL_PROCESS_ATTACH:
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
