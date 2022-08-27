#include <optional>
#include <system_error>

#include "mtx/monads.hpp"
#include "gtest/gtest.h"
#include <variant>

int add15_f(int i) { return i + 15; }
constexpr auto add15 = mtx::Monad(add15_f);
constexpr auto maybe_add15 = mtx::Maybe >>= add15;

TEST(maybe, success) {
  std::optional<int> a = 12;
  auto n = mtx::Value(a) >>= maybe_add15;

  EXPECT_TRUE(n().has_value());
  EXPECT_EQ(*n(), 27);
}

TEST(maybe, failure) {
  std::optional<int> a = std::nullopt;
  auto n = mtx::Value(a) >>= maybe_add15;

  EXPECT_TRUE(!n().has_value());
}

std::variant<std::error_condition, int> getnum_f(int filenum) {
  if (filenum == 12)
    return 12;
  else
    return std::errc::io_error;
}
constexpr auto getnum = mtx::Monad(getnum_f);

TEST(fallible, success) {
  auto sum = mtx::Value(12) >>= getnum >>=
      mtx::Fallible<std::error_condition> >>= add15;
  auto n = sum();

  EXPECT_TRUE(std::holds_alternative<int>(n));
  EXPECT_EQ(std ::get<int>(n), 27);
}

TEST(fallible, fail) {
  auto sum = mtx::Value(15) >>= getnum >>=
      mtx::Fallible<std::error_condition> >>= add15 >>= getnum >>=
      mtx::Fallible<std::error_condition> >>= add15 >>= getnum >>=
      mtx::Fallible<std::error_condition> >>= add15;
  auto n = sum();
  EXPECT_TRUE(std::holds_alternative<std::error_condition>(n));
  EXPECT_EQ(std ::get<std::error_condition>(n), std::errc::io_error);
}