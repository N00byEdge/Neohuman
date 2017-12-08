#include "SquadManager.h"

Neolib::SquadManager squadManager;

namespace Neolib {

	Squad::Squad() : id([]() {
		static int nextid = 0;
		return nextid++;
	}()) {

	}

	void EnemySquad::addUnit(BWAPI::Unit unit) {
		units.insert(unitManager.getEnemyData(unit));
	}

	void EnemySquad::removeUnit(BWAPI::Unit unit) {
		units.erase(unitManager.getEnemyData(unit));
	}

	void EnemySquad::updatePosition() {
		int xsum = 0, ysum = 0;
		for (auto &u : units) {
			xsum += u->lastPosition.x;
			ysum += u->lastPosition.y;
		}

		pos = BWAPI::Position(xsum / (int)units.size(), ysum / (int)units.size());

		for (auto &u : units)
			radius1 += (pos.x - u->lastPosition.x) * (pos.x - u->lastPosition.x) +
					   (pos.y - u->lastPosition.y) * (pos.y - u->lastPosition.y);

		radius1 = (int)sqrt(radius1);
		radius2 = radius1 * 3 + 100;
		radius1 *= 2;
		radius1 += 100;

		MINE(radius1, 500);
		MINE(radius2, 600);
	}

	void FriendlySquad::addUnit(BWAPI::Unit unit) {
		units.insert(unit);
	}

	void FriendlySquad::removeUnit(BWAPI::Unit unit) {
		units.erase(unit);
	}

	void FriendlySquad::updatePosition() {
		int xsum = 0, ysum = 0;
		for (auto &u : units) {
			xsum += u->getPosition().x;
			ysum += u->getPosition().y;
		}

		pos = BWAPI::Position(xsum / (int)units.size(), ysum / (int)units.size());

		for (auto &u : units)
			radius1 += (pos.x - u->getPosition().x) * (pos.x - u->getPosition().x) +
			(pos.y - u->getPosition().y) * (pos.y - u->getPosition().y);

		radius1 = (int)sqrt(radius1);
		radius2 = radius1 * 3;
		radius1 *= 2;

		MINE(radius1, 500);
		MINE(radius2, 600);
	}

	bool SquadManager::shouldAttack(SimResults sr) {
		BWAPI::Broodwar << sr.shortLosses;
		return (sr.postsim.scores.second * 5 <= sr.presim.scores.second) || (sr.shortLosses >= 0);
	}

	void SquadManager::onFrame() {
		// Clean up 0 sized squads
		for (auto it = enemySquads.begin(); it != enemySquads.end();)
			if (!(*it)->numUnits())
				it = enemySquads.erase(it);
			else ++it;

		for (auto it = friendlySquads.begin(); it != friendlySquads.end();)
			if (!(*it)->numUnits())
				it = friendlySquads.erase(it);
			else ++it;

		std::set<EnemySquad *> engagedEnemySquads;

		for (auto &fs : friendlySquads) {
			fs->updatePosition();
			for (auto &u : fs->units)
				if (fs->pos.getApproxDistance(u->getPosition()) > fs->radius2)
					removeFromSquad(u);
		}

		for (auto &es : enemySquads) {
			es->updatePosition();
			for (auto &u : es->units)
				if (es->pos.getApproxDistance(u->lastPosition) > es->radius2)
					removeFromSquad(u->u);
		}

		// Clean up 0 sized squads
		for (auto it = enemySquads.begin(); it != enemySquads.end();)
			if (!(*it)->numUnits())
				it = enemySquads.erase(it);
			else ++it;

			for (auto it = friendlySquads.begin(); it != friendlySquads.end();)
				if (!(*it)->numUnits())
					it = friendlySquads.erase(it);
				else ++it;

		for (auto &fs : friendlySquads) {
			// Engage nearby squads
			for (auto &es : enemySquads)
				if (fs->pos.getApproxDistance(es->pos) < 1000)
					fs->engagedSquads.insert(es.get()), es->engagedSquads.insert(fs.get());
		}

		// Try attacking with each squad
		for (auto &fs : friendlySquads) {
			FastAPproximation fap;
			for (auto &u : fs->units)
				fap.addIfCombatUnitPlayer1(u);

			for (auto &es : fs->engagedSquads)
				for (auto &u : ((EnemySquad *)es)->units)
					fap.addIfCombatUnitPlayer2(*u);

			fs->simres.presim.scores = fap.playerScores();
			fs->simres.presim.unitCounts = { (int)fap.getState().first->size(), (int)fap.getState().second->size() };

			fap.simulate(24 * 4); // Short sim

			fs->simres.shortsim.scores = fap.playerScores();
			fs->simres.shortsim.unitCounts = { (int)fap.getState().first->size(), (int)fap.getState().second->size() };
			int theirLosses = fs->simres.presim.scores.second - fs->simres.shortsim.scores.second;
			int ourLosses = fs->simres.presim.scores.first - fs->simres.shortsim.scores.first;
			fs->simres.shortLosses = theirLosses - ourLosses;

			fap.simulate(24 * 56); // The rest of the sim

			fs->simres.postsim.scores = fap.playerScores();
			fs->simres.postsim.unitCounts = { (int)fap.getState().first->size(), (int)fap.getState().second->size() };
		}

		// Squad merging
		bool escape = false;
		for (auto s1 = friendlySquads.begin(); !escape && s1 != friendlySquads.end(); ++s1) {
			for (auto s2 = friendlySquads.begin(); !escape && s2 != friendlySquads.end();) {
				if (s1 == s2) {
					++s2;
					continue;
				}
				// Very hacky merge
				if ((*s1)->pos.getApproxDistance((*s2)->pos) < (*s1)->radius1) {
					std::shared_ptr<FriendlySquad> fs = *s2; // Hold this squad for a bit
					for (auto &u : fs->units) {
						(*s1)->addUnit(u);
						squadLookup[u] = *s1;
					}

					fs->units.clear();
					s2 = friendlySquads.erase(s2);
					removeSquad(fs.get());
				}
				else ++s2;
			}
			if (escape)
				break;
		}

		// Enemy squad merging
		escape = false;
		for (auto s1 = enemySquads.begin(); !escape && s1 != enemySquads.end(); ++s1) {
			for (auto s2 = enemySquads.begin(); !escape && s2 != enemySquads.end();) {
				if (s1 == s2) {
					++s2;
					continue;
				}
				// Very hacky merge
				if ((*s1)->pos.getApproxDistance((*s2)->pos) < (*s1)->radius1) {
					std::shared_ptr<EnemySquad> fs = *s2; // Hold this squad for a bit
					for (auto &u : fs->units) {
						(*s1)->addUnit(u->u);
						squadLookup[u->u] = *s1;
					}

					fs->units.clear();
					s2 = enemySquads.erase(s2);
					removeSquad(fs.get());
				}
				else ++s2;
			}
			if (escape)
				break;
		}
	}

