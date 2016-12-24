#pragma once

#include <functional>
#include <type_traits>
#include <typeinfo>
#include <typeindex>

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

namespace awt
{
  namespace detail
  {
    template <class... Traits> class any_t;
  } // namespace detail
  template <class... Traits>
  using any = detail::any_t <any_trait::destructible, Traits...>;
  using normal_any = any<any_trait::copiable, any_trait::movable>;
  template <typename Signature>
  using function = any<any_trait::copiable, any_trait::movable, any_trait::callable<Signature>>;
  template <typename Signature>
  using unique_function = any<any_trait::movable, any_trait::callable<Signature>>;

  template <typename Type, typename... Traits>
  Type *any_cast(any<Traits...> *value);
  template <typename Type, typename... Traits>
  const Type *any_cast(const any<Traits...> *value);

  namespace detail
  {
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

    template <typename... Args> class overload_class;

    template <typename Arg>
    class overload_class<Arg> : Arg
    {
      using self = overload_class<Arg>;
    public:
      using Arg::operator ();
      template <class ActualArg, std::enable_if_t<!std::is_same<std::decay_t<ActualArg>, self>::value, int> = 0>
      overload_class(const ActualArg &arg) : Arg(arg) {}
    };

    template <typename Arg0, typename... Args>
    class overload_class<Arg0, Args...> : Arg0, overload_class<Args...>
    {
      using self = overload_class<Arg0, Args...>;
      using parent_t = overload_class<Args...>;

    public:
      using Arg0::operator();
      using parent_t::operator ();

      template <class ActualArg0, class... ActualArgs, std::enable_if_t<!std::is_same<std::decay_t<ActualArg0>, self>::value, int> = 0>
      overload_class(const ActualArg0 &arg0, const ActualArgs &... args) : Arg0(arg0), parent_t(args...) {}
    };

    template <typename... Args>
    auto overload(Args&&... args) -> overload_class<std::decay_t<Args>...> { return { std::forward<Args>(args)... }; }


    template <typename T>
    struct type_t { using type = T; };

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

  enum class any_stored_value_type : char
    {
      large,
      small,
    };
    constexpr auto any_small_size = 24; // possibly it's cooler to allow to specify it differently for different types of anys
    struct any_all_types {};

    template <typename T>
    using get_any_stored_value_type = std::integral_constant<any_stored_value_type, sizeof(T) <= any_small_size ? any_stored_value_type::small : any_stored_value_type::large>;

  template <class Trait>
  struct trait_impl { static_assert (std::is_same<Trait, void>::value, "Trait is not implemented"); };

  /* BEGIN any_trait::destructible implementation */
  template <>
  struct trait_impl<any_trait::destructible> {
    template <any_stored_value_type>
    struct func_impl;
    using dtor_signature = void(*)(void *);

    template <class RealType>
    struct any_base;
  };

  template <>
  struct trait_impl<any_trait::destructible>::func_impl<any_stored_value_type::small>
  {
    dtor_signature call_dtor = nullptr;

    template <typename T>
    static void placement_dtor(void *other) {
      return (static_cast<T *> (other))->~T();
    }

    template <typename T>
    constexpr func_impl(detail::type_t<T>) : call_dtor(&placement_dtor<T>) {
    }
  };

  template <>
  struct trait_impl<any_trait::destructible>::func_impl<any_stored_value_type::large>
  {
    dtor_signature call_dtor = nullptr;

    template <typename T>
    static void dtor(void *other) {
      return ::delete (static_cast<T *> (other));
    }

    template <typename T>
    constexpr func_impl(detail::type_t<T>) : call_dtor(&dtor<T>) {
    }
  };

  template <class RealType>
  struct trait_impl<any_trait::destructible>::any_base {
    ~any_base() {
      auto real_this = static_cast<RealType *> (this);
      if (!real_this->has_value())
        return;
      real_this->visit_ftable([&](auto f_table) { f_table->call_dtor(real_this->data_ptr());  });
      real_this->d.type_data.t_info = nullptr;
    }
  };
  /* END any_trait::destructible implementation */

  /* BEGIN any_trait::copiable implementation */
  template <>
  struct trait_impl<any_trait::copiable> {
    template <any_stored_value_type>
    struct func_impl;

    template <class RealType>
    struct any_base;
  };

  template <>
  struct trait_impl<any_trait::copiable>::func_impl<any_stored_value_type::large>
  {
    using clone_signature = void *(*)(const void *);
    clone_signature call_clone;
    template <typename T>
    static void *clone(const void *other) {
      return ::new T(*static_cast<const T *> (other));
    }

