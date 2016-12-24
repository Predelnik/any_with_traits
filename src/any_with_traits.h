#pragma once

#include "overload.h"

#include <type_traits>
#include <typeindex>

/* Template machinery - TODO Separate in a file */
#define STATIC_FOR_EACH(...) std::initializer_list<int > {(__VA_ARGS__, 0)...}
#define CPP14_CONSTEXPR constexpr
namespace tmp
{
  template<bool... vals> struct bool_pack {};
  template<bool A, bool B> struct second_bool : std::integral_constant<bool, B> {};
  template<typename A, typename B>
  struct is_not_same : std::true_type {};
  template<typename A>
  struct is_not_same<A, A> : std::false_type {};
  template<bool... Values>
  struct static_or : is_not_same<bool_pack<Values...>, bool_pack<(second_bool<Values, false>::value)...>> {};
  template<class Needle, class... Haystack>
  struct one_of : static_or<std::is_same<Needle, Haystack>::value...> {};
  template <typename T>
  struct type_t { using type = T; };
}

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename T>
struct forbid_implicit_casts
{
  forbid_implicit_casts(T value_arg) : value(value_arg) {}
  T value;
};

namespace any_trait {
  struct destructible {}; // TODO: enable by default
  struct copiable {};
  struct movable {};
  struct comparable {};
  struct orderable {};
  struct hashable {};
  template <typename Signature>
  struct callable {};
};

namespace detail {
  enum class any_stored_value_type : char
  {
    large,
    small,
  };
  constexpr auto any_small_size = 24; // possibly it's cooler to allow to specify it differently for different types of anys
  struct any_all_types {};
 
  template <typename T>
  using get_any_stored_value_type = std::integral_constant<any_stored_value_type, sizeof(T) <= any_small_size ? any_stored_value_type::small : any_stored_value_type::large>;
}

template <class Trait>
struct trait_impl { static_assert (std::is_same<Trait, void>::value, "Trait is not implemented"); };

/* BEGIN any_trait::destructible implementation */
template <>
struct trait_impl<any_trait::destructible> {
  template <detail::any_stored_value_type>
  struct func_impl;
  using dtor_signature = void(*)(void *);

  template <class RealType>
  struct any_base;
};

template <>
struct trait_impl<any_trait::destructible>::func_impl<detail::any_stored_value_type::small>
{
  dtor_signature call_dtor = nullptr;

  template <typename T>
  static void placement_dtor(void *other) {
    return (static_cast<T *> (other))->~T();
  }

  template <typename T>
  constexpr func_impl(tmp::type_t<T>) : call_dtor(&placement_dtor<T>) {
  }
};

template <>
struct trait_impl<any_trait::destructible>::func_impl<detail::any_stored_value_type::large>
{
  dtor_signature call_dtor = nullptr;

  template <typename T>
  static void dtor(void *other) {
    return ::delete (static_cast<T *> (other));
  }

  template <typename T>
  constexpr func_impl(tmp::type_t<T>) : call_dtor(&dtor<T>) {
  }
};

template <class RealType>
struct trait_impl<any_trait::destructible>::any_base {
  ~any_base() {
    auto real_this = static_cast<RealType *> (this);
    if (!real_this->has_value())
      return;
    real_this->template visit_ftable<func_impl>([&](auto f_table) { f_table->call_dtor(real_this->data_ptr());  });
    real_this->d.type_data.t_info = nullptr;
  }
};
/* END any_trait::destructible implementation */

/* BEGIN any_trait::copiable implementation */
template <>
struct trait_impl<any_trait::copiable> {
  template <detail::any_stored_value_type>
  struct func_impl;

  template <class RealType>
  struct any_base;
};

template <>
struct trait_impl<any_trait::copiable>::func_impl<detail::any_stored_value_type::large>
{
  using clone_signature = void *(*)(const void *);
  clone_signature call_clone;
  template <typename T>
  static void *clone(const void *other) {
    return ::new T(*static_cast<const T *> (other));
  }

