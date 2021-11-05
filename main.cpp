#include<iomanip>
#include<cmath>
#include<chrono>
#include <bit>
#include <limits>
#include <cstdlib>

#include "Screen.h"
#include "ThreadPool.h"

std::uint64_t millis()
{
  std::uint64_t ns = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::
    now().time_since_epoch()).count();
  return ns;
}

// fast inverse square root from https://en.wikipedia.org/wiki/Fast_inverse_square_root.
constexpr float Q_rsqrt(float number) noexcept
{
  static_assert(std::numeric_limits<float>::is_iec559); // (enable only on IEEE 754)

  float const y = std::bit_cast<float>(
    0x5f3759df - (std::bit_cast<std::uint32_t>(number) >> 1));
  return y * (1.5f - (number * 0.5f * y * y));
}

struct Ball {
  int x;
  int y;
  int r;
  std::uint32_t color;
};

constexpr std::uint32_t blend_pixels(std::uint32_t a, std::uint32_t b, float p)
{
  // 0xRRGGBB
  float ar = static_cast<float>((a >> (8 * 2)) & 0xFF);
  float br = static_cast<float>((b >> (8 * 2)) & 0xFF);
  std::uint32_t nr = static_cast<std::uint32_t>(std::lerp(ar, br, p));

  float ag = static_cast<float>((a >> (8 * 1)) & 0xFF);
  float bg = static_cast<float>((b >> (8 * 1)) & 0xFF);
  std::uint32_t ng = static_cast<std::uint32_t>(std::lerp(ag, bg, p));

  float ab = static_cast<float>((a >> (8 * 0)) & 0xFF);
  float bb = static_cast<float>((b >> (8 * 0)) & 0xFF);
  std::uint32_t nb = static_cast<std::uint32_t>(std::lerp(ab, bb, p));

  return (nr << (8 * 2)) | (ng << (8 * 1)) | (nb << (8 * 0));
}

void spin_ball(Ball& ball, int width, int height)
{
  auto global_time = millis() * .001f;
  ball.x = width / 2 + std::sin(global_time) * ball.r * 2.f;
  ball.y = height / 2 + std::cos(global_time) * ball.r;
}

int main(int argc, char* argv[])
{
  constexpr int width = 16 * 100;
  constexpr int height = 9 * 100;
  Screen<width, height> screen("Meta-Balls");
  
  auto pixels = std::vector<std::uint32_t>(width * height);
  
  ThreadPool pool(std::thread::hardware_concurrency());
  std::vector<std::future<bool>> results;
  
  // initialize scene
  std::uint32_t background_color = 0x5555AA;
  Ball ball1 = { 200, 200, 100, 0x00EEEE22 };
  Ball ball2 = { width / 2, height/ 2, 100, 0x00EE22EE };
  
  bool is_running = true;
  auto last_shown_fps = millis();

  while (is_running) {
    auto start = millis();
    if (int event = screen.poll_events(); event == 1) {
      is_running = false;
    }
    std::fill(pixels.begin(), pixels.end(), background_color);
    std::size_t offset = height / std::thread::hardware_concurrency();
    for (std::size_t line_idx = 0; line_idx < height; line_idx += offset) {
      results.emplace_back(
        pool.add_task([&pixels, ball1, ball2, line_idx, line_length = width, offset]() {
          for (std::size_t y = 0; y < offset; ++y) {
            for (std::size_t x = 0; x < line_length; ++x) {
              double s1
              {
                Q_rsqrt((x - ball1.x) * (x - ball1.x) + ((line_idx + y) - ball1.y) * ((line_idx + y) - ball1.y))
              };
              double s2
              {
                Q_rsqrt((x - ball2.x) * (x - ball2.x) + ((line_idx + y) - ball2.y) * ((line_idx + y) - ball2.y))
              };
              if (double s = s1 + s2; s >= 0.005) {
                pixels[(line_idx + y) * line_length + x] = blend_pixels(ball1.color, ball2.color, s1 / s);
              }
            }
          }
          return true;
        })
      );
    }
    // wait until every line is calculated
    for (auto&& result : results) {
      result.get();
    }
    screen.update_texture(pixels);
    spin_ball(ball2, width, height);
    results.clear();
    if (auto elapsed = millis() - start; elapsed > 0) {
      if (millis() - last_shown_fps > 500) {
        std::cout << std::setw(4) << (1000 / elapsed) << " fps\r";
        last_shown_fps = millis();
      }
    }
  }
  return 0;
}
