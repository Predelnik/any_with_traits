#pragma once

#include <type_traits>

/* Template machinery - TODO Separate in a file */
#define STATIC_FOR_EACH(...) std::initializer_list<int > {(__VA_ARGS__, 0)...}
#define ENABLE_BY_VALUE(VALUE) template <typename Res = std::integral_constant<bool, VALUE>, std::enable_if_t<Res::value, int> = 0>
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
}

namespace any_trait {
  struct copiable {};
  struct movable {};
  template <typename Signature>
  struct callable {};
  struct destructible {};
};

template <class Trait>
struct trait_impl { static_assert (std::is_same<Trait, void>::value, "Trait functions undefined"); };

template <>
struct trait_impl<any_trait::destructible> {
  struct func_impl
  {
    using signature = void(*)(void *);
    template <typename T>
    static void func(void *other) {
      return ::delete (static_cast<T *> (other));
    }
    template <typename T>
    void store() {
      call_destroy = &func<T>;
    }
    signature call_destroy;
  };

  template <class RealType>
  struct any_base {
    ~any_base() {
      auto real_this = static_cast<RealType *> (this);
      if (real_this->f_table)
        static_cast<func_impl *> (real_this->f_table)->call_destroy(real_this->data);
    }
  };
};

template <typename Ret, typename... ArgTypes>
struct trait_impl<any_trait::callable<Ret (ArgTypes...)>> {
  struct func_impl
  {
    using signature = Ret (*)(const void *, ArgTypes...);
    template <typename T>
    static Ret func(const void *object, ArgTypes... args) {
      return (*static_cast<const T *> (object))(args...);
    }
    template <typename T>
    void store() {
      call_call = &func<T>;
    }
    signature call_call;
  };

  template <class RealType>
  struct any_base {
    Ret operator ()(ArgTypes&&... args) const {
      auto real_this = static_cast<const RealType *> (this);
      return (real_this->f_table)->call_call(real_this->data, std::forward<ArgTypes>(args)...);
    }
  };
};

template <>
struct trait_impl<any_trait::copiable> {
  struct func_impl
  {
    using signature = void *(*)(const void *);
    template <typename T>
    static void *func(const void *other) {
      return new T(*static_cast<const T *> (other));
    }
    template <typename T>
    void store() {
      call_copy = &func<T>;
    }
    signature call_copy;
  };

  template <class RealType>
  struct any_base {
    void clone(const RealType &other) {
      auto real_this = static_cast<RealType *> (this);
      if (real_this->f_table)
        static_cast<trait_impl<any_trait::destructible>::func_impl *> (real_this->f_table)->call_destroy(real_this->data); // Crude way to call destructor
      real_this->f_table = other.f_table;
      real_this->data = static_cast<func_impl *> (real_this->f_table)->call_copy(&other.data);
    }
  };
};

template <>
struct trait_impl<any_trait::movable> {
  struct func_impl
  {
    using signature = void *(*)(const void *);
    template <typename T>
    static void *func(const void *other) {
      return new T(*static_cast<const T *> (other));
    }
    template <typename T>
    void store() {
    }
  };

  template <class RealType>
  struct any_base {
    void move_from(RealType &&other) {
      auto real_this = static_cast<RealType *> (this);
      real_this->f_table = other.f_table;
      real_this->data = other.data;
      other.f_table = nullptr;
    }
  };
};

namespace detail
{
  // Should this be amalgamated or separated?
  template <class... Traits>
  struct func_table : trait_impl<Traits>::func_impl... {
    template<class Trait>
    typename trait_impl<Trait>::func_impl *as_base() { return static_cast<typename trait_impl<Trait>::func_impl *> (this); }

    template <typename T>
    void store () {
      STATIC_FOR_EACH(this->as_base<Traits>()->template store<T> ());
    }
  };

  template <typename T, class... Traits>
  struct func_table_instance_impl {
    func_table<Traits...> value;
    func_table_instance_impl() {
      value.template store<T>();
    }
  };

  template <typename T, class... Traits>
  struct func_table_instance
  {
    static func_table_instance_impl<T, Traits...> value;
  };

  template <typename T, class... Traits>
  func_table_instance_impl<T, Traits...> func_table_instance<T, Traits...>::value;
}

template <class... Traits>
class any_t : public trait_impl<Traits>::template any_base<any_t<Traits...>>... {
  using self = any_t;
  constexpr static bool is_copiable = tmp::one_of<any_trait::copiable, Traits...>::value;
  constexpr static bool is_movable = tmp::one_of<any_trait::movable, Traits...>::value;
public:
  any_t() {}
  // TODO: check that type isn't any itself
  template <typename Type, std::enable_if_t<!std::is_same<std::decay_t<Type>, self>::value, int> = 0>
  any_t (Type &&value) {
    using decayed_type = std::decay_t<Type>;
    f_table = &detail::func_table_instance<decayed_type, Traits...>::value.value;
    data = new decayed_type (std::forward<Type>(value));
  }

  any_t(const self &other) { static_assert (is_copiable, "class copy construction is prohibited due to lack of copiable trait");  this->clone(other); }
  self &operator=(const self &other) { static_assert (is_copiable, "class copy assignment is prohibited due to lack of copiable trait"); this->clone(other); return *this; }
  any_t(self &&other) { static_assert (is_movable, "class move construction is prohibited due to lack of movable trait");  this->move_from(std::move (other)); }
  self &operator=(self &&other) { static_assert (is_movable, "class move assignment is prohibited due to lack of movable trait"); this->move_from(std::move (other)); return *this; }

private:
  detail::func_table<Traits...> *f_table = nullptr;
  void *data = nullptr; // for now only large


  template <class T>
  friend struct trait_impl;
};
