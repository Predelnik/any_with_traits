#include "gtest.h"
#include "any_with_traits.h"

#include <memory>
#include <functional>

struct C {
  C () {}
  ~C () {
  std::cout << "destruction\n";
}
C (const C &) {
  std::cout << "copy\n";
}
C(C &&) {
  std::cout << "move\n";
}
};

TEST(normal_any_type, all)
{
  C p;
  {
    any_t<any_trait::destructible, any_trait::copiable> x(p);
    auto y = x;
  }
  {
    any_t<any_trait::destructible> x(p);
    //auto y = x;
  }
  {
    any_t<any_trait::destructible, any_trait::movable> x(p);
    auto y = std::move(x);
    x = std::make_unique<C>(p);
  }
  {
    any_t<any_trait::destructible, any_trait::movable, any_trait::callable<int(int, int)>> x;
    x = [](int a, int b) { return a + b;  };
    std::cout << x(23, 15) << '\n';
  }
}