  template<typename T>
  constexpr func_impl(tmp::type_t<T>) : call_clone(&clone<T>) {}
};

template <>
struct trait_impl<any_trait::copiable>::func_impl<detail::any_stored_value_type::small>
{
  using copy_signature = void(*)(void *, const void *);
  copy_signature call_copy;

  template <typename T>
  static void copy(void *target, const void *source) {
    new (target) T(*static_cast<const T *> (source));
  }

  template <typename T>
  constexpr func_impl(tmp::type_t<T>) : call_copy(&copy<T>) {
  }
};

template <class RealType>
struct trait_impl<any_trait::copiable>::any_base {
  void clone(const RealType &other) {
    auto real_this = static_cast<RealType *> (this);
    real_this->~RealType();
    real_this->d.type_data = other.d.type_data;
    if (!real_this->has_value())
      return;

    real_this->template visit_ftable<func_impl>(sftb::overload(
      [&](const func_impl<detail::any_stored_value_type::small> *f_table) { f_table->call_copy(real_this->d.small_data, other.d.small_data); },
      [&](const func_impl<detail::any_stored_value_type::large> *f_table) { real_this->d.data = f_table->call_clone(other.d.data); }));
  }
};
/* END any_trait::copiable implementation */

/* BEGIN any_trait::movable implementation */
template <>
struct trait_impl<any_trait::movable> {
  template <detail::any_stored_value_type>
  struct func_impl;

  template <class RealType>
  struct any_base;
};

template <>
struct trait_impl<any_trait::movable>::func_impl <detail::any_stored_value_type::large>
{
  template <typename T>
  constexpr func_impl(tmp::type_t<T>) {}
};

template <>
struct trait_impl<any_trait::movable>::func_impl<detail::any_stored_value_type::small>
{

  using move_signature = void(*)(void *, void *);
  template <typename T>
  static void do_move(void *target, void *source) {
    new(target) T(std::move(*static_cast<T *> (source)));
  }
  template <typename T>
  constexpr func_impl(tmp::type_t<T>) : call_move(&do_move<T>) {
  }
  move_signature call_move = nullptr;
};

template <class RealType>
struct trait_impl<any_trait::movable>::any_base {
  void move_from(RealType &&other) {
    auto real_this = static_cast<RealType *> (this);
    real_this->~RealType();
    real_this->d.type_data = other.d.type_data;
    real_this->template visit_ftable<func_impl>(sftb::overload(
      [&](const func_impl<detail::any_stored_value_type::small> *f_table) { f_table->call_move(real_this->data_ptr(), other.data_ptr()); },
      [&](const func_impl<detail::any_stored_value_type::large> *) { real_this->d.data = other.d.data; other.d.type_data.clear(); }));
  }
};
/* END any_trait::movable implementation */

/* BEGIN any_trait::comparable implementation */
template <>
struct trait_impl<any_trait::comparable> {
  template <detail::any_stored_value_type>
  struct func_impl
  {
    using equal_to_signature = bool(*)(const void *, const void *);
    template <typename T>
    static bool equal_to(const void *first, const void *second) {
      return *static_cast<const T *> (first) == *static_cast<const T *> (second);
    }

    equal_to_signature call_equal_to = nullptr;

    template <typename T>
    constexpr func_impl(tmp::type_t<T>) : call_equal_to(&equal_to<T>)
    {}
  };

  template <class RealType>
  struct any_base {

    bool operator==(const RealType &other) const {
      auto real_this = static_cast<const RealType *> (this);
      if (!real_this->has_value() || !other.has_value())
        return real_this->has_value() == other.has_value();
      return real_this->type() == other.type() && real_this->template visit_ftable<func_impl>([&](auto f_table)
      { return f_table->call_equal_to(real_this->data_ptr(), other.data_ptr());  });
    }

    friend bool operator==(const RealType &first, const RealType & second) {
      return first.operator==(second);
    }

