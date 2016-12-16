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
template <typename T>
struct forbid_implicit_casts
{
  forbid_implicit_casts(T value_arg) : value(value_arg) {}
  T value;
};

namespace any_trait {
  struct copiable {};
  struct movable {};
  template <typename Signature>
  struct callable {};
  struct destructible {};
  struct comparable {};
};

namespace detail {
  enum class any_stored_value_type : char
  {
    large,
    small,
  };
  constexpr auto any_small_size = 24; // possibly it's cooler to allow to specify it differently for different types of anys
  template <typename T>
  using get_any_stored_value_type = std::integral_constant<any_stored_value_type, sizeof(T) <= any_small_size ? any_stored_value_type::small : any_stored_value_type::large>;
}

template <class Trait>
struct trait_impl { static_assert (std::is_same<Trait, void>::value, "Trait functions undefined"); };

template <>
struct trait_impl<any_trait::destructible> {
  struct func_impl
  {
    using dtor_signature = void(*)(void *);
    template <typename T>
    static void dtor(void *other) {
      return ::delete (static_cast<T *> (other));
    }


     dtor_signature call_dtor;

    template <typename T>
    void store_impl(std::integral_constant < detail::any_stored_value_type, detail::any_stored_value_type::large > ) {
      call_dtor = &dtor<T>;
    }

    template <typename T>
    static void placement_dtor(void *other) {
      return (static_cast<T *> (other))->~T ();
    }
    template <typename T>
    void store_impl(std::integral_constant<detail::any_stored_value_type, detail::any_stored_value_type::small>) {
      call_dtor = &placement_dtor<T>;
    }

    template <typename T>
    void store() {
      store_impl<T>(detail::get_any_stored_value_type<T> ());
    }
  };

  template <class RealType>
  struct any_base {
    ~any_base() {
      auto real_this = static_cast<RealType *> (this);
      if (real_this->d.type_data.f_table)
        static_cast<func_impl *> (real_this->d.type_data.f_table)->call_dtor(real_this->data_ptr());
    }
  };
};

template <>
struct trait_impl<any_trait::comparable> {
  struct func_impl
  {
    using equal_to_signature = bool(*)(const void *, const void *);
    template <typename T>
    static bool equal_to(const void *first, const void *second) {
      return *static_cast<const T *> (first) == *static_cast<const T *> (second);
    }

    equal_to_signature call_equal_to;

    template <typename T>
    void store() {
      call_equal_to = &equal_to<T>;
    }
  };

  template <class RealType>
  struct any_base {

    bool operator==(const RealType &other) const {
      auto real_this = static_cast<const RealType *> (this);
      return real_this->type() == other.type() && static_cast<func_impl *> (real_this->d.type_data.f_table)->call_equal_to(real_this->data_ptr(), other.data_ptr ());
    }

    friend bool operator==(const RealType &first, const RealType & second) {
      return first.operator==(second);
    }

    friend bool operator==(const RealType &first, forbid_implicit_casts<const RealType &> second) {
      return second.value.operator== (first);
    }

    bool operator!=(const RealType &other) const {
      return !(*this == other);
    }

    friend bool operator!=(const RealType &first, forbid_implicit_casts<const RealType &> second) {
      return second.value.operator!= (first);
    }

    friend bool operator!=(const RealType &first, const RealType & second) {
      return first.operator!=(second);
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
      return (real_this->d.type_data.f_table)->call_call(real_this->data_ptr (), std::forward<ArgTypes>(args)...);
    }
  };
};

template <>
struct trait_impl<any_trait::copiable> {
  struct func_impl
  {
    using clone_signature = void *(*)(const void *);
    using copy_signature = void (*)(void *, const void *);
    template <typename T>
    static void *clone(const void *other) {
      return ::new T(*static_cast<const T *> (other));
    }
    union
    {
      clone_signature call_clone;
      copy_signature call_copy;
    };

    template <typename T>
    void store_impl(std::integral_constant < detail::any_stored_value_type, detail::any_stored_value_type::large >) {
      call_clone = &clone<T>;
    }

    template <typename T>
    static void copy(void *target, const void *source) {
      new (target) T(*static_cast<const T *> (source));
    }

    template <typename T>
    void store_impl(std::integral_constant < detail::any_stored_value_type, detail::any_stored_value_type::small >) {
      call_copy = &copy<T>;
    }

    template <typename T>
    void store() {
      store_impl<T>(detail::get_any_stored_value_type<T> ());
    }
  };

