#pragma once

#include <utility>
#include <vector>

#include "Neohuman.h"

#define MIN(a,b) (((a) < (b) ? (a) : (b)))
#define MAX(a,b) (((a) < (b) ? (b) : (a)))

#define MINE(a,b) (a) = MIN((a), (b))
#define MAXE(a,b) (a) = MAX((a), (b))

#define ABS(val) ((val > 0) ? (val) : (unsigned)(~(val)) + 1U)
#define ABSL(val) ((val > 0) ? (val) : (unsigned long long)(~(val)) + 1ULL)

#define SATRUATION_RADIUS 300
#define WORKERAGGRORADIUS 100

template <int scale>
bool operator< (BWAPI::Point<int, scale> lhs, BWAPI::Point<int, scale> rhs);

namespace Neolib {
	// Random functions
	int randint(int, int);

	template <typename T>
	T &randele(const std::vector <T>&);

	// Drawing text to screen helpers
	const char *noRaceName(const char *name);

	struct hash {
		template <int scale>
		size_t operator()(const BWAPI::Point<int, scale> &p) const;
	};

}
