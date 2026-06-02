#include <atomic>
#include <chrono>
#include <thread>

#include "ftxui/component/screen_interactive.hpp"
#include "game.h"

int main() {
  using namespace std::chrono_literals;

  auto screen = ftxui::ScreenInteractive::Fullscreen();
  std::atomic_bool running = true;

  auto game = ftxui::Make<nebula::GameComponent>(screen.ExitLoopClosure());

  std::thread ticker([&] {
    while (running) {
      std::this_thread::sleep_for(95ms);
      screen.PostEvent(ftxui::Event::Custom);
    }
  });

  screen.Loop(game);
  running = false;
  ticker.join();
  return 0;
}
