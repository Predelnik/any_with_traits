#include "gtest.h"
#include "any_with_traits.h"

#include <array>
#include <map>
#include <memory>
#include <functional>
#include <unordered_set>
#include <algorithm>
#include <sstream>

TEST(any, all) {
  {
    // check for memory leaks
    awt::any<> v(std::vector<int>(1000));
    v = std::vector<int>(1500);
    v.emplace<std::vector<int>>(17, 17);
    EXPECT_EQ(std::vector<int>(17, 17), awt::any_cast<std::vector<int>>(v));
    v.emplace<std::vector<int>>({3, 5, 6});
    EXPECT_EQ(std::vector<int>({3, 5, 6}), awt::any_cast<std::vector<int>>(v));
  }
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
  {
    awt::any<> v1 (17);
    awt::any<> v2 (24);
    using std::swap;
    v1.swap(v2);
    EXPECT_EQ(24, awt::any_cast<int>(v1));
    EXPECT_EQ(17, awt::any_cast<int>(v2));
    swap(v2, v1);
    EXPECT_EQ(17, awt::any_cast<int>(v1));
    EXPECT_EQ(24, awt::any_cast<int>(v2));
  }

  {
    awt::any<> v (17), v1;
    EXPECT_EQ (17, v.value<int> ());
    EXPECT_DEBUG_DEATH (v1.value<int> (), "ptr != nullptr");
    EXPECT_EQ (17, v.value_or<int> (45));
    EXPECT_EQ (45, v1.value_or<int> (45));
    EXPECT_TRUE (v1.empty ());
    EXPECT_FALSE (v.empty ());
  }
  {
    awt::any<any_trait::ostreamable> v (23);
    std::stringstream ss;
    ss << v;
    EXPECT_EQ ("23", ss.str ());
    v = "ASDA";
    ss.str ("");
    ss.clear ();
    ss << v;
    EXPECT_EQ ("ASDA", ss.str ());
  }
}

struct c1 {
  int example_function(int a, int b) { return a + b; }
};

struct c2 {
  int example_function(int x) { return x; }
};

namespace {
int free_function(c1) { return 42; }
int free_function(c2) { return 53; }
}

namespace secret {
int free_function(const c1 &) { return 55; }
int free_function(const c2 &) { return 777; }
}

AWT_DEFINE_MEMBER_FUNCTION_CALL_TRAIT(has_example_function, example_function,
                                      int(int, int));

AWT_DEFINE_FREE_FUNCTION_CALL_TRAIT(has_free_function, free_function,
                                    call_free_function_on_me, int());

AWT_DEFINE_FREE_FUNCTION_CALL_TRAIT(has_secret_free_function,
                                    secret::free_function,
                                    call_secret_free_function_on_me, int(),
                                    const);

TEST(any, custom_traits) {
  {
    awt::any<any_trait::has_example_function> v;
    EXPECT_THROW(v.example_function(5, 5), std::bad_function_call);
    v = c1{};
    EXPECT_EQ(10, v.example_function(3, 7));
    // v = c2 {}; fails to compile
    // v = 25; fails to compile
  }
  {
    awt::any<any_trait::has_free_function> v;
    EXPECT_THROW(v.call_free_function_on_me(), std::bad_function_call);
    v = c1{};
    EXPECT_EQ(42, v.call_free_function_on_me());
    v = c2{};
    EXPECT_EQ(53, v.call_free_function_on_me());
  }
  {
    awt::any<any_trait::has_secret_free_function> v;
    EXPECT_THROW(v.call_secret_free_function_on_me(), std::bad_function_call);
    v = c1{};
    EXPECT_EQ(55, v.call_secret_free_function_on_me());
    v = c2{};
    EXPECT_EQ(777, v.call_secret_free_function_on_me());
  }
}