    friend bool operator==(const RealType &first, forbid_implicit_casts<const RealType &> second) {
      return second.value.operator== (first);
    }

    bool operator!=(const RealType &other) const {
      auto real_this = static_cast<const RealType *> (this);
      return !(*real_this == other);
    }

    friend bool operator!=(const RealType &first, forbid_implicit_casts<const RealType &> second) {
      return second.value.operator!= (first);
    }

    friend bool operator!=(const RealType &first, const RealType & second) {
      return first.operator!=(second);
    }
  };
};
/* END any_trait::comparable implementation */

/* BEGIN any_trait::orderable implementation */
template <>
struct trait_impl<any_trait::orderable> {
  template <detail::any_stored_value_type>
  struct func_impl
  {
    using less_than_signature = bool(*)(const void *, const void *);
    template <typename T>
    static bool less_than(const void *first, const void *second) {
      return *static_cast<const T *> (first) < *static_cast<const T *> (second);
    }

    less_than_signature call_less_than = nullptr;

    template <typename T>
    constexpr func_impl(tmp::type_t<T>) : call_less_than(&less_than<T>) {}
  };

  template <class RealType>
  struct any_base {

    bool operator<(const RealType &other) const {
      auto real_this = static_cast<const RealType *> (this);
      if (!real_this->has_value() || !other.has_value())
        return real_this->has_value() < other.has_value();
      if (real_this->type() != other.type())
        return std::type_index(real_this->type()) < std::type_index(other.type());

      return real_this->template visit_ftable<func_impl>([&](auto f_table)
        { return f_table->call_less_than(real_this->data_ptr(), other.data_ptr());  });
    }

    friend bool operator<(const RealType &first, const RealType & second) { return first.operator<(second); }
    friend bool operator<(const RealType &first, forbid_implicit_casts<const RealType &> second) {
      return first.operator<(second);
    }
    bool operator>(const RealType &other) const { 
      auto real_this = static_cast<const RealType *> (this);
      return other.operator< (*real_this); }
    friend bool operator>(const RealType &first, const RealType & second) {
      return first.operator>(second);
    }
    friend bool operator>(const RealType &first, forbid_implicit_casts<const RealType &> second) {
      return first.operator>(second);
    }
    bool operator>=(const RealType &other) const {
      auto real_this = static_cast<const RealType *> (this);
      return !(*real_this < other); }
    friend bool operator>=(const RealType &first, const RealType & second) {
      return first.operator>=(second);
    }
    friend bool operator>=(const RealType &first, forbid_implicit_casts<const RealType &> second) {
      return first.operator>=(second);
    }
    bool operator<=(const RealType &other) const { 
      auto real_this = static_cast<const RealType *> (this);
      return !(*real_this > other); }
    friend bool operator<=(const RealType &first, const RealType & second) {
      return first.operator<=(second);
    }
    friend bool operator<=(const RealType &first, forbid_implicit_casts<const RealType &> second) {
      return first.operator<=(second);
    }
  };
};
/* END any_trait::orderable implementation */

/* BEGIN any_trait::hashable implementation */
template <>
struct trait_impl<any_trait::hashable> {
  template <detail::any_stored_value_type>
  struct func_impl
  {
    using hash_signature = std::size_t(*)(const void *);
    template <typename T>
    static std::size_t hash_func(const void *value) {
      return std::hash<T>()(*static_cast<const T *> (value));
    }

    hash_signature call_hash = nullptr;

    template <typename T>
    constexpr func_impl(tmp::type_t<T>) : call_hash(&hash_func<T>)
    {}
  };

  template <class RealType>
  struct any_base {
    std::size_t hash() const
    {
      auto real_this = static_cast<const RealType *> (this);
      if (!real_this->has_value())
        return 7927u; // hash for empty any
      std::size_t res = real_this->template visit_ftable<func_impl>([&](auto f_table) { return f_table->call_hash(real_this->data_ptr());  });
      hash_combine(res, std::type_index(*real_this->d.type_data.t_info)); // adding type_info hash to original type hash
      return res;
    };
  };
};
/* END any_trait::hashable implementation */

