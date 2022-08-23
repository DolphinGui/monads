#include <optional>
#include <system_error>

#include "mtx/monads.hpp"
#include "gtest/gtest.h"
#include <variant>

auto lambda = [](int i) { return i + 1; };
auto monad1 = mtx::Monad(std::move(lambda));
int fun(int i) { return i + 1; }
auto monad2 = mtx::Monad(fun);
auto monad3 = mtx::Monad(std::function<int(int)>(&fun));
constexpr auto value = mtx::Value(1);

TEST(composition, mixing) {
  auto sum = value >>= monad1 >>= monad2 >>= monad3;
  ASSERT_EQ(sum(), 4);
}

TEST(composition, storing) {
  auto sum = value >>= monad1 >>= monad2 >>= monad3;
  std::function m = sum.releaseFunctor<int()>();
  ASSERT_EQ(m(), 4);
}