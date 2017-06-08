#pragma once

#include <utility>
#include <vector>

#define MIN(a, b) (((a) < (b) ? (a) : (b)))
#define MAX(a, b) (((a) < (b) ? (b) : (a)))

#define SATRUATION_RADIUS 350
#define WORKERAGGRORADIUS 200

namespace Neolib {
	// Random functions
	int randint(int, int);

	template <typename T>
	T randele(const std::vector <T>&);

	// Drawing text to screen helpers
	const char *noRaceName(const char *name);

}
