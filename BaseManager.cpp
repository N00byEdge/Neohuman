#include "BaseManager.h"
#include "Util.h"

#include "MapManager.h"
#include "UnitManager.h"

Neolib::BaseManager baseManager;

namespace Neolib {

	Base::Base(BWAPI::Unit rd): resourceDepot(rd), race(rd->getType().getRace()) {
		for (auto &b : mapManager.getAllBases())
			if (b->Location() == rd->getTilePosition())
				BWEMBase = b;

		// If we can't find the base, just take something close enough.
		if (!BWEMBase)
			for (auto &sl : BWEM::Map::Instance().StartingLocations())
				if (sl == rd->getTilePosition())
					for (auto &b : mapManager.getAllBases())
						if (ABS(b->Location().x - sl.x) + ABS(b->Location().y - sl.y) < 10)
							BWEMBase = b;
		

		if (BWEMBase == nullptr) {
			BWAPI::Broodwar->sendText("Unable to find BWEM base");
			return;
		}

		mainPos = BWEMBase->Location();

		for (auto &m : BWEMBase->Minerals())
			if(m->Unit()->exists())
				mineralMiners[m->Unit()];

		for (auto &gg : BWEMBase->Geysers())
			for (auto &ut : unitManager.getFriendlyUnitsByType())
				if (ut.first.isRefinery())
					for (auto &b : ut.second)
						if(b->getTilePosition() == gg->TopLeft())
							gasMiners[b];
	}

	ResourceCount Base::additionalWantedWorkers() const {
		ResourceCount sum;
		for (auto &m : mineralMiners)
			if (m.second.size() < 2)
				sum.minerals += 2 - m.second.size();

		for (auto &g : gasMiners)
			if (g.second.size() < 3)
				sum.gas += 3 - g.second.size();

		return sum;
	}

	ResourceCount Base::numWorkers() const {
		ResourceCount num;

		for (auto &m : mineralMiners)
			num.minerals += m.second.size();

		for (auto &g : gasMiners)
			num.gas += g.second.size();

		return num;
	}

	ResourceCount Base::calculateIncome() const {
		ResourceCount income;

		if (redAlert != -1)
			return income;

		for (auto &mf : mineralMiners)
			if (mf.second.size() == 0)
				continue;
			else if (mf.second.size() == 1)
				income.minerals += 65;
			else if (mf.second.size() == 2)
				income.minerals += 126;
			else
				income.minerals += 143;

		for (auto &gg : gasMiners)
			if (gg.first->getResources())
				income.gas += 103 * MIN(gg.second.size(), 3);
			else
				income.gas += 26 * MIN(gg.second.size(), 3);

		if (race == BWAPI::Races::Protoss)
			return ResourceCount((int)(1.055f * income.minerals), income.gas);
		else if (race == BWAPI::Races::Zerg)
			return ResourceCount((int)(1.032f * income.minerals), income.gas);
		else return income;
	}

	bool Base::operator< (const Base &other) const {
		return resourceDepot < other.resourceDepot;
	}

	bool Base::operator==(const Base &other) const {
		return resourceDepot == other.resourceDepot;
	}

	std::pair<BWAPI::TilePosition, BWAPI::TilePosition> Base::getNoBuildRegion() const {
		int minX = resourceDepot->getTilePosition().x, maxX = minX + resourceDepot->getType().tileWidth(), minY = resourceDepot->getTilePosition().y, maxY = minY + resourceDepot->getType().tileHeight();
		
		for (auto &mf : mineralMiners) {
			MINE(minX, mf.first->getTilePosition().x);
			MINE(minY, mf.first->getTilePosition().y);
			MAXE(maxX, mf.first->getTilePosition().x + mf.first->getType().tileWidth());
			MAXE(maxY, mf.first->getTilePosition().y + mf.first->getType().tileHeight());
		}

		for (auto &gg : BWEMBase->Geysers()) {
			MINE(minX, gg->TopLeft().x);
			MINE(minY, gg->TopLeft().y);
			MAXE(maxX, gg->BottomRight().x + 1);
			MAXE(maxY, gg->BottomRight().y + 1);
		}
		
		return { BWAPI::TilePosition(minX, minY), BWAPI::TilePosition(maxX, maxY) };
	}

	std::size_t Base::hash::operator()(const Base &b) const {
		return std::hash <BWAPI::Unit>()(b.resourceDepot);
	}

	BaseManager::BaseManager() {

	}

	BaseManager::~BaseManager() {

	}