    template<typename T>
    constexpr func_impl(detail::type_t<T>) : call_clone(&clone<T>) {}
  };

  template <>
  struct trait_impl<any_trait::copiable>::func_impl<any_stored_value_type::small>
  {
    using copy_signature = void(*)(void *, const void *);
    copy_signature call_copy;

    template <typename T>
    static void copy(void *target, const void *source) {
      new (target) T(*static_cast<const T *> (source));
    }

    template <typename T>
    constexpr func_impl(detail::type_t<T>) : call_copy(&copy<T>) {
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

      real_this->visit_ftable(overload(
        [&](const func_impl<any_stored_value_type::small> *f_table) { f_table->call_copy(real_this->d.small_data, other.d.small_data); },
        [&](const func_impl<any_stored_value_type::large> *f_table) { real_this->d.data = f_table->call_clone(other.d.data); }));
    }
  };
  /* END any_trait::copiable implementation */

  /* BEGIN any_trait::movable implementation */
  template <>
  struct trait_impl<any_trait::movable> {
    template <any_stored_value_type>
    struct func_impl;

    template <class RealType>
    struct any_base;
  };

  template <>
  struct trait_impl<any_trait::movable>::func_impl <any_stored_value_type::large>
  {
    template <typename T>
    constexpr func_impl(detail::type_t<T>) {}
  };

  template <>
  struct trait_impl<any_trait::movable>::func_impl<any_stored_value_type::small>
  {
    using move_signature = void(*)(void *, void *);
    template <typename T>
    static void do_move(void *target, void *source) {
      new(target) T(std::move(*static_cast<T *> (source)));
    }
    template <typename T>
    constexpr func_impl(detail::type_t<T>) : call_move(&do_move<T>) {
    }
    move_signature call_move = nullptr;
  };

  template <class RealType>
  struct trait_impl<any_trait::movable>::any_base {
    void move_from(RealType &&other) {
      auto real_this = static_cast<RealType *> (this);
      real_this->~RealType();
      real_this->d.type_data = other.d.type_data;
      real_this->visit_ftable(overload(
        [&](const func_impl<any_stored_value_type::small> *f_table) { f_table->call_move(real_this->data_ptr(), other.data_ptr()); },
        [&](const func_impl<any_stored_value_type::large> *) { real_this->d.data = other.d.data; }));
      other.d.type_data.clear();
    }
  };
  /* END any_trait::movable implementation */

  /* BEGIN any_trait::comparable implementation */
  template <>
  struct trait_impl<any_trait::comparable> {
    template <any_stored_value_type>
    struct func_impl
    {
      using equal_to_signature = bool(*)(const void *, const void *);
      template <typename T>
      static bool equal_to(const void *first, const void *second) {
        return *static_cast<const T *> (first) == *static_cast<const T *> (second);
      }

      equal_to_signature call_equal_to = nullptr;

      template <typename T>
      constexpr func_impl(detail::type_t<T>) : call_equal_to(&equal_to<T>)
      {}
    };

    template <class RealType>
    struct any_base {

      bool operator==(const RealType &other) const {
        auto real_this = static_cast<const RealType *> (this);
        if (!real_this->has_value() || !other.has_value())
          return real_this->has_value() == other.has_value();
        return real_this->type() == other.type() && real_this->visit_ftable([&](auto f_table)
        { return f_table->call_equal_to(real_this->data_ptr(), other.data_ptr());  });
      }

      friend bool operator==(const RealType &first, const RealType & second) {
        return first.operator==(second);
      }

      friend bool operator==(const RealType &first, detail::forbid_implicit_casts<const RealType &> second) {
        return second.value.operator== (first);
      }

      bool operator!=(const RealType &other) const {
        auto real_this = static_cast<const RealType *> (this);
        return !(*real_this == other);
      }

