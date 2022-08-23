# Monads

A monad is just a monoid in the category of endofunctors. Ok, more seriously a monad is a syntax akin to functions or variables. It's much easier to show rather than tell what a monad is.

```c++
#include "mtx/monads.hpp"

auto val = mtx::Value(123)
auto add1 = mtx::Monad([](int i){return i + 1;});
auto sum = val >>= add1;
auto sum_val = sum(); // sum_val == 124
```

As you can see, you first take a value and wrap it into a `Value`, then use the `>>=` operator to apply function Monad. When the Monad is applied by calling it like a function, the function is called on the value. In this case, it calls the lambda onto the value.

To state the obvious, monads are used to reprsent function composition logic. The `Value` monad is a syntactic sugar for `Monad([]{return value;})`, and is essentially a function in it of itself. The sum monad could be said to represent add1 ∘ val.

This library also contains various error handling monads. I think error handling can be relatively tersely expressed via monads rather than procedurally.

```c++

auto file = get_file(filename);
if(!a.has_value)
  return io_error;
auto json = to_json(read_file(*file, locale));
if(!json.has_value)
  return parse_error;
if(json->has_value(key))
  return not_found_error;
return json[key];
```

Here the error handling is interspersed between the business logic, making the code relatively messy.

```c++
try{
  auto file = get_file(filename);
  auto json = to_json(read_file(file, locale))
  return json[key]
} catch(const exception& e){
  return error(e); // or log and rethrow if that's what you want
}
```

Exceptions are significantly better in that all of the error handling logic is kept separately from the business logic. While variables are technically kept alive longer than strictly necessary, and arguably the try-catch block syntax is ugly, exceptions for many cases represent the most transparent and clean way to error handle. Exceptions even allow for easy error propogation.

The downside of exceptions is also arguably its upside. Many people don't actively think about exceptions, and don't write code that handles it well. Arguably this is a symptom of people not really error handling, and also the difficulty in writing code that is exception-safe or tolerant.

Functional programming tends not to use exceptions in favor of error handling, where monads make error checking much less painful.

```c++
// just pretend all of the functions are now monads
auto val = mtx::Value(filename) >>= get_file >>= mtx::Maybe
>>= read_file(locale) >>= to_json >>= mtx::Maybe >> getValue(key);
return val();
```

Here, the functions are "piped" to each other, with the Maybe monad representing conditional execution on whether or not the result is null. The `val()` returns an `optional` containing the value, although a `Fallible` monad works largely the same way for error codes instead of null;

```c++
auto val = mtx::Value(filename) >>= mtx::Trycatch >>= get_file >>= read_file(locale)
>>= to_json >>= getValue(key);
return val()
```

This is the exception based monad approach using the Trycatch monad. Note that this doesn't really have an analogue in most functional programming languages, as it uses exceptions to propogate the error. `val()` here also returns `optional`, although a `TrycatchHandled` monad exists that allows you to convert an exception into an error code.

For more usage, you can look at the unit tests in the `tests` directory. The monads were built using the transforming monad specialization returned by `TransformMonad`. The header file shows how `TransformMonad` does its magic.