	void BaseManager::onUnitComplete(BWAPI::Unit unit) {
		if (unit->getType().isResourceDepot() && unit->getPlayer() == BWAPI::Broodwar->self())
			bases.insert(Base(unit));

		if (unit->getType().isWorker() && unit->getPlayer() == BWAPI::Broodwar->self())
			homelessWorkers.insert(unit);

		if (unit->getType().isRefinery() && unit->getPlayer() == BWAPI::Broodwar->self())
			for (auto &b : bases)
				for (auto &g : b.BWEMBase->Geysers())
					if (g->TopLeft() == unit->getTilePosition())
						b.gasMiners[unit];
	}

	void BaseManager::onUnitDestroy(BWAPI::Unit unit) {
		// Check if base died
		if (unit->getType().isResourceDepot() && unit->getPlayer() == BWAPI::Broodwar->self()) {
			auto it = bases.find(unit);

			// If this is a base
			if (it != bases.end()) {
				// Make workers idle
				for (auto &mf : it->mineralMiners) {
					for (auto &w : mf.second) {
						homelessWorkers.insert(w);
						workerBaseLookup.erase(w);
					}
				}

				bases.erase(it);
			}
		}

		// Check if worker died
		if (unit->getType().isWorker() && unit->getPlayer() == BWAPI::Broodwar->self()) {
			homelessWorkers.erase(unit);
			looseWorkers.erase(unit);
			// If managed, cleanup
			auto bpIt = workerBaseLookup.find(unit);

			if (bpIt != workerBaseLookup.end()) {
				for (auto &mf : bpIt->second->mineralMiners) {
					auto mineralIt = mf.second.find(unit);
					if (mineralIt != mf.second.end())
						mf.second.erase(mineralIt);
				}

				for (auto &gg : bpIt->second->gasMiners) {
					auto gasIt = gg.second.find(unit);
					if (gasIt != gg.second.end())
						gg.second.erase(gasIt);
				}
				
				workerBaseLookup.erase(bpIt);
			}
		}

		// Check if mineral field/gas died
		if (unit->getType().isMineralField() || (unit->getType().isRefinery() && unit->getPlayer() == BWAPI::Broodwar->self())) {
			for (auto &b : bases) {
				auto minIt = b.mineralMiners.find(unit);
				if (minIt != b.mineralMiners.end()) {
					for (auto &w : minIt->second) {
						workerBaseLookup.erase(w);
						homelessWorkers.insert(w);
					}
					b.mineralMiners.erase(minIt);
				}

				auto gasIt = b.gasMiners.find(unit);
				if (gasIt != b.gasMiners.end()) {
					for (auto &w : gasIt->second) {
						workerBaseLookup.erase(w);
						homelessWorkers.insert(w);
					}
					b.gasMiners.erase(gasIt);
				}
			}
		}
	}

	void BaseManager::onUnitDiscover(BWAPI::Unit unit) {
		if (unit->getType().isMineralField())
			for (auto &b : bases)
				for (auto &mf : b.BWEMBase->Minerals())
					if (mf->Unit() == unit)
						b.mineralMiners[unit];
	}

