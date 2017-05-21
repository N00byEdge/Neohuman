#pragma once

#include <BWAPI.h>

namespace Neolib {
	struct ResourceCount {
		ResourceCount();
		ResourceCount(int, int);
		ResourceCount(BWAPI::UnitType);
		ResourceCount(BWAPI::UpgradeType);
		ResourceCount(BWAPI::TechType);

		ResourceCount operator+ (const ResourceCount &other) const;
		ResourceCount operator+=(const ResourceCount &other);
		ResourceCount operator- (const ResourceCount &other) const;
		ResourceCount operator-=(const ResourceCount &other);
		ResourceCount operator* (const int factor) const;
		ResourceCount operator*=(const int factor);
		ResourceCount operator/ (const int divisor) const;
		ResourceCount operator/=(const int divisor);

		int minerals, gas;
	};

	class ResourceManager {
		public:
			ResourceCount getSpendableResources();
			bool canAfford(ResourceCount);
	};

}

bool operator<= (Neolib::ResourceCount &lhs, Neolib::ResourceCount &rhs);
bool operator>= (Neolib::ResourceCount &lhs, Neolib::ResourceCount &rhs);
bool operator== (Neolib::ResourceCount &lhs, Neolib::ResourceCount &rhs);

extern Neolib::ResourceManager resourceManager;
