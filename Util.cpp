#include "Util.h"

#include <Random>

#include "Neohuman.h"

std::mt19937 mt = std::mt19937(std::default_random_engine{});

int Neolib::randint(int min, int max) {
	std::uniform_int_distribution<int> dist(min, max);
	return dist(mt);
}

template <typename T>
T Neolib::randele(std::vector <T> &v) {
	return v[randint(0, v.size())];
}

const char *Neolib::noRaceName(const char *name) {
	for (const char *c = name; *c; c++)
		if (*c == '_') return ++c;
	return name;
}