	void BaseManager::onFrame() {
		for (auto it = homelessWorkers.begin(); it != homelessWorkers.end();) {
			ResourceCount wantedWorkers;
			ResourceCount numWorkers;

			for (auto &b : bases)
				wantedWorkers += b.additionalWantedWorkers(), numWorkers += b.numWorkers();

			if (wantedWorkers.gas && numWorkers.minerals > numWorkers.gas / 3) {
				assignToGas(*it);
				it = homelessWorkers.erase(it);
				continue;
			}

			if (wantedWorkers.minerals) {
				assignToMinerals(*it);
				it = homelessWorkers.erase(it);
				continue;
			}
			
			++it;
		}

		for (auto &b : bases) {
			for (auto &mf : b.mineralMiners)
				for (auto &w : mf.second)
					if (w->isUnderAttack())
						b.redAlert = BWAPI::Broodwar->getFrameCount();

			for (auto &gg : b.gasMiners)
				for (auto &w : gg.second)
					if (w->isUnderAttack())
						b.redAlert = BWAPI::Broodwar->getFrameCount();

			if (b.redAlert != -1) {
				if (b.redAlert < BWAPI::Broodwar->getFrameCount() - 32)
					b.redAlert = -1;
			}
		}

		for(auto &b : bases) {
			if (b.redAlert == -1) {
				for (auto &mf : b.mineralMiners) {
					for (auto &w : mf.second) {
						if (w->isConstructing())
							continue;
						if ((w->isCarryingMinerals() || w->isCarryingGas()) && (w->getOrder() != BWAPI::Orders::ReturnGas && w->getOrder() != BWAPI::Orders::ReturnMinerals))
							w->returnCargo();
						else if (!(w->isCarryingMinerals() || w->isCarryingGas()) && w->getOrderTarget() != mf.first)
							w->gather(mf.first);
					}
				}

				for (auto &gg : b.gasMiners) {
					for (auto &w : gg.second) {
						if (w->isConstructing())
							continue;
						if ((w->isCarryingMinerals() || w->isCarryingGas()) && w->getOrderTarget() != b.resourceDepot)
							w->returnCargo();
						else if (!(w->isCarryingMinerals() || w->isCarryingGas()) && w->getOrderTarget() != gg.first)
							w->gather(gg.first);
					}
				}
			}

			for(auto &mf : b.mineralMiners)
				for (auto &w : mf.second) {
					auto enemyUnit = unitManager.getClosestVisibleEnemy(w, BWAPI::Filter::IsDetected, true);
					if (b.redAlert != -1 && enemyUnit.u && w->getDistance(enemyUnit.u) <= WORKERAGGRORADIUS)
						w->attack(enemyUnit.u);
					else {
						if (w->isConstructing())
							continue;
						if ((w->isCarryingMinerals() || w->isCarryingGas()) && (w->getOrder() != BWAPI::Orders::ReturnGas && w->getOrder() != BWAPI::Orders::ReturnMinerals))
							w->returnCargo();
						else if (!(w->isCarryingMinerals() || w->isCarryingGas()) && w->getOrderTarget() != mf.first)
							w->gather(mf.first);
					}
				}

			for (auto &gg : b.gasMiners)
				for (auto &w : gg.second) {
					auto enemyUnit = unitManager.getClosestVisibleEnemy(w, BWAPI::Filter::IsDetected, true);
					if (b.redAlert != -1 && enemyUnit.u && w->getDistance(enemyUnit.u) <= WORKERAGGRORADIUS)
						w->attack(enemyUnit.u);
					else {
						if (w->isConstructing())
							continue;
						if ((w->isCarryingMinerals() || w->isCarryingGas()) && w->getOrderTarget() != b.resourceDepot)
							w->returnCargo();
						else if (!(w->isCarryingMinerals() || w->isCarryingGas()) && w->getOrderTarget() != gg.first)
							w->gather(gg.first);
					}
				}
		}
	}

	void BaseManager::takeUnit(BWAPI::Unit unit) {
		looseWorkers.insert(unit);
		onUnitDestroy(unit);
	}

	void BaseManager::giveBackUnit(BWAPI::Unit unit) {
		looseWorkers.erase(unit);
		onUnitComplete(unit);
	}

	BWAPI::Unit BaseManager::findBuilder(BWAPI::UnitType builderType) {
		{
			auto homelessIt = homelessWorkers.begin();
			if (homelessIt != homelessWorkers.end()) {
				BWAPI::Unit ret = *homelessIt;
				homelessWorkers.erase(homelessIt);
				return ret;
			}
		}

		{
			unsigned workersAtBestPatch;
			BWAPI::Unit preferredUnit = nullptr;

			for (auto &b : bases) {
				for (auto min : b.mineralMiners) {
					if (min.second.size()) {
						if (preferredUnit == nullptr) {
							for (auto &u : min.second) {
								if (!u->isCarryingMinerals()) {
									preferredUnit = *(min.second.begin());
									workersAtBestPatch = min.second.size();
									continue;
								}
							}
						}

						if (preferredUnit == nullptr || min.second.size() > workersAtBestPatch) {
							for (auto &u : min.second) {
								if (!u->isCarryingMinerals()) {
									preferredUnit = *(min.second.begin());
									workersAtBestPatch = min.second.size();
									continue;
								}
							}
						}
					}
				}
			}

			if (preferredUnit != nullptr)
				return preferredUnit;
		}

		{
			unsigned workersAtBestPatch;
			BWAPI::Unit preferredUnit = nullptr;

			for (auto &b : bases) {
				for (auto min : b.mineralMiners) {
					if (min.second.size()) {
						if (preferredUnit == nullptr) {
							for (auto &u : min.second) {
								if (u->isCarryingMinerals()) {
									preferredUnit = *(min.second.begin());
									workersAtBestPatch = min.second.size();
									continue;
								}
							}
						}

						if (min.second.size() > workersAtBestPatch) {
							for (auto &u : min.second) {
								if (u->isCarryingMinerals()) {
									preferredUnit = *(min.second.begin());
									workersAtBestPatch = min.second.size();
									continue;
								}
							}
						}
					}
				}
			}

			if (preferredUnit != nullptr)
				return preferredUnit;
		}

		return nullptr;
	}

	BWAPI::Unit BaseManager::findClosestBuilder(BWAPI::UnitType builderType, BWAPI::Position at) {
		return findBuilder(builderType);
	}

