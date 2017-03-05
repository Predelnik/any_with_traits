#include "gtest.h"
#include "any_with_traits.h"

#include <array>
#include <map>
#include <memory>
#include <functional>
#include <unordered_set>
#include <algorithm>

TEST(any, all) {
  {
    awt::any<> v(123);
    EXPECT_TRUE(v.type() == typeid(int));
    EXPECT_EQ(123, *awt::any_cast<int>(&v));
    EXPECT_EQ(nullptr, awt::any_cast<double>(&v));
    EXPECT_EQ(123, awt::any_cast<int>(v));
    EXPECT_THROW(awt::any_cast<double>(v), std::bad_cast);
  }
  {
    awt::any<any_trait::copiable, any_trait::movable> v(123);
    EXPECT_EQ(123, awt::any_cast<int>(v));
    auto v2 = v;
    EXPECT_EQ(123, awt::any_cast<int>(v2));
    auto v3 = std::move(v);
    EXPECT_EQ(123, awt::any_cast<int>(v3));
    v3 = {};
    EXPECT_FALSE(v3.has_value());
  }

  {
    using huge_type = std::array<int, 123>;
    huge_type initial_value;
    initial_value.fill(123);

    awt::any<any_trait::copiable, any_trait::movable> v(initial_value);
    EXPECT_EQ(initial_value, awt::any_cast<huge_type>(v));
    auto v2 = v;
    EXPECT_EQ(initial_value, awt::any_cast<huge_type>(v2));
    auto v3 = std::move(v);
    EXPECT_EQ(initial_value, awt::any_cast<huge_type>(v3));
    v3.reset();
    EXPECT_FALSE(v3.has_value());
    v2 = v3;
    EXPECT_FALSE(v2.has_value());
    v = std::move(v3);
    EXPECT_FALSE(v.has_value());
  }
  {
    using any = awt::any<any_trait::callable<int(int, int)>>;
    any v(std::plus<int>{});
    EXPECT_EQ(17, v(12, 5));
    v = std::minus<int>{};
    EXPECT_EQ(7, v(12, 5));
    any p;
    EXPECT_THROW(p(12, 5), std::bad_function_call);
    int x = 12, y = 5;
    EXPECT_EQ(x - y, v(x, y));
  }
  {
    using any = awt::any<any_trait::comparable, any_trait::copiable>;
    any v(13);
    auto y = v;
    EXPECT_EQ(y, v);
    EXPECT_TRUE(13 == v);
    EXPECT_TRUE(v == 13);
    y = std::string("Hello");
    EXPECT_EQ(std::string("Hello"), y);
    EXPECT_NE(13, y);
    EXPECT_TRUE(13 != y);
    EXPECT_TRUE(y != 13);
    any empty;
    EXPECT_NE(empty, y);
    y = empty;
    EXPECT_EQ(y, empty);
  }
  {
    using any =
        awt::any<any_trait::orderable, any_trait::movable, any_trait::copiable>;
    std::vector<any> v;
    v.emplace_back(25);
    v.emplace_back(-15);
    v.emplace_back(3);
    std::sort(v.begin(), v.end());
    EXPECT_EQ(25, awt::any_cast<int>(v.back()));
    EXPECT_EQ(-15, awt::any_cast<int>(v.front()));
    std::map<any, int> m;
    m[13] = 27;
    m[std::string("acdcd")] = 34;
    m[std::vector<int>{1, 5, 3}] = 666;
    EXPECT_EQ(3u, m.size());
    EXPECT_EQ(666, m[std::vector<int>({1, 5, 3})]);
    EXPECT_LT(any{}, m[13]);
    EXPECT_FALSE(any{} < any{});
    EXPECT_FALSE(any{} > any{});
  }
  {
    using any = awt::any<any_trait::movable, any_trait::copiable,
                         any_trait::hashable, any_trait::comparable>;
    std::unordered_set<any> vv;
    vv.insert(17);
    vv.insert(13);
    vv.insert(19);
    vv.insert(std::string("13"));
    vv.insert(any{}); // empty any is fine for hashing
    EXPECT_EQ(1, vv.count(std::string("13")));
    EXPECT_EQ(0, vv.count(std::string("27")));
    EXPECT_EQ(1, vv.count({}));
  }
  {
    auto value = std::make_unique<int>(42);
    auto f = [a = std::move(value)](int b, int c) { return *a + b + c; };
    // std::function<int(int, int)> v0(std::move (f)); // -- not compiling
    awt::unique_function<int(int, int)> v(std::move(f));
    EXPECT_EQ(72, v(13, 17));
    auto v2 = std::move(v);
    EXPECT_EQ(62, v2(10, 10));
    EXPECT_THROW(v(5, 5), std::bad_function_call);
  }
}
