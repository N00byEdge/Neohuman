#include "SoundFile.h"

#include <mmsystem.h>
#include <conio.h>
#include <fstream>

namespace Neolib {

	SoundFile::SoundFile(LPCWSTR filename) {
		hi = GetModuleHandle(0);

		std::ifstream inf(filename, std::ios::binary);

		if (inf) {
			inf.seekg(0, std::ios::end);
			auto length = inf.tellg();
			buf = (LPCWSTR)new char[(unsigned)length];
			inf.seekg(0, std::ios::beg);
			inf.read((char*) buf, length);
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
}


