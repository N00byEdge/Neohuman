#include "SoundFile.h"

#ifdef WIN32

#include <conio.h>
#include <fstream>
#include <mmsystem.h>
#include <thread>

namespace Neolib {

auto launchSound = [](const sf::SoundBuffer &sb) {
	sf::Sound sound;
	sound.setBuffer(sb);
	sound.play();
	while (sound.getStatus() == sf::SoundSource::Status::Playing) {};
};

SoundFile::SoundFile(const char *filename) {
	buffer.loadFromFile(filename);
}

void SoundFile::play() {
	auto t = std::thread(launchSound, std::ref(buffer));
	t.detach();
}

} // namespace Neolib

#endif
