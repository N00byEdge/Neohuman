#pragma once

namespace Neolib {

	struct SupplyCount {
		SupplyCount();
		SupplyCount(int protoss, int terran, int zerg);

		SupplyCount operator+ (const SupplyCount &other) const;
		SupplyCount operator+=(const SupplyCount &other);
		SupplyCount operator- (const SupplyCount &other) const;
		SupplyCount operator-=(const SupplyCount &other);
		SupplyCount operator* (const int factor) const;
		SupplyCount operator*=(const int factor);
		SupplyCount operator/ (const int divisor) const;
		SupplyCount operator/=(const int divisor);

		int protoss, terran, zerg;
	};

	class SupplyManager {
		public:
			SupplyCount usedSupply();
			SupplyCount availableSupply();
			SupplyCount wantedSupplyOverhead();
			SupplyCount wantedAdditionalSupply();
	};

	int mainSupply(SupplyCount);

}

extern Neolib::SupplyManager supplyManager;