/* BEGIN any_trait::callable implementation */
template <typename Ret, typename... ArgTypes>
struct trait_impl<any_trait::callable<Ret (ArgTypes...)>> {
  template <detail::any_stored_value_type>
  struct func_impl
  {
    using signature = Ret (*)(const void *, ArgTypes...);
    template <typename T>
    static Ret func(const void *object, ArgTypes... args) {
      return (*static_cast<const T *> (object))(args...);
    }
    signature call_call = nullptr;
    template <typename T>
    constexpr func_impl(tmp::type_t<T>)
    {
      call_call = &func<T>;
    }
  };

  template <class RealType>
  struct any_base {
    Ret operator ()(ArgTypes&&... args) const {
      auto real_this = static_cast<const RealType *> (this);
      if (!real_this->has_value ())
        throw std::bad_function_call{};
      return real_this->template visit_ftable<func_impl>([&](auto f_table) { return f_table->call_call(real_this->data_ptr(), std::forward<ArgTypes>(args)...);  });
    }
  };
};
/* END any_trait::callable implementation */

namespace detail // TODO: move to any related namespace
{
  template <detail::any_stored_value_type value_type, class... Traits>
  struct func_table : trait_impl<Traits>::template func_impl<value_type>... {

  template <typename T>
  constexpr func_table(tmp::type_t<T> t) : trait_impl<Traits>::template func_impl<value_type> (t)... {}
  };

  template <typename T, detail::any_stored_value_type value_type, class... Traits>
  struct func_table_instance {
    static constexpr func_table<value_type, Traits...> value = { tmp::type_t<T>() };
  };

  template <typename T, detail::any_stored_value_type value_type, class... Traits>
  constexpr func_table<value_type, Traits...> func_table_instance<T, value_type, Traits...>::value;
}

template <class... Traits>
class any_t : public trait_impl<Traits>::template any_base<any_t<Traits...>>... {
  using self = any_t;
  constexpr static bool is_copiable = tmp::one_of<any_trait::copiable, Traits...>::value;
  constexpr static bool is_movable = tmp::one_of<any_trait::movable, Traits...>::value;
public:
  any_t() {}
  template <typename Type, std::enable_if_t<!std::is_same<std::decay_t<Type>, self>::value, int> = 0>
  any_t(Type &&value) {
    *this = std::forward<Type>(value);
  }

  template <typename Type, std::enable_if_t<!std::is_same<std::decay_t<Type>, self>::value, int> = 0>
  self &operator=(Type &&value) {
    using decayed_type = std::decay_t<Type>;
    static_assert (!std::is_base_of<decayed_type, self>::value, "Possible error in traits implementation");
    d.type_data.t_info = &typeid (decayed_type);
    using t = detail::get_any_stored_value_type<decayed_type>;
    d.type_data.stored_value_type = t::value;
    dispatch_and_fill(t(), std::forward<Type>(value));
    return *this;
  }

  template <typename Type>
  void dispatch_and_fill(std::integral_constant<detail::any_stored_value_type, detail::any_stored_value_type::small> c, Type &&value) {
    using decayed_type = std::decay_t<Type>;
    d.type_data.small_f_table = &detail::func_table_instance<decayed_type, detail::any_stored_value_type::small, Traits...>::value;
    new (d.small_data) decayed_type (std::forward<Type>(value));
  }

  template <typename Type>
  void dispatch_and_fill(std::integral_constant<detail::any_stored_value_type, detail::any_stored_value_type::large> c, Type &&value) {
    using decayed_type = std::decay_t<Type>;
    d.type_data.large_f_table = &detail::func_table_instance<decayed_type, detail::any_stored_value_type::large, Traits...>::value;
    d.data = new decayed_type(std::forward<Type>(value));

  }

