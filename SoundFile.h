#pragma once

#include <windows.h>

namespace Neolib {
	class SoundFile {
		public:
			SoundFile(LPCWSTR filename);
			~SoundFile();

			void play();
			void play_async();

		private:
			LPCWSTR buf = nullptr;
			HINSTANCE hi;
	};
}