	void SquadManager::onUnitDeath(BWAPI::Unit unit) {
		unmanagedUnits.erase(unit);
	}

	void SquadManager::onUnitCreate(BWAPI::Unit unit) {
		unmanagedUnits.insert(unit);
	}

	void SquadManager::onUnitRenegade(BWAPI::Unit unit) {
		onUnitDeath(unit);
		onUnitCreate(unit);
	}

	void SquadManager::onEnemyRecognize(std::shared_ptr<EnemyData> ed) { /*
		if (isInSquad(ed->u))
			return;

		for (auto &sq : enemySquads) {
			if (sq->pos.getApproxDistance(ed->lastPosition) < sq->radius2) { // If close enough to join this squad...
				sq->addUnit(ed->u); // ...then join it. As simple as that.
				return;
			}
		}

		// Create its own squad, with blackjack and hookers!
		addToNewSquad(ed->u);*/
	}

	void SquadManager::onEnemyLose(std::shared_ptr<EnemyData> ed) {
		removeFromSquad(ed->u);
	}

	std::shared_ptr<Squad> SquadManager::getSquad(BWAPI::Unit unit) {
		auto it = squadLookup.find(unit);
		if (it != squadLookup.end()) return it->second;
		else return nullptr;
	}

	std::shared_ptr<Squad> SquadManager::addToNewSquad(BWAPI::Unit unit) {
		std::shared_ptr <Squad> ret;
		
		if (unitManager.isOwn(unit)) {
			auto fsq = std::make_shared <FriendlySquad>();
			friendlySquads.push_back(fsq);
			ret = fsq;
		}

		else if (unitManager.isEnemy(unit)) {
			auto esq = std::make_shared <EnemySquad>();
			enemySquads.push_back(esq);
			ret = esq;
		}

		if(ret) {
			unmanagedUnits.erase(unit);
			squadLookup[unit] = ret;
			ret->addUnit(unit);
		}

		return ret;
	}

	std::shared_ptr<Squad> SquadManager::addNewFriendlySquad() {
		auto ret = std::make_shared<FriendlySquad>();
		friendlySquads.push_back(ret);
		return ret;
	}

	std::shared_ptr<Squad> SquadManager::addNewEnemySquad() {
		auto ret = std::make_shared<EnemySquad>();
		enemySquads.push_back(ret);
		return ret;
	}

	bool SquadManager::isInSquad(BWAPI::Unit unit) {
		return getSquad(unit) != nullptr;
	}

	void SquadManager::removeFromSquad(BWAPI::Unit unit) {
		auto it = squadLookup.find(unit);
		if (it != squadLookup.end()) {
			it->second->removeUnit(unit);	
			squadLookup.erase(unit);
		}
	}

	std::vector<std::shared_ptr<EnemySquad>> &SquadManager::getEnemySquads() {
		return enemySquads;
	}

	std::vector<std::shared_ptr<FriendlySquad>> &SquadManager::getFriendlySquads() {
		return friendlySquads;
	}

	void SquadManager::removeSquad(FriendlySquad *sq) {
		for (auto it = friendlySquads.begin(); it != friendlySquads.end();)
			if (it->get() == sq)
				it = friendlySquads.erase(it);
			else ++it;

		for (auto &s : enemySquads)
			s->squadWasRemoved(sq);

		removeSquadReferences(sq);
	}

	void SquadManager::removeSquad(EnemySquad *sq) {
		for (auto it = enemySquads.begin(); it != enemySquads.end();)
			if (it->get() == sq)
				it = enemySquads.erase(it);
			else ++it;

		for (auto &s : friendlySquads)
			s->squadWasRemoved(sq);

		removeSquadReferences(sq);
	}

	void SquadManager::removeSquadReferences(Squad *sq) {
		for (auto it = squadLookup.begin(); it != squadLookup.end();) // It shouldn't exist here, but let's be on the safe side.
			if (it->second.get() == sq)
				it = squadLookup.erase(it);
			else ++it;
	}

	void SquadManager::addUnitToSquad(BWAPI::Unit unit, std::shared_ptr<Squad> sq) {
		sq->addUnit(unit);
		unmanagedUnits.erase(unit);
	}

}
