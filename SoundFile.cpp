#include "SoundFile.h"

#ifdef WIN32

#include <conio.h>
#include <fstream>
#include <mmsystem.h>

namespace Neolib {

SoundFile::SoundFile(const char *filename) {
  hi = GetModuleHandle(0);

  std::ifstream inf(filename, std::ios::binary);

  if (inf) {
    inf.seekg(0, std::ios::end);
    auto length = inf.tellg();
#ifdef _DEBUG
    buf = (LPCWSTR) new char[(unsigned)length];
#else
    buf = (LPCSTR) new char[(unsigned)length];
#endif
    inf.seekg(0, std::ios::beg);
    inf.read((char *)buf, length);
    inf.close();
  }
}

SoundFile::~SoundFile() {
  PlaySound(NULL, 0, 0);
  delete[] buf;
}

void SoundFile::play() {
  if (buf)
    PlaySound(buf, hi, SND_MEMORY);
}

void SoundFile::play_async() {
  if (buf)
    PlaySound(buf, hi, SND_MEMORY | SND_ASYNC);
}
} // namespace Neolib

#endif
