#pragma once

#ifdef WIN32

#include <windows.h>

namespace Neolib {
struct SoundFile {
  SoundFile(const char *filename);
  ~SoundFile();

  void play();
  void play_async();

private:
#ifdef _DEBUG
  LPCWSTR buf = nullptr;
#else
  LPCSTR buf = nullptr;
#endif
  HINSTANCE hi;
};
} // namespace Neolib

#endif