  template <class RealType>
  struct any_base {
    void clone(const RealType &other) {
      auto real_this = static_cast<RealType *> (this);
      real_this->~RealType();
      real_this->d.type_data = other.d.type_data;
      switch (other.d.type_data.stored_value_type) {
        case detail::any_stored_value_type::large:
          real_this->d.data = static_cast<func_impl *> (real_this->d.type_data.f_table)->call_clone(other.d.data);
          break;
        case detail::any_stored_value_type::small :
          real_this->d.type_data.f_table->call_copy(real_this->d.small_data, other.d.small_data);
          break;
      }

    }
  };
};

template <>
struct trait_impl<any_trait::movable> {
  struct func_impl
  {
    using move_signature = void (*)(void *, void *);
    template <typename T>
    static void do_move(void *target ,void *source) {
      new(target) T (std::move (*static_cast<T *> (source)));
    }
    template <typename T>
    void store_impl(std::integral_constant < detail::any_stored_value_type, detail::any_stored_value_type::large >) {
    }
    template <typename T>
    void store_impl(std::integral_constant < detail::any_stored_value_type, detail::any_stored_value_type::small >) {
      call_move = &do_move<T>;
    }
    template <typename T>
    void store() {
      return store_impl<T>(detail::get_any_stored_value_type<T> ());
    }

    move_signature call_move;
  };

  template <class RealType>
  struct any_base {
    void move_from(RealType &&other) {
      auto real_this = static_cast<RealType *> (this);
      real_this->~RealType();
      switch (other.d.type_data.stored_value_type) {
      case detail::any_stored_value_type::large:
        real_this->d = other.d;
        other.d.type_data.clear();
        break;
      case detail::any_stored_value_type::small:
        real_this->d.type_data = other.d.type_data;
        real_this->d.type_data.f_table->call_move(real_this->data_ptr (), other.data_ptr ()); // data in other comes to state `moved from` and could be safely destroyed
        break;
      }
    }
  };
};

namespace detail // TODO: move to any related namespace
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
  any_t(Type &&value) {
    *this = std::forward<Type>(value);
  }

  template <typename Type, std::enable_if_t<!std::is_same<std::decay_t<Type>, self>::value, int> = 0>
  self &operator=(Type &&value) {
    using decayed_type = std::decay_t<Type>;
    d.type_data.t_info = &typeid (decayed_type);
    d.type_data.f_table = &detail::func_table_instance<decayed_type, Traits...>::value.value;
    using t = detail::get_any_stored_value_type<decayed_type>;
    d.type_data.stored_value_type = t::value;
    dispatch_and_fill(t(), std::forward<Type>(value));
    return *this;
  }

  template <typename Type>
  void dispatch_and_fill(std::integral_constant<detail::any_stored_value_type, detail::any_stored_value_type::small> c, Type &&value) {
    using decayed_type = std::decay_t<Type>;
    new (d.small_data) decayed_type (std::forward<Type>(value));
  }

  template <typename Type>
  void dispatch_and_fill(std::integral_constant<detail::any_stored_value_type, detail::any_stored_value_type::large> c, Type &&value) {
    using decayed_type = std::decay_t<Type>;
    d.data = new decayed_type(std::forward<Type>(value));

  }

  any_t(const self &other) { static_assert (is_copiable, "class copy construction is prohibited due to lack of copiable trait");  this->clone(other); }
  self &operator=(const self &other) { static_assert (is_copiable, "class copy assignment is prohibited due to lack of copiable trait"); this->clone(other); return *this; }
  any_t(self &&other) { static_assert (is_movable, "class move construction is prohibited due to lack of movable trait");  this->move_from(std::move(other)); }
  self &operator=(self &&other) { static_assert (is_movable, "class move assignment is prohibited due to lack of movable trait"); this->move_from(std::move(other)); return *this; }
  const type_info &type() const { return *d.type_data.t_info; }

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

private:
  struct {
    struct
    {
      detail::func_table<Traits...> *f_table = nullptr;
      const std::type_info *t_info = nullptr;
      detail::any_stored_value_type stored_value_type;
      void clear () {
        f_table = nullptr;
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

template <typename Type, typename... Traits>
Type *any_cast (any_t<Traits...> *value) {
  if (value)
   return value->cast<Type> ();

  return nullptr;
}

template <typename Type, typename... Traits>
const Type *any_cast(const any_t<Traits...> *value) {
  if (value)
    return value->cast<Type>();

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

