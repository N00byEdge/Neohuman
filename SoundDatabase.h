#pragma once

#include "SoundFile.h"

#include <string>
#include <vector>
#include <map>

namespace Neolib {

	class SoundDatabase {
		public:
			void play_sound(std::string sound, bool async = true);

			void loadSounds();

			void playRandomSadSound();
			void playRandomHappySound();
			void playRandomKillSound();

#ifdef WIN32

		private:
			void playRandom(const std::vector <std::string> &);
			void loadSound(std::string soundname);
			std::map <std::string, SoundFile> sounds;

#endif
	};

}

extern Neolib::SoundDatabase soundDatabase;
