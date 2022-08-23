#pragma once

#include <functional>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace mtx {
namespace detail {
template <typename Functor, typename = void> class MonadT;
struct Transform {};
template <template <class...> class, class...>
struct is_specialization : std::false_type {};
template <template <class...> class Template, class... Args>
struct is_specialization<Template, Template<Args...>> : std::true_type {};
template <typename T>
constexpr bool is_monad = is_specialization<MonadT, T>::value;
} // namespace detail
template <typename T>
concept nonmonadic = !detail::is_monad<T>;
template <typename T>
concept monadic = detail::is_monad<T>;

template <typename Functor> struct detail::MonadT<Functor> {
  Functor f;

  constexpr MonadT(Functor &&f) : f(std::move(f)) {}
  template <typename... Params>
  decltype(auto) operator()(Params &&...params) const
      noexcept(noexcept(f(params...))) {
    return f(std::forward<Params>(params)...);
  }
  template <typename... Params>
  decltype(auto)
  operator()(Params &&...params) noexcept(noexcept(f(params...))) {
    return f(std::forward<Params>(params)...);
  }

  template <typename R>
  constexpr decltype(auto) operator>>=(MonadT<R> right) noexcept {
    auto n = [left = std::move(*this),
              right = std::move(right)]<typename... Args>(Args &&...args) {
      return right.f(left.f(std::forward<Args>(args)...));
    };
    return MonadT<decltype(n)>(std::move(n));
  }

  template <typename R>
  constexpr decltype(auto) operator>>=(MonadT<R> right) const noexcept {
    auto n = [left = *this, right = right]<typename... Args>(Args &&...args) {
      return right.f(left.f(std::forward<Args>(args)...));
    };
    return MonadT<decltype(n)>(std::move(n));
  }
};
template <typename Functor> constexpr auto Monad(Functor &&f) noexcept {
  return detail::MonadT<Functor>(std::forward<Functor>(f));
}
// transformative monads cannot be invoked like normal monads
// and may only act on other monads
template <typename Functor> struct detail::MonadT<Functor, detail::Transform> {
  Functor f;

  constexpr MonadT(Functor &&f) : f(std::move(f)) {}
  template <typename R>
  constexpr decltype(auto) operator>>=(MonadT<R> right) const noexcept {
    return MonadT<decltype(f(right))>(f(right));
  }
  template <typename R>
  constexpr decltype(auto) operator>>=(MonadT<R> right) noexcept {
    return MonadT<decltype(f(right))>(f(right));
  }
};
template <typename T> constexpr auto TransformMonad(T &&f) noexcept {
  return detail::MonadT<T, detail::Transform>(std::forward<T>(f));
}

constexpr auto Value(auto &&val) {
  return Monad([val = std::move(val)]() { return val; });
}

constexpr auto Identity = Monad([](auto &&param) { return param; });
constexpr auto Maybe = TransformMonad(
    [](auto &&mon) requires monadic<std::decay_t<decltype(mon)>> {
      return [mon = std::move(mon)]<typename T>(std::optional<T> &&param)
                 -> std::optional<std::invoke_result_t<decltype(mon), T>> {
        if (param.has_value())
          return mon(*param);
        else
          return std::nullopt;
      };
    });

template <typename ErrType>
constexpr auto Fallible = TransformMonad([
](auto &&mon) requires monadic<std::decay_t<decltype(mon)>> {
  return [mon = std::move(mon)]<typename T>(std::variant<ErrType, T> &&param)
             -> std::variant<ErrType, std::invoke_result_t<decltype(mon), T>> {
    if (std::holds_alternative<ErrType>(param)) {
      return param;
    } else {
      return mon(std::get<T>(param));
    }
  };
});

template <typename ExceptionType, bool no_passthrough = true>
constexpr auto Trycatch = TransformMonad([
](auto &&mon) requires monadic<std::decay_t<decltype(mon)>> {
  return [mon = std::move(mon)]<typename... T>(T &&...params) noexcept(
             no_passthrough)
             -> std::optional<std::invoke_result_t<decltype(mon), T...>> {
    try {
      return mon(std::forward<T>(params)...);
    } catch (const ExceptionType &) {
      return std::nullopt;
    }
  };
});

template <typename ExceptionType, bool rethrow = false,
          bool no_passthrough = true>
constexpr auto TrycatchHandled(auto &&handler) noexcept {
  using ErrType = std::invoke_result_t<decltype(handler), ExceptionType>;
  return TransformMonad([handler = std::move(handler)](
      auto &&mon) requires monadic<std::decay_t<decltype(mon)>> {
    return [mon = std::move(mon), handler = std::move(handler)]<typename... T>(
               T &&...params) noexcept(!rethrow && no_passthrough)
               -> std::conditional_t<
                   !rethrow,
                   std::variant<ErrType,
                                std::invoke_result_t<decltype(mon), T...>>,
                   std::invoke_result_t<decltype(mon), T...>> {
      try {
        return mon(std::forward<T>(params)...);
      } catch (const ExceptionType &e) {
        if constexpr (rethrow) {
          handler(e);
          throw;
        } else {
          return handler(e);
        }
      }
    };
  });
}
} // namespace mtx