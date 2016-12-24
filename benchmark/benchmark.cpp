#define NONIUS_RUNNER

#include "nonius.h++"
#include "any_with_traits.h"

__declspec(noinline) int f(int x) { return std::abs(x); }

NONIUS_BENCHMARK("awt::function", [](nonius::chronometer meter) {
  awt::function<int(int)> p = &f;
  meter.measure([=](int i) { return p(i); });
})

NONIUS_BENCHMARK("function<void (int)>", [](nonius::chronometer meter) {
  std::function<int(int)> p = &f;
  meter.measure([=](int i) { return p (i); });
})

