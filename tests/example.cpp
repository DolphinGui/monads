#include "mtx/monads.hpp"
#include "fmt/format.h"

int main() {
  auto lazy = mtx::Value(14);
  auto add1 = mtx::Monad([](int i) { return i + 1; });
  auto add2 = add1 >>= add1;
  auto val = mtx::Value(12);
  auto total = val >>= add2;
  auto maybeadd = mtx::Maybe >>= add2;
  auto maybeval = mtx::Value(std::optional(12)) >>= maybeadd;

  fmt::print("{}, {}, {}\n", lazy(), total(), *maybeval());
}