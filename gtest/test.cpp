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

TEST(any, all)
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
