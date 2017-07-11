#include "CombatSimulator.h"

Neolib::CombatSimulator combatSimulator;

SparCraft::Player_AttackClosest me(SparCraft::Players::Player_One);
SparCraft::Player_AttackClosest other(SparCraft::Players::Player_One);

SparCraft::PlayerPtr pme = SparCraft::PlayerPtr(&me);
SparCraft::PlayerPtr pother = SparCraft::PlayerPtr(&other);

namespace Neolib {
	CombatSimulator::CombatSimulator() {

	}

	void CombatSimulator::init() {
		myMap = SparCraft::Map::Map(BWAPI::Broodwar);
		myMapPtr = std::make_shared <SparCraft::Map>(myMap);
		state.setMap(myMapPtr);
		BWAPI::Broodwar->sendText("%ld", myMapPtr.use_count());
	}

	void CombatSimulator::addUnitToSimulator(int id, BWAPI::Position pos, BWAPI::UnitType ut, bool isEnemy,  int health, int shields) {
		SparCraft::Unit u2(ut, pos, id, isEnemy ? SparCraft::Players::Player_Two : SparCraft::Players::Player_One, health + shields, 0, BWAPI::Broodwar->getFrameCount(), BWAPI::Broodwar->getFrameCount());
		//u2.setUnitID(id);
		state.addUnit(u2);
	}

	void CombatSimulator::clear() {
		me = SparCraft::Player_AttackClosest(SparCraft::Players::Player_One);
		other = SparCraft::Player_AttackClosest(SparCraft::Players::Player_One);
		state = SparCraft::GameState();
		state.setMap(myMapPtr);
	}
	
	bool CombatSimulator::simulate() {
		SparCraft::Game g(state, pme, pother, 300);
		try {
			g.play();
		}
		catch (SparCraft::SparCraftException e) {
			// Expected. Whatever.
		}
		SparCraft::GameState state = g.getState();
		auto winner = state.winner();
		return winner == SparCraft::Players::Player_One;
	}

	void CombatSimulator::onStart() {
		SparCraft::init();
		SparCraft::AIParameters::Instance().parseFile("bwapi-data\\AI\\SparCraft_Config.txt");
	}

}
