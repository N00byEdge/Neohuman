#pragma once

#include <utility>
#include <vector>

#include "Neohuman.h"

#define MIN(a,b) (((a) < (b) ? (a) : (b)))
#define MAX(a,b) (((a) < (b) ? (b) : (a)))

#define MINE(a,b) (a) = MIN((a), (b))
#define MAXE(a,b) (a) = MAX((a), (b))

#define ABS(val) ((val > 0) ? (val) : -(val))

#define SATRUATION_RADIUS 350
#define WORKERAGGRORADIUS 200

bool operator< (BWAPI::TilePosition lhs, BWAPI::TilePosition rhs);

namespace Neolib {
	// Random functions
	int randint(int, int);

	template <typename T>
	T randele(const std::vector <T>&);

	// Drawing text to screen helpers
	const char *noRaceName(const char *name);

}
