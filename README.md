# `tjg::Int`

A C++23 header-only utility class for representing integers with an explicit **endianness**.
This is useful when working with binary protocols, files, or network formats where values must be
stored or transmitted in little-endian or big-endian order regardless of the host machine’s
native endianness.

## Features

- Works with any `std::integral` type (signed or unsigned).
- Encodes **endianness** as an std::endian  template parameter.
- Provides `native()`, `little()`, and `big()` accessors.
- Arithmetic, bitwise, and comparison operators behave like normal integers.
- Implicit conversion to the underlying type (`T`).
- Storage size and alignment match the underlying type exactly.
- `std::hash` specialization for use in unordered containers.
- `endian_cast()`, `narrow_cast()` and `byteswap()` helper functions.

## Example

```cpp
#include "Int.hpp"
#include <iostream>
#include <cstdint>

using namespace tjg;

int main() {
  BigUint<std::uint32_t> bigNum = 0x12345678;
  LilUint<std::uint32_t> lilNum = bigNum;  // endian conversion

  std::cout << std::hex
            << "Native: " << bigNum.native() << "\n"
            << "Big:    " << bigNum.big()    << "\n"
            << "Little: " << bigNum.little() << "\n";

  // Arithmetic
  auto result = bigNum + 5u;  // returns NativeT
  std::cout << "Result: " << result << "\n";
}
```

### Output (on a big-endian machine):

```
Native: 12345678
Big:    12345678
Little: 78563412
Result: 1234567d
```

## API Overview

### Class Template

```cpp
template<std::integral T = int, std::endian E = std::endian::native>
class Int;
```

- **`T`**: underlying integer type (e.g. `std::uint32_t`, `int16_t`).
- **`E`**: endianness for storage.

### Important Aliases

- `BigInt<T>`  – signed big-endian integer.
- `LilInt<T>`  – signed little-endian integer.
- `BigUint<T>` – unsigned big-endian integer.
- `LilUint<T>` – unsigned little-endian integer.

### Core Functions

- `T native() const` – get as native-endian.
- `T little() const` – get as little-endian.
- `T big() const` – get as big-endian.
- `T& raw()` / `const T& raw() const` – direct raw storage access.
- `T* ptr()` – pointer to raw storage (native only).
- Conversion operator to `T`.

### Operators

- **Comparison:** `==`, `<=>` compare numerically.
- **Arithmetic:** `+ - * / %` with `std::integral` or another `Int`.
- **Bitwise:** `| & ^ ~` with `std::integral` or another `Int`.
- **Shift:** `<< >>`.
- **Compound assignments:** `+= -= *= /= %= <<= >>= |= &= ^=`.
- **Increment/decrement:** `++ --`.
- **Boolean context:** `operator bool()`.

### Helper Functions

- `endian_cast<To>(x)` – convert endianness `Int<T, From>` → `Int<T, To>`.
- `narrow_cast<To>(x)` - convert to narrower type of same endianness.
- `byteswap(x)`        – reverse byte order.
- `hash_value(x)`      - produces a hash value for boost::hash compatibility.
- `std::hash<Int>`     - for `std` unordered (hashed) containers

## Design Notes

- **No runtime penalty**: all conversions and swaps are `constexpr`.
- **No-swap equality**: `operator==` compares stored raw bytes (useful for
  serialization).
- **Native results**: arithmetic with `std::integral` or `Int` produces the
  usual C++ native results to avoid redundant swapping.
- **Hashing**: `std::hash<Int>` does not swap.
- **Layout**: `static_assert` checks enforce standard layout, trivial
  copyability, and size/alignment equality with `T`.
