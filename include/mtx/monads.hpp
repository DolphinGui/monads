#pragma once

#include <functional>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace mtx {
namespace detail {
template <typename Functor> class Monad;
template <template <class...> class, class...>
struct is_specialization_impl : std::false_type {};
template <template <class...> class Template, class... Args>
struct is_specialization_impl<Template, Template<Args...>> : std::true_type {};
template <template <class...> class Class, class... Args>
constexpr bool is_specialization =
    is_specialization_impl<Class, Args...>::value;
template <typename T> constexpr bool is_monad = is_specialization<Monad, T>;
struct Uncallable_t {};
constexpr auto Uncallable = Uncallable_t{};
template <typename T> struct strip_optional_impl { using type = T; };
template <typename T> struct strip_optional_impl<std::optional<T>> {
  using type = T;
};
template <typename T> using strip_optional = typename strip_optional_impl<T>::type;
} // namespace detail
template <typename T>
concept nonmonadic = !detail::is_monad<T>;
template <typename T>
concept monadic = detail::is_monad<T>;

template <typename Functor, typename Composer> struct MonadT {
  Functor _f;
  Composer _c;
  constexpr MonadT(Functor &&f, Composer &&c)
      : _f(std::move(f)), _c(std::move(c)) {}
  constexpr MonadT(const Functor &f, const Composer &c) : _f(f), _c(c) {}

  template <typename F, typename C>
  constexpr auto operator>>=(MonadT<F, C> right) const {
    return _c(_f, right._f, _c);
  }

  template <typename... Params> constexpr auto operator()(Params &&...p) {
    return _f(std::forward<Params>(p)...);
  }
};
constexpr auto Monad(auto &&Functor) noexcept {
  return MonadT(
      std::forward<std::decay_t<decltype(Functor)>>(Functor),
      []<typename L, typename R, typename Self>(L &&left, R &&right, Self &&s) {
        return MonadT(
            [left = std::move(left),
             right = std::move(right)]<typename... Params>(Params &&...p) {
              return right(left(std::forward<Params>(p)...));
            },
            std::move(s));
      });
}
constexpr auto Value(auto &&val) noexcept {
  return Monad([val = std::move(val)]() { return val; });
}

constexpr auto Maybe = MonadT(
    detail::Uncallable,
    []<typename L, typename R, typename Self>(L &&left, R &&right, Self &&s) {
      return MonadT(
          [right = std::move(right)]<typename T>(std::optional<T> p)
              -> std::optional<detail::strip_optional<
                  std::invoke_result_t<std::decay_t<decltype(right)>, T>>> {
            if (p.has_value()) {
              using Ret =
                  std::invoke_result_t<std::decay_t<decltype(right)>, T>;
              if constexpr (detail::is_specialization<std::optional, Ret>) {
                auto result = right(*p);
                if (result.has_value())
                  return *result;
                else
                  return std::nullopt;
              } else {
                return right(*p);
              }
            } else {
              return std::nullopt;
            }
          },
          std::move(s));
    });

} // namespace mtx