#include "Util.h"

#include <Random>

#include "Neohuman.h"

std::mt19937_64 mt(std::chrono::system_clock::now().time_since_epoch().count());

bool operator< (BWAPI::TilePosition lhs, BWAPI::TilePosition rhs) {
	if (lhs.x < rhs.x)
		return true;
	if (lhs.x > rhs.x)
		return false;
	return lhs.y < rhs.y;
}

int Neolib::randint(int min, int max) {
	std::uniform_int_distribution<int> dist(min, max);
	return dist(mt);
}

template <typename T>
T Neolib::randele(const std::vector <T> &v) {
	return v[randint(0, v.size() - 1)];
}

const char *Neolib::noRaceName(const char *name) {
	for (const char *c = name; *c; c++)
		if (*c == '_') return ++c;
	return name;
}