  any_t(const self &other) { static_assert (is_copiable, "class copy construction is prohibited due to lack of copiable trait");  this->clone(other); }
  self &operator=(const self &other) { static_assert (is_copiable, "class copy assignment is prohibited due to lack of copiable trait"); this->clone(other); return *this; }
  any_t(self &&other) { static_assert (is_movable, "class move construction is prohibited due to lack of movable trait");  this->move_from(std::move(other)); }
  self &operator=(self &&other) { static_assert (is_movable, "class move assignment is prohibited due to lack of movable trait"); this->move_from(std::move(other)); return *this; }
  const type_info &type() const { return *d.type_data.t_info; }
  bool has_value() const { return d.type_data.t_info; }
  void reset() { this->~self(); }

private:
  template <typename Type>
  Type *cast() {
    if (*d.type_data.t_info == typeid (std::remove_const_t<Type>))
      return static_cast<Type *> (data_ptr ());

    return nullptr;
  }

  void *data_ptr () {
    switch (d.type_data.stored_value_type) {
    case detail::any_stored_value_type::large: return d.data;
    case detail::any_stored_value_type::small: return &d.small_data;
    }
    return nullptr;
  }

  const void *data_ptr() const {
    return (const_cast<self *> (this))->data_ptr();
  }

  template <template <detail::any_stored_value_type> class BaseType, class VisitorType>
  auto visit_ftable(const VisitorType &visitor) const
  {
    switch (d.type_data.stored_value_type)
    {
    case detail::any_stored_value_type::small:
      return visitor(static_cast<const BaseType<detail::any_stored_value_type::small> *> (d.type_data.small_f_table));
      break;
    case detail::any_stored_value_type::large:
      return visitor(static_cast<const BaseType<detail::any_stored_value_type::large> *> (d.type_data.large_f_table));
      break;
    }
    throw 0; // Shoud not be called
  }

private:
  struct {
    struct
    {
      union
      {
        const detail::func_table<detail::any_stored_value_type::small, Traits...> *small_f_table = nullptr;
        const detail::func_table<detail::any_stored_value_type::large, Traits...> *large_f_table;
      };
      const std::type_info *t_info = nullptr;
      detail::any_stored_value_type stored_value_type = detail::any_stored_value_type::large;
      void clear () {
        small_f_table = nullptr;
        t_info = nullptr;
        stored_value_type = detail::any_stored_value_type::large; // shouldn't matter
      }
    } type_data;
    union
    {
      void *data = nullptr;
      char small_data[detail::any_small_size];
    };
  } d;

  template <class T>
  friend struct trait_impl;
  template <typename Type, typename... Traits>
  friend Type *any_cast(any_t<Traits...> *value);
  template <typename Type, typename... Traits>
  friend const Type *any_cast(const any_t<Traits...> *value);
};

namespace std
{
  template <class... Traits>
  struct hash<any_t<Traits...>>
  {
  private:
    using wrapped_type = any_t<Traits...>;
  public:
    size_t operator ()(const wrapped_type &value) const { return value.hash(); }
  };
}

template <typename Type, typename... Traits>
Type *any_cast (any_t<Traits...> *value) {
  if (value)
   return value->template cast<Type> ();

  return nullptr;
}

template <typename Type, typename... Traits>
const Type *any_cast(const any_t<Traits...> *value) {
  if (value)
    return value->template cast<Type>();

  return nullptr;
}

template <typename Type, typename... Traits>
Type &any_cast(any_t<Traits...> &value) {
  auto ptr = any_cast<Type> (&value);
  if (ptr)
    return *ptr;

  throw std::bad_cast {}; // technically should be bad_any_cast
}

template <typename Type, typename... Traits>
const Type &any_cast(const any_t<Traits...> &value) {
  auto ptr = any_cast<Type> (&value);
  if (ptr)
    return *ptr;

  throw std::bad_cast {}; // technically should be bad_any_cast
}

