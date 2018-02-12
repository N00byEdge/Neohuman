#pragma once

#include "BWAPI.h"
#include "UnitManager.h"

#include <set>

namespace Neolib {

struct FastAPproximation {
  struct FAPUnit {
    FAPUnit(BWAPI::Unit u);
    FAPUnit(EnemyData ed);
    const FAPUnit &operator=(const FAPUnit &other) const;

    int id = 0;

    mutable int x = 0, y = 0;

    mutable int health = 0;
    mutable int maxHealth = 0;
    mutable int armor = 0;

    mutable int shields = 0;
    mutable int shieldArmor = 0;
    mutable int maxShields = 0;

    mutable double speed = 0;
    mutable bool flying = 0;
    mutable int elevation = -1;

    mutable BWAPI::UnitSizeType unitSize;

    mutable int groundDamage = 0;
    mutable int groundCooldown = 0;
    mutable int groundMaxRange = 0;
    mutable int groundMinRange = 0;
    mutable BWAPI::DamageType groundDamageType;

    mutable int airDamage = 0;
    mutable int airCooldown = 0;
    mutable int airMaxRange = 0;
    mutable int airMinRange = 0;
    mutable BWAPI::DamageType airDamageType;

    mutable BWAPI::UnitType unitType;
    mutable BWAPI::Player player = nullptr;
    mutable bool isOrganic = false;
    mutable bool didHealThisFrame = false;
    mutable int score = 0;

    mutable int attackCooldownRemaining = 0;

    bool operator<(const FAPUnit &other) const;
  };

  /**
   * \brief Creates a new FasAPproximation. It will have no units or other state
   */
  FastAPproximation();

  /**
   * \brief Adds the unit to the simulator for player 1
   * \param fu The FAPUnit to add
   */
  void addUnitPlayer1(FAPUnit fu);
  /**
   * \brief Adds the unit to the simulator for player 1, only if it is a combat unit
   * \param fu The FAPUnit to add
   */
  void addIfCombatUnitPlayer1(FAPUnit fu);
  /**
   * \brief Adds the unit to the simulator for player 2
   * \param fu The FAPUnit to add
   */
  void addUnitPlayer2(FAPUnit fu);
  /**
   * \brief Adds the unit to the simulator for player 2, only if it is a combat unit
   * \param fu The FAPUnit to add
   */
  void addIfCombatUnitPlayer2(FAPUnit fu);

  /**
   * \brief Starts the simulation. You can run this function multiple times. Feel free to run once, get the state and keep running.
   * \param nFrames the number of frames to simulate. A negative number runs the sim until combat is over.
   */
  void simulate(int nFrames = 96); // = 24*4, 4 seconds on fastest

  /**
   * \brief Default score calculation function
   * \return Returns the score for alive units, for each player
   */
  std::pair<int, int> playerScores() const;
  /**
   * \brief Default score calculation, only counts non-buildings.
   * \return Returns the score for alive non-buildings, for each player
   */
  std::pair<int, int> playerScoresUnits() const;
  /**
   * \brief Default score calculation, only counts buildings.
   * \return Returns the score for alive buildings, for each player
   */
  std::pair<int, int> playerScoresBuildings() const;
  /**
   * \brief Gets the internal state of the simulator. You can use this to get any info about the unit participating in the simulation or edit the state.
   * \return Returns a pair of pointers, where each pointer points to a vector containing that player's units.
   */
  std::pair<std::vector<FAPUnit> *, std::vector<FAPUnit> *> getState();
  /**
   * \brief Clears the simulation. All units are removed for both players. Equivalent to reconstructing.
   */
  void clear();

private:
  std::vector<FAPUnit> player1, player2;

  bool didSomething = false;
  static void dealDamage(const FAPUnit &fu, int damage, BWAPI::DamageType damageType);
  static int distButNotReally(const FAPUnit &u1, const FAPUnit &u2);
  static bool isSuicideUnit(BWAPI::UnitType ut);
  void unitsim(const FAPUnit &fu, std::vector<FAPUnit> &enemyUnits);
  static void medicsim(const FAPUnit &fu, std::vector<FAPUnit> &friendlyUnits);
  bool suicideSim(const FAPUnit &fu, std::vector<FAPUnit> &enemyUnits);
  void isimulate();
  static void unitDeath(const FAPUnit &fu, std::vector<FAPUnit> &itsFriendlies);
  static void convertToUnitType(const FAPUnit &fu, BWAPI::UnitType ut);
};

} // namespace Neolib

extern Neolib::FastAPproximation fap;
