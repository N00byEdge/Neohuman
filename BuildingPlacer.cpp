#include "BuildingPlacer.h"

#include "BuildingQueue.h"
#include "BaseManager.h"
#include "MapManager.h"

Neolib::BuildingPlacer buildingPlacer;

namespace Neolib {

	void BuildingPlacer::init() {
		mapWidth = BWAPI::Broodwar->mapWidth(), mapHeight = BWAPI::Broodwar->mapHeight();
		for (BWAPI::TilePosition pos(0, 0); pos.x < mapWidth; ++pos.x)
			for (pos.y = 0; pos.y < mapHeight; ++pos.y)
				mapCanBuild[pos.x + pos.y * 256] = BWEM::Map::Instance().GetTile(pos).Buildable();
	}

	BWAPI::TilePosition BuildingPlacer::getBuildLocation(BWAPI::UnitType ut, BWAPI::TilePosition suggestedPosition) {
		bool needsCreep = ut.requiresCreep();
		bool canBePlacedOnCreep = needsCreep || ut.getRace() == BWAPI::Races::Zerg;

		if (ut.isResourceDepot() || ut.isRefinery())
			return BWAPI::TilePositions::None;

		for (int x = 0; x < mapWidth; ++x) {
			for (int y = 0; y < mapHeight; ++y) {
				if (needsCreep && BWAPI::Broodwar->hasCreep(BWAPI::TilePosition(x, y)))
					canBuild[x + y * 256] = mapCanBuild[x + y * 256];
				else if (needsCreep)
					canBuild[x + y * 256] = false;
				else if (!canBePlacedOnCreep && !BWAPI::Broodwar->hasCreep(BWAPI::TilePosition(x, y)))
					canBuild[x + y * 256] = mapCanBuild[x + y * 256];
				else if (!canBePlacedOnCreep)
					canBuild[x + y * 256] = false;
				if (!BWAPI::Broodwar->isVisible(BWAPI::TilePosition(x, y)))
					canBuild[x + y * 256] = false;
			}
		}

		for (auto &b : BWAPI::Broodwar->getAllUnits()) {
			if ((b->getType().isBuilding() || b->getType().isResourceContainer()) && !b->isFlying()) {
				unsigned x = b->getTilePosition().x, xEnd = b->getTilePosition().x + b->getType().tileWidth();
				unsigned y = b->getTilePosition().y, yEnd = b->getTilePosition().y + b->getType().tileHeight();

				// Buildings with addons
				if (b->getPlayer() == BWAPI::Broodwar->self() && b->getType() == BWAPI::UnitTypes::Terran_Factory || b->getType() == BWAPI::UnitTypes::Terran_Starport || b->getType() == BWAPI::UnitTypes::Terran_Science_Facility || b->getType() == BWAPI::UnitTypes::Terran_Command_Center)
					xEnd += 2;
				
				for (; x < xEnd; ++x)
					for (y = b->getTilePosition().y; y < yEnd; ++y)
						canBuild[x + y * 256] = false;
			}

			for (auto &o : buildingQueue.buildingsQueued()) {
				if (o.buildingType.isBuilding()) {
					unsigned x = o.designatedLocation.x, xEnd = o.designatedLocation.x + o.buildingType.tileWidth();
					unsigned y = o.designatedLocation.y, yEnd = o.designatedLocation.y + o.buildingType.tileHeight();

					// Buildings with addons
					if (o.buildingType == BWAPI::UnitTypes::Terran_Factory || o.buildingType == BWAPI::UnitTypes::Terran_Starport || o.buildingType == BWAPI::UnitTypes::Terran_Science_Facility || o.buildingType == BWAPI::UnitTypes::Terran_Command_Center)
						xEnd += 2;

					for (; x < xEnd; ++x)
						for (y = o.designatedLocation.y; y < yEnd; ++y)
							canBuild[x + y * 256] = false;
				}
			}

			for (auto &o : mapManager.getNoBuildRects()) {
				unsigned x = o.first.x, xEnd = o.second.x;
				unsigned y = o.first.y, yEnd = o.second.y;

				for (; x < xEnd; ++x)
					for (unsigned ty = y; ty < yEnd; ++ty)
						canBuild[x + ty * 256] = false;
			}
		}

		for (auto &b : baseManager.getAllBases()) {
			auto nbs = b.getNoBuildRegion();
			for (int x = nbs.first.x; x < nbs.second.x; ++x)
				for (int y = nbs.first.y; y < nbs.second.y; ++y)
					canBuild[x + y * 256] = false;
		}

		unsigned e = 1;
		bool xMax = false, xMin = false, yMax = false, yMin = false;
		for (int state = 0; !xMax || !xMin || !yMax || !yMin; ++state) {
			for (unsigned i = 0; i < e - 1; ++i) {
				if (canPlaceBuilding(ut, suggestedPosition) && BWAPI::Broodwar->canBuildHere(suggestedPosition, ut, nullptr, true))
					return suggestedPosition;

				if (suggestedPosition.x < 0)
					xMin = true;
				if (suggestedPosition.y < 0)
					yMin = true;
				if (suggestedPosition.x >= mapWidth)
					xMax = true;
				if (suggestedPosition.y >= mapHeight)
					xMax = true;
				
				switch (state) {
					case 0:
						++suggestedPosition.x;
						break;
					case 1:
						--suggestedPosition.y;
						break;
					case 2:
						--suggestedPosition.x;
						break;
					case 3:
						++suggestedPosition.y;
						break;
				}
			}

			if (state == 3) {
				e += 2;
				state = -1;
				--suggestedPosition.x;
				++suggestedPosition.y;
			}
		}

		return BWAPI::TilePositions::None;
	}

	bool BuildingPlacer::canBuildAt(BWAPI::TilePosition tp) {
		if (tp.isValid())
			return canBuild[tp.x + tp.y * 256];
		else
			return false;
	}

	bool BuildingPlacer::canPlaceBuilding(BWAPI::UnitType ut, BWAPI::TilePosition location) {
		BWAPI::TilePosition pos(location);
		int xEnd = location.x + ut.tileWidth(), yEnd = location.y + ut.tileHeight();

		// Buildings with addons
		if (ut == BWAPI::UnitTypes::Terran_Factory || ut == BWAPI::UnitTypes::Terran_Starport || ut == BWAPI::UnitTypes::Terran_Science_Facility)
			xEnd += 2;

		// Buildings with one tile of padding
		if (!ut.isResourceDepot() && !ut.isRefinery()) {
			--location.x;
			--location.y;
			++xEnd;
			++yEnd;
		}

		for (; pos.x < xEnd; ++pos.x)
			for (pos.y = location.y; pos.y < yEnd; ++pos.y)
				if (!canBuildAt(pos))
					return false;

		return true;
	}

}
