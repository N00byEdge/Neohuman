#pragma once

#include <utility>
#include <vector>

#include "Neohuman.h"

#define MIN(a, b) (((a) < (b) ? (a) : (b)))
#define MAX(a, b) (((a) < (b) ? (b) : (a)))

#define MINE(a, b) (a) = MIN((a), (b))
#define MAXE(a, b) (a) = MAX((a), (b))

#define ABS(val)                                                               \
  ((val > 0) ? (val) : (decltype(val))(~(val)) + (decltype(val))1)

template <int scale>
bool operator<(BWAPI::Point<int, scale> lhs, BWAPI::Point<int, scale> rhs);

namespace Neolib {
// Random functions
int randint(int, int);

template <typename T>
inline T randele(const std::vector<T> &v) {
  return v[randint(0, v.size() - 1)];
}

template <typename Container, typename Key>
bool isIn(const Container &c, const Key &k) {
  return c.find(k) != c.end();
}

// Drawing text to screen helpers
const char *noRaceName(const char *name);

struct hash {
  template <int scale>
  size_t operator()(const BWAPI::Point<int, scale> &p) const;
};

} // namespace Neolib
