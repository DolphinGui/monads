#include <stdexcept>

#include "mtx/monads.hpp"
#include "gtest/gtest.h"

struct NotEven : public std::runtime_error {
  NotEven(const char *what) : std::runtime_error(what) {}
};
struct NonevenErr {};
int add15_f(int i) { return i + 15; }
constexpr auto add15 = mtx::Monad(add15_f);
int evensOnly_f(int i) {
  if (i % 2 != 0)
    throw NotEven("not an even number");
  return i;
}
constexpr auto evensOnly = mtx::Monad(evensOnly_f);
constexpr auto eleven = mtx::Value(11);
constexpr auto four = mtx::Value(4);

TEST(exceptions, base) {
  auto result = four >>= mtx::Trycatch<NotEven> >>= evensOnly >>= add15;
  ASSERT_TRUE(result().value() == 19);
}

TEST(exceptions, unhandled) {
  auto result = eleven >>= mtx::Trycatch<NotEven> >>= evensOnly >>= add15;
  ASSERT_TRUE(!result().has_value());
}

TEST(exceptions, handled) {
  const char *str = nullptr;
  auto handler = [&](const NotEven &e) {
    str = e.what();
    return NonevenErr{};
  };
  auto result = four >>= mtx::TrycatchHandled<NotEven>(handler) >>= add15 >>=
      evensOnly;
  ASSERT_TRUE(std::holds_alternative<NonevenErr>(result()));
  ASSERT_STREQ(str, "not an even number");
}

TEST(exceptions, rethrown) {
  auto handler = [&](const NotEven &e) { return NonevenErr{}; };
  auto result = four >>= mtx::TrycatchHandled<NotEven, true>(handler) >>=
      add15 >>= evensOnly;
  bool has_thrown = false;
  try {
    auto n = result();
    static_assert(!mtx::is_specialization<std::variant, decltype(n)>,
                  "rethrowing exception handling monads should not"
                  "return result types");
  } catch (...) {
    has_thrown = true;
  }
  ASSERT_TRUE(has_thrown);
}