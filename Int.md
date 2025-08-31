<!---
@file
@copyright 2025 Terry Golubiewski, all rights reserved.
@author Terry Golubiewski
--->

# `tjg::Int<T, E>` — fixed-endian integral wrapper

A tiny, trivially-copyable integer wrapper that **stores** in a chosen byte
order, yet **behaves like a normal integer** when you use it. Great for network
packets, on-disk formats, and packed structs.

- `T`: an [`std::integral`](https://en.cppreference.com/w/cpp/concepts/integral)
  type (e.g. `std::uint16_t`, `std::int32_t`, `std::uint64_t`).
- `E`: an [`std::endian`](https://en.cppreference.com/w/cpp/types/endian) tag
  that fixes the object’s storage order (`std::endian::big` or
  `std::endian::little`).

`Int` keeps the same size and alignment as `T`, is standard-layout and
[`trivially copyable`](https://en.cppreference.com/w/cpp/types/is_trivially_copyable),
so it works cleanly in POD aggregates and can be `memcpy`’d. Arithmetic/bitwise
operators use **value semantics**; the class simply does any required byte
swapping internally to preserve the chosen storage order.

---

## Synopsis

```cpp
namespace std {
constexpr endian operator~(endian x) noexcept; // flip little<->big
}

namespace tjg {

// Narrowing-aware cast helper (scalar)
template<class To, class From>
constexpr To narrow_cast(From x) noexcept;

// A convertible-to test used by Int’s constrained ctors
template<class From, class To>
concept NonNarrowing = /* convertible via list-init without narrowing */;

// -----------------------------
// Fixed-endian integer wrapper:
// -----------------------------
template<std::integral T = int, std::endian E = std::endian::native>
class Int {
public:
  using value_type = T;
  static constexpr std::endian Endian = E;

  // constructors/assignment
  constexpr Int() noexcept = default;
  constexpr Int(const Int&) noexcept = default;
  constexpr Int& operator=(const Int&) = default;
  constexpr bool operator==(const Int&) const = default;

  // scalar in: explicit to avoid surprises in mixed expressions
  constexpr explicit Int(T v) noexcept;

  // Only non-narrowing conversions/assignment are allowed (explicit).
  template<std::integral U> requires NonNarrowing<U,T>
  constexpr explicit Int(U u) noexcept;

  template<std::integral U, std::endian E2> requires NonNarrowing<U,T>
  constexpr Int& operator=(Int<U, E2> u) noexcept;

  // value access
  [[nodiscard]] constexpr T value() const noexcept; // numeric value

  // reference to physical storage
  constexpr       T& raw()       noexcept;
  constexpr const T& raw() const noexcept;

  // pointer to storage only when storage order == native (SFINAE)
  constexpr       T* ptr()       noexcept
    requires (E == std::endian::native);
  constexpr const T* ptr() const noexcept
    requires (E == std::endian::native);

  // numeric comparison (three-way)
  constexpr auto operator<=>(const Int&) const noexcept; // C++20 three-way
  // unary/arithmetic/bitwise: behave like T by value
  constexpr Int operator+() const noexcept;
  constexpr auto operator-() const noexcept;
  constexpr Int operator~() const noexcept;
  constexpr bool operator!() const noexcept;
  constexpr explicit operator bool() const noexcept;
  // ++/--, compound assigns, shifts, etc. (see header)

  // Int ⊕ Int bitwise ops return Int (same endian)
  constexpr Int operator|(Int) const noexcept;
  constexpr Int operator&(Int) const noexcept;
  constexpr Int operator^(Int) const noexcept;
}; // Int

// CTAD
template<std::integral U>
Int(U) -> Int<U, std::endian::native>;

template<std::signed_integral   T=int> using BigInt  = Int<T, std::endian::big>;
template<std::signed_integral   T=int> using LilInt  = Int<T, std::endian::little>;
template<std::unsigned_integral T=unsigned> using BigUint = Int<T, std::endian::big>;
template<std::unsigned_integral T=unsigned> using LilUint = Int<T, std::endian::little>;

// ------------ Free functions ------------

// Reverse physical byte order (no-ops for 1-byte T)
template<std::integral T, std::endian E>
constexpr Int<T, (E==std::endian::little?std::endian::big:std::endian::little)>
byteswap(Int<T,E> x) noexcept;

// Re-tag endian (at value level)
template<std::endian To, std::integral T, std::endian From>
constexpr Int<T, To> endian_cast(Int<T, From> x) noexcept;

// Narrowing-aware cast between Ints of different T (value-preserving only)
template<std::integral ToT, std::integral FmT, std::endian E>
  requires (!std::is_same_v<ToT,FmT>)
constexpr Int<ToT, std::endian::native> narrow_cast(Int<FmT, E>) noexcept;

// no-op overload when ToT == FmT
template<std::integral ToT, std::integral FmT, std::endian E>
  requires (std::is_same_v<ToT,FmT>)
constexpr Int<ToT, E> narrow_cast(Int<FmT, E>) noexcept;

// Hash interop
template<std::integral T, std::endian E>
constexpr std::size_t hash_value(const Int<T,E>&) noexcept;

} // tjg

namespace std {
// hash specialization for unordered containers
template<std::integral T, std::endian E>
struct hash<::tjg::Int<T,E>> {
  constexpr size_t operator()(const ::tjg::Int<T,E>& x) const noexcept;
};
} // std
```

> **Notes**
>
> • `value()` returns the numeric value as a host-endian `T`.
> • `big()`   returns the numeric value as a big-endian `T`.
> • `little()` returns the numeric value as a little-endian `T`.
> • `raw()` exposes the stored representation (in `E`). Use with care; it’s 
    intended for wire/disk fields.  
> • `ptr()` exists only when `E == std::endian::native` so that taking the
    address is always safe for direct I/O.  
> • All operators (arithmetic, comparison, shifts, bitwise) are defined in terms
    of **numeric value semantics**.
> • `std::byteswap` is a C++23 facility:
    see [`std::byteswap`](https://en.cppreference.com/w/cpp/numeric/byteswap).  
> • The three-way comparison operator is standardized in C++20:
    see [three-way comparison](https://en.cppreference.com/w/cpp/language/three_way_comparison).

---

## Rationale

- **Correctness by construction**: Endianness is fixed at the type level;
  objects always store in that byte order.  
- **Ergonomic use**: Implicit conversion **out** to `T` is allowed; converting
  **in** is `explicit` (and narrowing is rejected) to prevent surprises.  
- **Performance**: On-native storage does no swapping; cross-endian paths swap
  only when needed, often elided by constant-folding.  
- **Interoperability**: Same size/align as `T` and trivially copyable, so it’s
  safe in packed structs and can be read/written via I/O APIs.

---

## Description
`tjg::Int<T,E>` is a thin, trivially copyable wrapper around an integral type `T`
with an associated endianness `E` (either [`std::endian::native`](https://en.cppreference.com/w/cpp/types/endian),
[`std::endian::big`](https://en.cppreference.com/w/cpp/types/endian), or
[`std::endian::little`](https://en.cppreference.com/w/cpp/types/endian)).

It stores the integer in raw form, swapping bytes at construction or access when
necessary, but all operators and conversions behave with **numeric value
semantics**, as if operating directly on `T`.

The class is intended for use in portable binary protocols, file formats, and
packed structures where a specific byte order is required, but where ergonomic
use as a normal integer is also desired.

## Template Parameters
- **T** — any type satisfying [`std::integral`](https://en.cppreference.com/w/cpp/concepts/integral).
- **E** — an [`std::endian`](https://en.cppreference.com/w/cpp/types/endian) tag, defaulting to native.

## Constructors and Assignment
- `Int()` — default initializes to zero.
- `explicit Int(T u)` — construct from a `T`.
- Copy/move constructors and assignment are defaulted.

Assignment overloads mirror construction rules: assigning from narrowing types
is ill-formed.

## Observers
- `T& raw() noexcept;`  
  `const T& raw() const noexcept;`  
  Access underlying storage in endian form.

- `T* ptr() noexcept;` (only if `E == std::endian::native`)  
  Pointer to underlying storage.

- `T value() const noexcept;`  
  Access the numeric value in native endianness.

- `T big() const noexcept;`  
  Access the numeric value in big endianness.

- `T little() const noexcept;`  
  Access the numeric value in little endianness.

- `explicit operator T() const noexcept;`  
  Implicit conversion to `T` yields `value()`.

- `explicit operator bool() const noexcept;`  
  True if value is nonzero.

## Arithmetic and Bitwise Operators
Arithmetic and bitwise operators behave as if applied to the underlying `T`.
- `+ - * / % << >>` with either `Int` or scalar operands → return promoted `T`.
- `| & ^` with `Int` operands → return `Int` of same endian.  
- `| & ^` with mixed scalar/Int operands → return promoted `T`.

Compound assignment (`+=`, `-=`, `*=`, etc.) is provided for both scalars and
other `Int`s.

## Unary Operators
- `+x` → `Int`  
- `-x` → promoted `T`  
- `~x` → `Int`  
- `!x` → `bool`  

## Increment/Decrement
- `++x`, `--x` → `Int&`  
- `x++`, `x--` → returns previous value as `T`

## Comparison Operators
- `operator==` and `<=>` use **numeric value semantics**.

## Non-member Functions
- `hash_value(const Int<T,E>&)` — returns hash of raw storage, coherent with `==`.
- `endian_cast<To>(Int<T,From>)` — convert to a different endian representation.
- `byteswap(Int<T,E>)` — return `Int<T,~E>` with bytes reversed.
- `narrow_cast<ToT>(Int<FmT,E>)` — convert to Int of different underlying type.

## Type Aliases
- `BigInt<T>` — signed big-endian Int.
- `LilInt<T>` — signed little-endian Int.
- `BigUint<T>` — unsigned big-endian Int.
- `LilUint<T>` — unsigned little-endian Int.

## Hash Support
Specialization of [`std::hash`](https://en.cppreference.com/w/cpp/utility/hash)
is provided so `Int<T,E>` can be used as keys in `std::unordered_map` and
`std::unordered_set`.

## FAQ

**Q: Why provide `std::operator~(std::endian)`?**  
A: It’s a convenience used by the library (and callers) to flip
   `std::endian::little` ↔ `std::endian::big` in constant expressions.

**Q: When should I use `raw()` vs `value()`?**  
- Use `value()` (or `static_cast<T>`) for normal arithmetic and comparisons.  
- Use `raw()` only when interfacing with on-the-wire/on-disk fields that are
  already in the fixed byte order.

**Q: How do I swap endianness of an `Int`?**  
Call `tjg::byteswap(x)`. It returns a new `Int<T, ~E>` and is implemented in
terms of [`std::byteswap`](https://en.cppreference.com/w/cpp/numeric/byteswap).

**Q: Is `Int` safe in packed structs or for `memcpy`?**  
Yes. It is standard-layout and trivially copyable with the same size/align as
`T`, so you can use it in PODs and perform raw I/O safely.

---

## Example: IPv4 addresses and ports (network byte order)

Network “byte order” is big-endian. These aliases express that directly in the type:

```cpp
#include "Int.hpp"

#include <array>
#include <span>
#include <string>
#include <exception>
#include <system_error>
#include <utility>
#include <cstdint>
#include <cerrno>

#include <arpa/inet.h>   // ::inet_ntop, ::inet_pton, AF_INET, INET_ADDRSTRLEN
#include <netinet/in.h>  // ::sockaddr_in, ::in_addr, ::sa_family_t
#include <sys/socket.h>  // ::socket, ::bind, ::recvfrom
#include <unistd.h>      // ::close

namespace tjg {

// Strong types for host/network values (IPv4 / port)
using NetIpV4 = tjg::Int<std::uint32_t, std::endian::big>; // wire order
using NetPort = tjg::Int<std::uint16_t, std::endian::big>; // wire order

// Wrap POSIX in_addr but expose a typed network-order IPv4
struct InAddr : public ::in_addr {
  [[nodiscard]]
  NetIpV4 addr() const { auto rv = NetIpV4{}; rv.raw() = s_addr; return rv; }
  void addr(NetIpV4 ip) { s_addr = ip.raw(); }
  InAddr() = default;
  explicit InAddr(NetIpV4 ip) { addr(ip); }
  InAddr& operator=(NetIpV4 ip) { addr(ip); return *this; }
  operator NetIpV4() const { return addr(); }
}; // InAddr

// Wrap POSIX sockaddr_in but expose typed accessors
struct SockAddrIn : public ::sockaddr_in {
  [[nodiscard]] NetPort port() const
    { NetPort p; p.raw() = sin_port; return p; }
  [[nodiscard]] NetIpV4 addr() const
    { NetIpV4 a; a.raw() = sin_addr.s_addr; return a; }
  void port(NetPort p)  { sin_port = p.raw(); }
  void addr(NetIpV4 ip) { sin_addr.s_addr = ip.raw(); }
  explicit SockAddrIn(NetPort p = NetPort{}, NetIpV4 ip = NetIpV4{}) {
    sin_family = AF_INET;
    addr(ip);
    port(p);
  }
}; // SockAddrIn

// String ⇄ IPv4 helpers (network order)
std::string InetPath(NetIpV4 ip) {
  auto buf  = std::array<char, INET_ADDRSTRLEN>{};
  auto tmp  = InAddr{ip};
  auto text = ::inet_ntop(AF_INET, &tmp,
                          buf.data(), static_cast<socklen_t>(buf.size()));
  return text ? std::string{text} : std::string{};
}

NetIpV4 InetAddr(std::string dotted) {
  auto tmp = InAddr{};
  // inet_pton expects network-order result stored into tmp
  if (::inet_pton(AF_INET, dotted.c_str(), &tmp) != 1)
    return NetIpV4{};
  return tmp.addr();
}

// RAII UDP socket wrapper
struct DatagramSocket {
  int fd{-1};

private:
  [[noreturn]] static void Throw(std::string what) {
    throw std::system_error{errno, std::system_category(),
                          std::string{"DatagramSocket: "} + what};
  }

  bool _close() noexcept {
    auto status = 0;
    if (fd != -1)
      status = ::close(fd);
    fd = -1;
    return (status == 0);
  }

public:
  void close() { if (!_close()) Throw("close"); }

  explicit DatagramSocket(const SockAddrIn& local) {
    fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) Throw("socket");
    const auto* sa = reinterpret_cast<const ::sockaddr*>(&local);
    if (::bind(fd, sa, sizeof(local)) < 0) { (void) _close(); Throw("bind"); }
  }

  ~DatagramSocket() { (void) _close(); }

  template<std::size_t N>
  std::size_t recvfrom(std::span<char, N> buf, SockAddrIn* from = nullptr) {
    SockAddrIn remote;
    auto addr_len = static_cast<::socklen_t>(sizeof(remote));
    int flags = 0;
    auto n = ::recvfrom(fd, buf.data(), buf.size(), flags,
                        reinterpret_cast<::sockaddr*>(&remote), &addr_len);
    if (n == -1) Throw("recvfrom");
    if (from) *from = remote;
    return static_cast<std::size_t>(n);
  }

  DatagramSocket(const DatagramSocket&)            = delete;
  DatagramSocket& operator=(const DatagramSocket&) = delete;
  DatagramSocket(DatagramSocket&&)                 = delete;
  DatagramSocket& operator=(DatagramSocket&&)      = delete;
}; // DatagramSocket

} // tjg

#include <unordered_map>
#include <iostream>
#include <atomic>
#include <csignal>

static std::atomic<bool> TerminateFlag{false};
static void HandleSignal(int) { TerminateFlag = true; }

int main() {
  std::signal(SIGINT,  HandleSignal); // Ctrl-C
  std::signal(SIGTERM, HandleSignal); // kill

  // Count datagrams per source IP (network-order key)
  auto counts = std::unordered_map<tjg::NetIpV4, std::size_t>{};
  // If you want the IP addresses sorted then use... (may byte-swap)
  // `auto counts = std::map<std::uint32_t, std::size_t>{};`

  try {
    // This is the only line that byte-swaps on a little-endian machine.
    auto sock = tjg::DatagramSocket(tjg::SockAddrIn{tjg::NetPort{9999}});
    auto buf  = std::array<char, 1500>{};
    auto peer = tjg::SockAddrIn{};
    while (!TerminateFlag) {
      (void) sock.recvfrom(std::span{buf}, &peer);
      ++counts[peer.addr()];
    }
  }
  catch (const std::exception& x) {
    std::cerr << "\nException: " << x.what() << '\n';
  }

  // Display count of messages received from each peer.
  // May byte-swap if `counts` is an `std::map`.
  for (const auto& [addr, count] : counts)
    std::cout << tjg::InetPath(tjg::NetIpV4{addr}) << '\t' << count << '\n';
} // main
```
