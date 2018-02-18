#include "SoundDatabase.h"

#include <vector>

#include "Util.h"

Neolib::SoundDatabase soundDatabase;

const std::string basePath = "bwapi-data/read/sound/", fileExtension = ".ogg";

const std::vector<std::string> sadsounds = {"Shittyflute", "Sad1", "Sad2",
                                            "Alert"};

const std::vector<std::string> happysounds = {"Seinfeld", "Fail1", "Fail2",
                                              "Nope", "Haha"};

const std::vector<std::string> killsounds = {"Multikill", "Monsterkill",
                                             "Cockduck"};

const std::vector<std::string> funnysounds = {"Mayo"};

namespace Neolib {

#ifdef WIN32

void SoundDatabase::play_sound(std::string sound) {
  auto it = sounds.find(sound);

  if (it == sounds.end())
    return;

  it->second.play();
}

void SoundDatabase::loadSounds() {
  for (auto &s : sadsounds)
    loadSound(s);

  for (auto &s : happysounds)
    loadSound(s);

  for (auto &s : killsounds)
    loadSound(s);

  for (auto &s : funnysounds)
    loadSound(s);

  loadSound("alice");
}

void SoundDatabase::playRandomSadSound() { return playRandom(sadsounds); }

void SoundDatabase::playRandomHappySound() { return playRandom(happysounds); }

void SoundDatabase::playRandomKillSound() { return playRandom(killsounds); }

void SoundDatabase::playRandomFunnySound() { return playRandom(funnysounds); }

void SoundDatabase::unload() {
	sounds.clear();
}

void SoundDatabase::playRandom(const std::vector<std::string> &v) {
  if constexpr (IsOnTournamentServer())
    play_sound(v[randint(0, v.size() - 1)]);
}

void SoundDatabase::loadSound(std::string soundname) {
  sounds.emplace(soundname,
                 std::string(basePath + soundname + fileExtension).c_str());
}

#else

void SoundDatabase::play_sound(std::string sound, bool async) {}

void SoundDatabase::loadSounds() {}

void SoundDatabase::playRandomSadSound() {}

void SoundDatabase::playRandomHappySound() {}

void SoundDatabase::playRandomKillSound() {}

#endif

} // namespace Neolib
