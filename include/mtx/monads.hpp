#pragma once

#include <functional>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace mtx {
template <typename, typename> class MonadT;
namespace detail {
template <template <typename...> typename, class...>
struct is_specialization_impl : std::false_type {};
template <template <typename...> typename Template, typename... Args>
struct is_specialization_impl<Template, Template<Args...>> : std::true_type {};
struct Uncallable_t {};
constexpr auto Uncallable = Uncallable_t{};
template <typename T> struct strip_optional_impl { using type = T; };
template <typename T> struct strip_optional_impl<std::optional<T>> {
  using type = T;
};
} // namespace detail
template <template <class...> class Class, class... Args>
constexpr bool is_specialization =
    detail::is_specialization_impl<Class, Args...>::value;
template <typename T> constexpr bool is_monad = is_specialization<MonadT, T>;
template <typename T>
using strip_optional = typename detail::strip_optional_impl<T>::type;
template <typename Var>
using strip_err = std::conditional_t<is_specialization<std::variant, Var>,
                                     std::variant_alternative_t<1, Var>, Var>;

template <typename T>
concept nonmonadic = !is_monad<T>;
template <typename T>
concept monadic = is_monad<T>;

template <typename Functor, typename Composer> struct MonadT {
  Functor _f;
  Composer _c;
  constexpr MonadT(Functor &&f, Composer &&c)
      : _f(std::move(f)), _c(std::move(c)) {}

  constexpr MonadT(const Functor &f, const Composer &c) : _f(f), _c(c) {}

  template <typename Sig> constexpr auto releaseFunctor() noexcept {
    return std::function<Sig>(std::move(_f));
  }
  template <typename Sig> constexpr auto copyFunctor() const noexcept {
    return std::function<Sig>(_f);
  }

  template <typename F, typename C>
  constexpr auto operator>>=(MonadT<F, C> right) const noexcept {
    return _c(_f, right._f, _c);
  }

  template <typename... Params>
  constexpr auto operator()(Params &&...p) noexcept(noexcept(_f(p...))) {
    return _f(std::forward<Params>(p)...);
  }
  template <typename... Params>
  constexpr auto operator()(Params &&...p) const noexcept(noexcept(_f(p...))) {
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
              -> std::optional<
                  strip_optional<std::invoke_result_t<decltype(right), T>>> {
            if (p.has_value()) {
              using Ret = std::invoke_result_t<decltype(right), T>;
              if constexpr (is_specialization<std::optional, Ret>) {
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

// unlike the maybe monad this can't strip redundant Result variants
// so it shouldn't be used on functions that are already fallible
template <typename ErrType>
constexpr auto Fallible = MonadT(
    detail::Uncallable,
    []<typename L, typename R, typename Self>(L &&left, R &&right, Self &&s) {
      return MonadT(
          [right = std::move(right)]<typename T>(std::variant<ErrType, T> p)
              -> std::variant<ErrType,
                              std::invoke_result_t<decltype(right), T>> {
            using Ret = std::invoke_result_t<decltype(right), T>;
            if (std::holds_alternative<T>(p)) {
              if constexpr (is_specialization<std::variant, Ret>) {
                auto result = right(std::get<T>(p));
                if (std::holds_alternative<Ret>(result))
                  return std::get<Ret>(result);
                else
                  return std::get<ErrType>(result);
              } else {
                return right(std::get<T>(p));
              }
            } else {
              return std::get<ErrType>(p);
            }
          },
          std::move(s));
    });

template <typename Exception, bool passthrough = false>
constexpr auto Trycatch = MonadT(detail::Uncallable, []<typename L, typename R,
                                                        typename Self>(
                                                         L &&left, R &&right,
                                                         Self &&s) {
  return MonadT(
      [right = std::move(right)]<typename... Params>(Params &&...p) noexcept(
          passthrough)
          -> std::optional<std::invoke_result_t<decltype(right), Params...>> {
        try {
          return right(std::forward<Params>(p)...);
        } catch (const Exception &) {
          return std::nullopt;
        }
      },
      std::move(s));
});

template <typename Exception, bool rethrow = false, bool passthrough = false>
constexpr auto TrycatchHandled(auto &&handler) {
  return MonadT(
      detail::Uncallable,
      [handler = std::move(handler)]<typename L, typename R, typename Self>(
          L &&left, R &&right, Self &&s) {
        return MonadT(
            [right = std::move(right),
             handler = std::move(handler)]<typename... Params>(
                Params &&...p) noexcept(passthrough)
                -> std::conditional_t<
                    rethrow, std::invoke_result_t<decltype(right), Params...>,
                    std::variant<
                        std::invoke_result_t<decltype(handler), Exception>,
                        std::invoke_result_t<decltype(right), Params...>>> {
              try {
                return right(std::forward<Params>(p)...);
              } catch (const Exception &e) {
                if constexpr (!rethrow) {
                  return handler(e);
                } else {
                  handler(e);
                  throw;
                }
              }
            },
            std::move(s));
      });
}
} // namespace mtx