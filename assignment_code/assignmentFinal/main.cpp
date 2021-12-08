#include <iostream>
#include <chrono>

#include "SkeletonViewerApp.hpp"

using namespace GLOO;

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0]
              << " PREFIX where PREFIX is "
                 "relative to assets/final"
              << std::endl;
    std::cout << "For example, if you're trying to load "
                 "dragon.obj, run with: "
              << argv[0] << " dragon" << std::endl;
    return -1;
  }
  std::unique_ptr<SkeletonViewerApp> app =
      make_unique<SkeletonViewerApp>("6837FinalProject", glm::ivec2(1440, 900),
                                     "final/" + std::string(argv[1]));

  app->SetupScene();

  using Clock = std::chrono::high_resolution_clock;
  using TimePoint =
      std::chrono::time_point<Clock, std::chrono::duration<double>>;
  TimePoint last_tick_time = Clock::now();
  TimePoint start_tick_time = last_tick_time;
  while (!app->IsFinished()) {
    TimePoint current_tick_time = Clock::now();
    double delta_time = (current_tick_time - last_tick_time).count();
    double total_elapsed_time = (current_tick_time - start_tick_time).count();
    app->Tick(delta_time, total_elapsed_time);
    last_tick_time = current_tick_time;
  }
  return 0;
}