      friend bool operator!=(const RealType &first, detail::forbid_implicit_casts<const RealType &> second) {
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
    template <any_stored_value_type>
    struct func_impl
    {
      using less_than_signature = bool(*)(const void *, const void *);
      template <typename T>
      static bool less_than(const void *first, const void *second) {
        return *static_cast<const T *> (first) < *static_cast<const T *> (second);
      }

      less_than_signature call_less_than = nullptr;

      template <typename T>
      constexpr func_impl(detail::type_t<T>) : call_less_than(&less_than<T>) {}
    };

    template <class RealType>
    struct any_base {

      bool operator<(const RealType &other) const {
        auto real_this = static_cast<const RealType *> (this);
        if (!real_this->has_value() || !other.has_value())
          return real_this->has_value() < other.has_value();
        if (real_this->type() != other.type())
          return std::type_index(real_this->type()) < std::type_index(other.type());

        return real_this->visit_ftable([&](auto f_table)
        { return f_table->call_less_than(real_this->data_ptr(), other.data_ptr());  });
      }

      friend bool operator<(const RealType &first, const RealType & second) { return first.operator<(second); }
      friend bool operator<(const RealType &first, detail::forbid_implicit_casts<const RealType &> second) {
        return first.operator<(second);
      }
      bool operator>(const RealType &other) const {
        auto real_this = static_cast<const RealType *> (this);
        return other.operator< (*real_this);
      }
      friend bool operator>(const RealType &first, const RealType & second) {
        return first.operator>(second);
      }
      friend bool operator>(const RealType &first, detail::forbid_implicit_casts<const RealType &> second) {
        return first.operator>(second);
      }
      bool operator>=(const RealType &other) const {
        auto real_this = static_cast<const RealType *> (this);
        return !(*real_this < other);
      }
      friend bool operator>=(const RealType &first, const RealType & second) {
        return first.operator>=(second);
      }
      friend bool operator>=(const RealType &first, detail::forbid_implicit_casts<const RealType &> second) {
        return first.operator>=(second);
      }
      bool operator<=(const RealType &other) const {
        auto real_this = static_cast<const RealType *> (this);
        return !(*real_this > other);
      }
      friend bool operator<=(const RealType &first, const RealType & second) {
        return first.operator<=(second);
      }
      friend bool operator<=(const RealType &first, detail::forbid_implicit_casts<const RealType &> second) {
        return first.operator<=(second);
      }
    };
  };
  /* END any_trait::orderable implementation */

  /* BEGIN any_trait::hashable implementation */
  template <>
  struct trait_impl<any_trait::hashable> {
    template <any_stored_value_type>
    struct func_impl
    {
      using hash_signature = std::size_t(*)(const void *);
      template <typename T>
      static std::size_t hash_func(const void *value) {
        return std::hash<T>()(*static_cast<const T *> (value));
      }

      hash_signature call_hash = nullptr;

      template <typename T>
      constexpr func_impl(detail::type_t<T>) : call_hash(&hash_func<T>)
      {}
    };

    template <class RealType>
    struct any_base {
      std::size_t hash() const
      {
        auto real_this = static_cast<const RealType *> (this);
        if (!real_this->has_value())
          return 7927u; // hash for empty any
        std::size_t res = real_this->visit_ftable([&](auto f_table) { return f_table->call_hash(real_this->data_ptr());  });
        detail::hash_combine(res, std::type_index(*real_this->d.type_data.t_info)); // adding type_info hash to original type hash
        return res;
      };
    };
  };
  /* END any_trait::hashable implementation */

  /* BEGIN any_trait::callable implementation */
  template <typename Ret, typename... ArgTypes>
  struct trait_impl<any_trait::callable<Ret(ArgTypes...)>> {
    template <any_stored_value_type>
    struct func_impl
    {
      using signature = Ret(*)(const void *, ArgTypes...);
      template <typename T>
      static Ret func(const void *object, ArgTypes... args) {
        return (*static_cast<const T *> (object))(args...);
      }
      signature call_call = nullptr;
      template <typename T>
      constexpr func_impl(detail::type_t<T>)
      {
        call_call = &func<T>;
      }
    };

    template <class RealType>
    struct any_base {
      Ret operator ()(ArgTypes&&... args) const {
        auto real_this = static_cast<const RealType *> (this);
        if (!real_this->has_value())
          throw std::bad_function_call{};
        return real_this->visit_ftable([&](auto f_table) { return f_table->call_call(real_this->data_ptr(), std::forward<ArgTypes>(args)...);  });
      }
    };
  };
  /* END any_trait::callable implementation */

 template <any_stored_value_type value_type, class... Traits>
 struct func_table : trait_impl<Traits>::template func_impl<value_type>... {

   template <typename T>
   constexpr func_table(detail::type_t<T> t) : trait_impl<Traits>::template func_impl<value_type>(t)... {}
 };

 template <typename T, any_stored_value_type value_type, class... Traits>
 struct func_table_instance {
      static constexpr func_table<value_type, Traits...> value = { detail::type_t<T>() };
    };

 template <typename T, any_stored_value_type value_type, class... Traits>
 constexpr func_table<value_type, Traits...> func_table_instance<T, value_type, Traits...>::value;

