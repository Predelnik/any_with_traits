#include "gtest.h"
#include "any_with_traits.h"

#include <array>
#include <map>
#include <memory>
#include <functional>
#include <unordered_set>

TEST(any, all)
{
  {
    any_t<any_trait::destructible> v(123);
    EXPECT_TRUE(v.type() == typeid (int));
    EXPECT_EQ(123, *any_cast<int> (&v));
    EXPECT_EQ(nullptr, any_cast<double> (&v));
    EXPECT_EQ(123, any_cast<int> (v));
    EXPECT_THROW(any_cast<double> (v), std::bad_cast);
  }
  {
    any_t < any_trait::destructible, any_trait::copiable, any_trait::movable> v(123);
    EXPECT_EQ(123, any_cast<int> (v));
    auto v2 = v;
    EXPECT_EQ(123, any_cast<int> (v2));
    auto v3 = std::move(v);
    EXPECT_EQ(123, any_cast<int> (v3));
    v3 = {};
    EXPECT_FALSE(v3.has_value ());
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
    v3.reset ();
    EXPECT_FALSE(v3.has_value());
    v2 = v3;
    EXPECT_FALSE(v2.has_value());
    v = std::move(v3);
    EXPECT_FALSE(v.has_value());
  }
  {
    using any = any_t <any_trait::destructible, any_trait::callable<int(int, int)>>;
    any v(std::plus<int> {});
    EXPECT_EQ (17, v (12, 5));
    v = std::minus<int> {};
    EXPECT_EQ(7, v(12, 5));
    any p;
    EXPECT_THROW(p(12, 5), std::bad_function_call);
  }
  {
    using any = any_t <any_trait::destructible, any_trait::comparable, any_trait::copiable>;
    any v(13);
    auto y = v;
    EXPECT_EQ(y, v);
    EXPECT_TRUE(13 == v);
    EXPECT_TRUE(v == 13);
    y = std::string ("Hello");
    EXPECT_EQ(std::string("Hello"), y);
    EXPECT_NE(13, y);
    EXPECT_TRUE(13 != y);
    EXPECT_TRUE(y != 13);
    any empty;
    EXPECT_NE (empty, y);
    y = empty;
    EXPECT_EQ(y ,empty);
  }
  {
    using any = any_t<any_trait::destructible, any_trait::orderable, any_trait::movable, any_trait::copiable> ;
    std::vector<any> v;
    v.emplace_back(25);
    v.emplace_back(-15);
    v.emplace_back(3);
    std::sort(v.begin(), v.end());
    EXPECT_EQ(25, any_cast<int> (v.back()));
    EXPECT_EQ(-15, any_cast<int> (v.front()));
    std::map<any, int> m;
    m[13] = 27;
    m[std::string ("acdcd")] = 34;
    m[std::vector<int> {1, 5, 3}] = 666;
    EXPECT_EQ(3u, m.size());
    EXPECT_EQ(666, m[std::vector<int> ({1, 5, 3})]);
    EXPECT_LT(any{}, m[13]);
    EXPECT_FALSE(any{} < any{});
    EXPECT_FALSE(any{} > any{});
  }
  {
    using any = any_t<any_trait::destructible, any_trait::movable, any_trait::copiable, any_trait::hashable, any_trait::comparable>;
    std::unordered_set<any> vv;
    vv.insert(17);
    vv.insert(13);
    vv.insert(19);
    vv.insert(std::string("13"));
    vv.insert({}); // empty any is fine for hashing
    EXPECT_EQ(1, vv.count (std::string ("13")));
    EXPECT_EQ(0, vv.count(std::string("27")));
    EXPECT_EQ(1, vv.count({}));
  }
}
