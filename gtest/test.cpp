#include "gtest.h"
#include "any_with_traits.h"

#include <array>
#include <memory>
#include <functional>

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
  {
    any_t <any_trait::destructible, any_trait::callable<int (int, int)>> v (std::plus<int> {});
    EXPECT_EQ (17, v (12, 5));
    v = std::minus<int> {};
    EXPECT_EQ(7, v(12, 5));
  }
  {
    any_t <any_trait::destructible, any_trait::comparable, any_trait::copiable> v(13);
    auto y = v;
    EXPECT_EQ(y, v);
    EXPECT_TRUE(13 == v);
    EXPECT_TRUE(v == 13);
    y = std::string ("Hello");
    EXPECT_EQ(std::string("Hello"), y);
    EXPECT_NE(13, y);
    EXPECT_TRUE(13 != y);
    EXPECT_TRUE(y != 13);
  }
}
