#include "Util.h"

#include <Random>

#include "Neohuman.h"

std::mt19937_64 mt(std::chrono::system_clock::now().time_since_epoch().count());

template <int scale>
bool operator< (BWAPI::Point<int, scale> lhs, BWAPI::Point<int, scale> rhs) {
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
T &Neolib::randele(const std::vector <T> &v) {
	return v[randint(0, v.size() - 1)];
}

const char *Neolib::noRaceName(const char *name) {
	for (const char *c = name; *c; c++)
		if (*c == '_') return ++c;
	return name;
}

template <int scale>
size_t Neolib::hash::operator()(const BWAPI::Point<int, scale> &p) const {
	return std::hash <int>()(p.x << 16 | p.y);
}
