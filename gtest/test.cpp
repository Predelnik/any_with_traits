#include "gtest.h"
#include "any_with_traits.h"

#include <array>
#include <memory>
#include <functional>

struct C {
  char vv[10000];
  int x = -123;
  C (int v) : x(v) {}
  ~C () {
  std::cout << "destruction\n";
}
  C &operator= (const C &) { return *this; }
  C &operator= (C &&) { return *this; }
  bool operator== (const C &other) const { return x == other.x;  }
  bool operator!= (const C &other) const { return x != other.x; }

C (const C &other) {
  std::cout << "copy\n";
  x = other.x;
}
C(C &&) {
  std::cout << "move\n";
}
};

TEST(any, all)
{
  {
    any_t<any_trait::destructible> v(123);
    EXPECT_TRUE(v.type() == typeid (int));
    EXPECT_EQ(123, *any_cast<int> (&v));
    EXPECT_EQ(nullptr, any_cast<double> (&v));
    EXPECT_EQ(123, any_cast<int> (v));
    try {
      any_cast<double> (v);
      FAIL() << "Expected std::bad_cast";
    }
    catch (std::bad_cast const &) {
    }
    catch (...) {
      FAIL() << "Expected std::bad_cast";
    }
  }

  {
    any_t < any_trait::destructible, any_trait::copiable, any_trait::movable> v(123);
    EXPECT_EQ(123, any_cast<int> (v));
    auto v2 = v;
    EXPECT_EQ(123, any_cast<int> (v2));
    auto v3 = std::move(v);
    EXPECT_EQ(123, any_cast<int> (v3));
  }

  {
    using huge_type = std::array<int, 123>;
    huge_type initial_value;
    initial_value.fill(123);

    any_t < any_trait::destructible, any_trait::copiable, any_trait::movable> v(initial_value);
    EXPECT_EQ(initial_value, any_cast<huge_type> (v));
    auto v2 = v;
    EXPECT_EQ(initial_value, any_cast<huge_type> (v2));
    auto v3 = std::move(v);
    EXPECT_EQ(initial_value, any_cast<huge_type> (v3));
  }
}
