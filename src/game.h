#pragma once

#include <chrono>
#include <functional>
#include <random>
#include <string>
#include <vector>

#include "ftxui/component/component.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/dom/elements.hpp"

namespace nebula {

struct Position {
  int x = 0;
  int y = 0;
};

class GameComponent : public ftxui::ComponentBase {
 public:
  explicit GameComponent(std::function<void()> quit);

  ftxui::Element Render() override;
  bool OnEvent(ftxui::Event event) override;

 private:
  enum class Tile {
    Empty,
    Star,
    Crystal,
    Player,
    Drone,
    Explosion,
  };

  struct Drone {
    Position pos;
    Position dir;
  };

  static constexpr int kWidth = 28;
  static constexpr int kHeight = 16;

  void Reset();
  void Tick();
  void MovePlayer(int dx, int dy);
  void MoveDrones();
  void SpawnCrystal();
  void EnsureDroneCount();
  void HandleCollision();
  void AddScore(int value);

  bool IsInside(Position p) const;
  bool OccupiedByDrone(Position p) const;
  Position RandomEmptyPosition();
  int TargetDroneCount() const;
  int TickDelay() const;
  std::string StatusText() const;
  ftxui::Element Cell(Tile tile) const;

  std::function<void()> quit_;
  std::mt19937 rng_;
  std::vector<Drone> drones_;
  std::vector<Position> stars_;
  Position player_;
  Position crystal_;
  Position explosion_;
  bool has_explosion_ = false;
  bool paused_ = false;
  bool game_over_ = false;
  int score_ = 0;
  int best_score_ = 0;
  int lives_ = 3;
  int level_ = 1;
  int combo_ = 1;
  int tick_ = 0;
  int last_collect_tick_ = -1000;
};

}  // namespace nebula
