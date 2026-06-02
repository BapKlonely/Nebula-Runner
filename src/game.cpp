#include "game.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>

#include "ftxui/dom/elements.hpp"

namespace nebula {
namespace {

using ftxui::bgcolor;
using ftxui::bold;
using ftxui::border;
using ftxui::center;
using ftxui::Color;
using ftxui::Element;
using ftxui::flex;
using ftxui::hbox;
using ftxui::paragraph;
using ftxui::separator;
using ftxui::text;
using ftxui::vbox;

constexpr auto kComboWindow = 24;

Position StepToward(Position from, Position to) {
  const int dx = to.x - from.x;
  const int dy = to.y - from.y;
  if (std::abs(dx) > std::abs(dy)) {
    return {dx > 0 ? 1 : -1, 0};
  }
  if (dy != 0) {
    return {0, dy > 0 ? 1 : -1};
  }
  return {0, 0};
}

}  // namespace

GameComponent::GameComponent(std::function<void()> quit)
    : quit_(std::move(quit)), rng_(std::random_device{}()) {
  Reset();
}

void GameComponent::Reset() {
  drones_.clear();
  stars_.clear();
  player_ = {kWidth / 2, kHeight / 2};
  crystal_ = {3, 3};
  explosion_ = {-1, -1};
  has_explosion_ = false;
  paused_ = false;
  game_over_ = false;
  score_ = 0;
  lives_ = 3;
  level_ = 1;
  combo_ = 1;
  tick_ = 0;
  last_collect_tick_ = -1000;

  std::uniform_int_distribution<int> x_dist(0, kWidth - 1);
  std::uniform_int_distribution<int> y_dist(0, kHeight - 1);
  for (int i = 0; i < 42; ++i) {
    stars_.push_back({x_dist(rng_), y_dist(rng_)});
  }

  SpawnCrystal();
  EnsureDroneCount();
}

ftxui::Element GameComponent::Render() {
  using ftxui::color;

  std::vector<Element> rows;
  rows.reserve(kHeight);
  for (int y = 0; y < kHeight; ++y) {
    std::vector<Element> cells;
    cells.reserve(kWidth);
    for (int x = 0; x < kWidth; ++x) {
      Position p{x, y};
      Tile tile = Tile::Empty;
      if (std::find_if(stars_.begin(), stars_.end(), [&](Position star) {
            return star.x == x && star.y == y;
          }) != stars_.end()) {
        tile = Tile::Star;
      }
      if (crystal_.x == x && crystal_.y == y) {
        tile = Tile::Crystal;
      }
      if (OccupiedByDrone(p)) {
        tile = Tile::Drone;
      }
      if (has_explosion_ && explosion_.x == x && explosion_.y == y) {
        tile = Tile::Explosion;
      }
      if (player_.x == x && player_.y == y) {
        tile = Tile::Player;
      }
      cells.push_back(Cell(tile));
    }
    rows.push_back(hbox(std::move(cells)));
  }

  auto title = hbox({
                   text(" NEBULA RUNNER ") | bold |
                       color(Color::RGB(118, 220, 255)),
                   text("  星云矿区侦察任务") | color(Color::RGB(190, 210, 220)),
                 }) |
               center;

  auto stats =
      hbox({
          text(" Score ") | bold,
          text(std::to_string(score_)) | color(Color::RGB(105, 230, 170)) | bold,
          text("   Best ") | bold,
          text(std::to_string(best_score_)) | color(Color::RGB(255, 215, 112)),
          text("   Level ") | bold,
          text(std::to_string(level_)) | color(Color::RGB(118, 220, 255)),
          text("   Lives ") | bold,
          text(std::string(lives_, '<')) | color(Color::RGB(255, 105, 125)),
          text("   Combo x") | bold,
          text(std::to_string(combo_)) | color(Color::RGB(255, 180, 95)),
      });

  auto board = vbox(std::move(rows)) | border;
  auto help =
      vbox({
          text("控制") | bold | color(Color::RGB(118, 220, 255)),
          text("WASD / 方向键  移动"),
          text("P              暂停"),
          text("R              重新开始"),
          text("Q / Esc        退出"),
          separator(),
          text("图例") | bold | color(Color::RGB(118, 220, 255)),
          hbox({Cell(Tile::Player), text("  飞船")}),
          hbox({Cell(Tile::Crystal), text("  能量晶体")}),
          hbox({Cell(Tile::Drone), text("  巡航无人机")}),
          hbox({Cell(Tile::Star), text("  星尘")}),
          separator(),
          paragraph(StatusText()) | color(Color::RGB(210, 220, 230)),
      }) |
      border;

  auto overlay = text("");
  if (paused_) {
    overlay = text("  PAUSED  ") | bold | color(Color::Black) |
              bgcolor(Color::RGB(255, 215, 112));
  }
  if (game_over_) {
    overlay = text("  GAME OVER - 按 R 重开  ") | bold | color(Color::White) |
              bgcolor(Color::RGB(210, 65, 85));
  }

  return vbox({
             title,
             separator(),
             stats | center,
             hbox({board, separator(), help | flex}) | flex,
             overlay | center,
           }) |
         bgcolor(Color::RGB(10, 14, 24));
}

bool GameComponent::OnEvent(ftxui::Event event) {
  if (event == ftxui::Event::Escape || event == ftxui::Event::Character("q") ||
      event == ftxui::Event::Character("Q")) {
    quit_();
    return true;
  }

  if (event == ftxui::Event::Character("r") ||
      event == ftxui::Event::Character("R")) {
    Reset();
    return true;
  }

  if (event == ftxui::Event::Character("p") ||
      event == ftxui::Event::Character("P")) {
    if (!game_over_) {
      paused_ = !paused_;
    }
    return true;
  }

  if (event == ftxui::Event::Custom) {
    Tick();
    return true;
  }

  if (paused_ || game_over_) {
    return false;
  }

  if (event == ftxui::Event::ArrowUp ||
      event == ftxui::Event::Character("w") ||
      event == ftxui::Event::Character("W")) {
    MovePlayer(0, -1);
    return true;
  }
  if (event == ftxui::Event::ArrowDown ||
      event == ftxui::Event::Character("s") ||
      event == ftxui::Event::Character("S")) {
    MovePlayer(0, 1);
    return true;
  }
  if (event == ftxui::Event::ArrowLeft ||
      event == ftxui::Event::Character("a") ||
      event == ftxui::Event::Character("A")) {
    MovePlayer(-1, 0);
    return true;
  }
  if (event == ftxui::Event::ArrowRight ||
      event == ftxui::Event::Character("d") ||
      event == ftxui::Event::Character("D")) {
    MovePlayer(1, 0);
    return true;
  }

  return false;
}

void GameComponent::Tick() {
  if (paused_ || game_over_) {
    return;
  }

  ++tick_;
  has_explosion_ = false;
  level_ = 1 + score_ / 180;
  EnsureDroneCount();

  if (tick_ % TickDelay() == 0) {
    MoveDrones();
  }

  if (tick_ - last_collect_tick_ > kComboWindow) {
    combo_ = 1;
  }

  HandleCollision();
}

void GameComponent::MovePlayer(int dx, int dy) {
  Position next{player_.x + dx, player_.y + dy};
  if (!IsInside(next)) {
    return;
  }
  player_ = next;
  HandleCollision();
}

void GameComponent::MoveDrones() {
  std::uniform_int_distribution<int> chance(0, 99);
  for (auto& drone : drones_) {
    Position step = drone.dir;
    if (chance(rng_) < 36 + level_ * 3) {
      step = StepToward(drone.pos, player_);
    } else if (chance(rng_) < 24) {
      std::array<Position, 4> dirs = {{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
      step = dirs[static_cast<size_t>(chance(rng_) % dirs.size())];
    }

    Position next{drone.pos.x + step.x, drone.pos.y + step.y};
    if (!IsInside(next)) {
      drone.dir = {-step.x, -step.y};
      next = {drone.pos.x + drone.dir.x, drone.pos.y + drone.dir.y};
    }
    if (IsInside(next)) {
      drone.pos = next;
      drone.dir = step;
    }
  }
}

void GameComponent::SpawnCrystal() {
  crystal_ = RandomEmptyPosition();
}

void GameComponent::EnsureDroneCount() {
  std::uniform_int_distribution<int> dir_dist(0, 3);
  std::array<Position, 4> dirs = {{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
  while (static_cast<int>(drones_.size()) < TargetDroneCount()) {
    Position p = RandomEmptyPosition();
    const int distance = std::abs(p.x - player_.x) + std::abs(p.y - player_.y);
    if (distance < 7) {
      continue;
    }
    drones_.push_back({p, dirs[static_cast<size_t>(dir_dist(rng_))]});
  }
}

void GameComponent::HandleCollision() {
  if (player_.x == crystal_.x && player_.y == crystal_.y) {
    if (tick_ - last_collect_tick_ <= kComboWindow) {
      combo_ = std::min(combo_ + 1, 9);
    } else {
      combo_ = 1;
    }
    last_collect_tick_ = tick_;
    AddScore(25 * combo_);
    SpawnCrystal();
  }

  if (OccupiedByDrone(player_)) {
    --lives_;
    combo_ = 1;
    explosion_ = player_;
    has_explosion_ = true;
    player_ = {kWidth / 2, kHeight / 2};
    if (lives_ <= 0) {
      game_over_ = true;
      best_score_ = std::max(best_score_, score_);
    }
  }
}

void GameComponent::AddScore(int value) {
  score_ += value;
  best_score_ = std::max(best_score_, score_);
}

bool GameComponent::IsInside(Position p) const {
  return p.x >= 0 && p.x < kWidth && p.y >= 0 && p.y < kHeight;
}

bool GameComponent::OccupiedByDrone(Position p) const {
  return std::any_of(drones_.begin(), drones_.end(), [&](const Drone& drone) {
    return drone.pos.x == p.x && drone.pos.y == p.y;
  });
}

Position GameComponent::RandomEmptyPosition() {
  std::uniform_int_distribution<int> x_dist(0, kWidth - 1);
  std::uniform_int_distribution<int> y_dist(0, kHeight - 1);
  while (true) {
    Position p{x_dist(rng_), y_dist(rng_)};
    if ((p.x == player_.x && p.y == player_.y) || OccupiedByDrone(p)) {
      continue;
    }
    return p;
  }
}

int GameComponent::TargetDroneCount() const {
  return std::min(4 + level_, 13);
}

int GameComponent::TickDelay() const {
  return std::max(2, 8 - level_ / 2);
}

std::string GameComponent::StatusText() const {
  if (game_over_) {
    return "任务失败。无人机封锁了航线，按 R 可立即重新部署。";
  }
  if (paused_) {
    return "任务暂停。观察路线后按 P 继续。";
  }
  std::ostringstream out;
  out << "连续收集晶体可提高倍率。当前无人机数量 "
      << drones_.size() << "，移动间隔 " << TickDelay() << " 个节拍。";
  return out.str();
}

ftxui::Element GameComponent::Cell(Tile tile) const {
  using ftxui::color;

  switch (tile) {
    case Tile::Player:
      return text("A ") | bold | color(Color::RGB(20, 245, 220)) |
             bgcolor(Color::RGB(13, 45, 58));
    case Tile::Crystal:
      return text("<>") | bold | color(Color::RGB(130, 220, 255)) |
             bgcolor(Color::RGB(18, 41, 76));
    case Tile::Drone:
      return text("[]") | bold | color(Color::RGB(255, 105, 125)) |
             bgcolor(Color::RGB(65, 18, 30));
    case Tile::Explosion:
      return text("**") | bold | color(Color::RGB(255, 230, 120)) |
             bgcolor(Color::RGB(105, 42, 18));
    case Tile::Star:
      return text(". ") | color(Color::RGB(95, 115, 145)) |
             bgcolor(Color::RGB(10, 14, 24));
    case Tile::Empty:
    default:
      return text("  ") | bgcolor(Color::RGB(10, 14, 24));
  }
}

}  // namespace nebula