	void BaseManager::assignToMinerals(BWAPI::Unit unit) {
		//workerBaseLookup[unit] = base;
		BWAPI::Unit mostAttractiveMineral = nullptr;
		const Base *mostAttractiveBase = nullptr;
		unsigned mineralMinersAt;
		unsigned distanceTo;
		int resourcesAt;

		for (auto &b : bases) {
			for (auto &mf : b.mineralMiners)
				mf.second.erase(unit);
			for (auto &mf : b.gasMiners)
				mf.second.erase(unit);
		}

		for (auto &b : bases) {
			for (auto &mf : b.mineralMiners) {
				unsigned dist = mf.first->getDistance(b.resourceDepot);
				if (mf.second.size() < 2 && mostAttractiveMineral == nullptr) {
					mostAttractiveMineral = mf.first;
					mostAttractiveBase = &b;
					mineralMinersAt = mf.second.size();
					distanceTo = dist;
					resourcesAt = mf.first->getResources();
					continue;
				}

				else if (mostAttractiveMineral && mf.second.size() < 2 && mf.second.size() < mineralMinersAt) {
					mostAttractiveMineral = mf.first;
					mostAttractiveBase = &b;
					mineralMinersAt = mf.second.size();
					distanceTo = dist;
					resourcesAt = mf.first->getResources();
					continue;
				}
				else if (mostAttractiveMineral && mf.second.size() > mineralMinersAt)
					continue;

				else if (mostAttractiveMineral && dist < distanceTo) {
					mostAttractiveMineral = mf.first;
					mostAttractiveBase = &b;
					mineralMinersAt = mf.second.size();
					distanceTo = dist;
					resourcesAt = mf.first->getResources();
					continue;
				}
				else if (mostAttractiveMineral && distanceTo > dist)
					continue;

				else if (mostAttractiveMineral && mf.first->getResources() > resourcesAt) {
					mostAttractiveMineral = mf.first;
					mostAttractiveBase = &b;
					mineralMinersAt = mf.second.size();
					distanceTo = dist;
					resourcesAt = mf.first->getResources();
					continue;
				}
			}
		}

		if (mostAttractiveMineral != nullptr) {
			mostAttractiveBase->mineralMiners[mostAttractiveMineral].insert(unit);
			workerBaseLookup[unit] = mostAttractiveBase;
		}
	}

	void BaseManager::assignToGas(BWAPI::Unit unit) {
		BWAPI::Unit mostAttractiveGas = nullptr;
		const Base *mostAttractiveBase = nullptr;
		unsigned gasMinersAt;
		unsigned distanceTo;
		int resourcesAt;

		for (auto &b : bases) {
			for (auto &mf : b.mineralMiners)
				mf.second.erase(unit);
			for (auto &mf : b.gasMiners)
				mf.second.erase(unit);
		}

		for (auto &b : bases) {
			for (auto &gg : b.gasMiners) {
				unsigned dist = gg.first->getDistance(b.resourceDepot);
				if (gg.second.size() < 3 && mostAttractiveGas == nullptr) {
					mostAttractiveGas = gg.first;
					mostAttractiveBase = &b;
					gasMinersAt = gg.second.size();
					distanceTo = dist;
					resourcesAt = gg.first->getResources();
					continue;
				}

				else if (gg.second.size() < 3 && gg.second.size() < gasMinersAt) {
					mostAttractiveGas = gg.first;
					mostAttractiveBase = &b;
					gasMinersAt = gg.second.size();
					distanceTo = dist;
					resourcesAt = gg.first->getResources();
					continue;
				}
				else if (mostAttractiveGas && gg.second.size() > gasMinersAt)
					continue;

				else if (mostAttractiveGas && dist < distanceTo) {
					mostAttractiveGas = gg.first;
					mostAttractiveBase = &b;
					gasMinersAt = gg.second.size();
					distanceTo = dist;
					resourcesAt = gg.first->getResources();
					continue;
				}
				else if (mostAttractiveGas && distanceTo > dist)
					continue;

				else if (mostAttractiveGas && gg.first->getResources() > resourcesAt) {
					mostAttractiveGas = gg.first;
					mostAttractiveBase = &b;
					gasMinersAt = gg.second.size();
					distanceTo = dist;
					resourcesAt = gg.first->getResources();
					continue;
				}
			}
		}

		if (mostAttractiveGas != nullptr) {
			mostAttractiveBase->gasMiners[mostAttractiveGas].insert(unit);
			workerBaseLookup[unit] = mostAttractiveBase;
		}
	}

	const std::set <Base> &BaseManager::getAllBases() const {
		return bases;
	}

	const std::set<BWAPI::Unit> &BaseManager::getHomelessWorkers() const {
		return homelessWorkers;
	}

	const std::set<BWAPI::Unit>& BaseManager::getLooseWorkers() const {
		return looseWorkers;
	}

}
