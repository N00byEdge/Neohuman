#pragma once

#include "SoundFile.h"

#include <map>
#include <string>
#include <vector>

namespace Neolib {

struct SoundDatabase {
  void play_sound(std::string sound);

  void loadSounds();

  void playRandomSadSound();
  void playRandomHappySound();
  void playRandomKillSound();
  void playRandomFunnySound();

  void unload();

#ifdef WIN32

private:
  void playRandom(const std::vector<std::string> &);
  void loadSound(std::string soundname);
  std::map<std::string, SoundFile> sounds;

#endif
};

} // namespace Neolib

extern Neolib::SoundDatabase soundDatabase;