  template <class... Traits>
  class any_t : public trait_impl<Traits>::template any_base<any_t<Traits...>>... {
    using self = any_t;
    constexpr static bool is_copiable = detail::tmp::one_of<any_trait::copiable, Traits...>::value;
    constexpr static bool is_movable = detail::tmp::one_of<any_trait::movable, Traits...>::value;
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
    void dispatch_and_fill(std::integral_constant<any_stored_value_type, any_stored_value_type::small> c, Type &&value) {
      using decayed_type = std::decay_t<Type>;
      d.type_data.small_f_table = &detail::func_table_instance<decayed_type, any_stored_value_type::small, Traits...>::value;
      new (d.small_data) decayed_type(std::forward<Type>(value));
    }

    template <typename Type>
    void dispatch_and_fill(std::integral_constant<any_stored_value_type, any_stored_value_type::large> c, Type &&value) {
      using decayed_type = std::decay_t<Type>;
      d.type_data.large_f_table = &detail::func_table_instance<decayed_type, any_stored_value_type::large, Traits...>::value;
      d.data = new decayed_type(std::forward<Type>(value));

    }

    any_t(const self &other) { static_assert (is_copiable, "class copy construction is prohibited due to lack of copiable trait");  this->clone(other); }
    self &operator=(const self &other) { static_assert (is_copiable, "class copy assignment is prohibited due to lack of copiable trait"); this->clone(other); return *this; }
    any_t(self &&other) { static_assert (is_movable, "class move construction is prohibited due to lack of movable trait");  this->move_from(std::move(other)); }
    self &operator=(self &&other) { static_assert (is_movable, "class move assignment is prohibited due to lack of movable trait"); this->move_from(std::move(other)); return *this; }
    const std::type_info &type() const { return *d.type_data.t_info; }
    bool has_value() const { return d.type_data.t_info; }
    void reset() { this->~self(); }

  private:
    template <typename Type>
    Type *cast() {
      if (*d.type_data.t_info == typeid (std::remove_const_t<Type>))
        return static_cast<Type *> (data_ptr());

      return nullptr;
    }

    void *data_ptr() {
      switch (d.type_data.stored_value_type) {
      case any_stored_value_type::large: return d.data;
      case any_stored_value_type::small: return &d.small_data;
      }
      return nullptr;
    }

    const void *data_ptr() const {
      return (const_cast<self *> (this))->data_ptr();
    }

    template <class VisitorType>
    auto visit_ftable(const VisitorType &visitor) const
    {
      switch (d.type_data.stored_value_type)
      {
      case any_stored_value_type::small:
        return visitor(d.type_data.small_f_table);
        break;
      case any_stored_value_type::large:
        return visitor(d.type_data.large_f_table);
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
          const func_table<any_stored_value_type::small, Traits...> *small_f_table = nullptr;
          const func_table<any_stored_value_type::large, Traits...> *large_f_table;
        };
        const std::type_info *t_info = nullptr;
        any_stored_value_type stored_value_type = any_stored_value_type::large;
        void clear() {
          small_f_table = nullptr;
          t_info = nullptr;
          stored_value_type = any_stored_value_type::large; // shouldn't matter
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
    template <typename Type, typename... Traits1>
    friend Type *awt::any_cast(awt::any<Traits1...> *value);
    template <typename Type, typename... Traits1>
    friend const Type *awt::any_cast(const awt::any<Traits1...> *value);
  };
  } // namespace detail

  template <typename Type, typename... Traits>
  Type *any_cast(any<Traits...> *value) {
    if (value)
      return value->template cast<Type>();

    return nullptr;
  }

  template <typename Type, typename... Traits>
  const Type *any_cast(const any<Traits...> *value) {
    if (value)
      return value->template cast<Type>();

    return nullptr;
  }

  template <typename Type, typename... Traits>
  Type &any_cast(any<Traits...> &value) {
    auto ptr = any_cast<Type> (&value);
    if (ptr)
      return *ptr;

    throw std::bad_cast{}; // technically should be bad_any_cast
  }

  template <typename Type, typename... Traits>
  const Type &any_cast(const any<Traits...> &value) {
    auto ptr = any_cast<Type> (&value);
    if (ptr)
      return *ptr;

    throw std::bad_cast{}; // technically should be bad_any_cast
  }
} // namespace awt

namespace std
{
  template <class... Traits>
  struct hash<awt::any<Traits...>>
  {
  private:
    using wrapped_type = awt::any<Traits...>;
  public:
    size_t operator ()(const wrapped_type &value) const { return value.hash(); }
  };
}

