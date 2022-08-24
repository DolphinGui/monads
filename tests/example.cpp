#include "fmt/format.h"
#include "mtx/monads.hpp"

#include <optional>

std::optional<int> evensOnlyF(int i) {
  if (i % 2 == 0)
    return i;
  else
    return std::nullopt;
}
constexpr auto evensOnly = mtx::Monad(evensOnlyF);

int main() {
  auto lazy = mtx::Value(14);
  auto add1 = mtx::Monad([](int i) { return i + 1; });
  auto add2 = add1 >>= add1;
  auto val = mtx::Value(12);
  auto total = val >>= add2;
  auto maybeval = mtx::Value(std::optional(12)) >>= mtx::Maybe >>= add2 >>=
      evensOnly >>= mtx::Maybe >>= add2;

  fmt::print("{}, {}, {}\n", lazy(), total(), *maybeval());
}