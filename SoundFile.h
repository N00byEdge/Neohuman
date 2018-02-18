#pragma once

#include "SFML/Audio.hpp"

#ifdef WIN32

#include <windows.h>

namespace Neolib {
struct SoundFile {
  SoundFile(const char *filename);

  void play();

private:
	sf::SoundBuffer buffer;
};
} // namespace Neolib

#endif
