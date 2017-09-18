#include "ResourceManager.h"

#include "BWAPI.h"

namespace Neolib {

	ResourceCount::ResourceCount() : minerals(0), gas(0) {

	}

	ResourceCount::ResourceCount(int minerals, int gas) : minerals(minerals), gas(gas) {

	}

	ResourceCount::ResourceCount(BWAPI::UnitType ut) : minerals(ut.mineralPrice()), gas(ut.gasPrice()) {

	}

	ResourceCount::ResourceCount(BWAPI::UpgradeType ut) : minerals(ut.mineralPrice()), gas(ut.gasPrice()) {

	}

	ResourceCount::ResourceCount(BWAPI::TechType tt) : minerals(tt.mineralPrice()), gas(tt.gasPrice()) {

	}

	ResourceCount ResourceCount::operator+ (const ResourceCount &other) const {
		return ResourceCount(minerals + other.minerals, gas + other.gas);
	}

	ResourceCount &ResourceCount::operator+=(const ResourceCount &other) {
		minerals += other.minerals;
		gas += other.gas;
		return *this;
	}

	ResourceCount ResourceCount::operator- (const ResourceCount &other) const {
		return ResourceCount(minerals - other.minerals, gas - other.gas);
	}

	ResourceCount &ResourceCount::operator-=(const ResourceCount &other) {
		minerals -= other.minerals;
		gas -= other.gas;
		return *this;
	}

	ResourceCount ResourceCount::operator* (const int factor) const {
		return ResourceCount(minerals * factor, gas * factor);
	}
	
	ResourceCount &ResourceCount::operator*=(const int factor) {
		minerals *= factor;
		gas *= factor;
		return *this;
	}

	ResourceCount ResourceCount::operator/ (const int divisor) const {
		return ResourceCount(minerals / divisor, gas / divisor);
	}

	ResourceCount &ResourceCount::operator/=(const int divisor) {
		minerals /= divisor;
		gas /= divisor;
		return *this;
	}
}

bool operator<= (Neolib::ResourceCount &lhs, Neolib::ResourceCount &rhs) {
	return lhs.minerals <= rhs.minerals && lhs.gas <= rhs.gas;
}

bool operator>= (Neolib::ResourceCount &lhs, Neolib::ResourceCount &rhs) {
	return lhs.minerals >= rhs.minerals && lhs.gas >= rhs.gas;
}

bool operator== (Neolib::ResourceCount &lhs, Neolib::ResourceCount &rhs) {
	return lhs.minerals == rhs.minerals && lhs.gas == rhs.gas;
}
