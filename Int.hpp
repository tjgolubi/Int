/// @file
/// @copyright 2025 Terry Golubiewski, all rights reserved.
/// @author Terry Golubiewski

#pragma once
#include <concepts>   // std::integral
#include <functional> // std::hash
#include <type_traits>// std::is_trivially_copyable_t
#include <compare>    // operator<=>
#include <bit>        // std::endian, std::byteswap
#include <memory>     // std::addressof
#include <utility>    // std::declval
#include <cstddef>    // std::size_t

namespace std {

constexpr endian operator~(endian x) noexcept {
  if (x == endian::little)
    return endian::big;
  else
    return endian::little;
}

} // std

namespace tjg {

template<typename To, typename From>
constexpr To narrow_cast(From x) noexcept { return static_cast<To>(x); }

template<class From, class To>
concept NonNarrowing = std::same_as<From, To> || (std::convertible_to<From, To>
              && requires (From x) { To{ x }; }); // list-init rejects narrowing

template<std::integral T=int, std::endian E = std::endian::native>
requires (std::same_as<T, std::remove_cv_t<T>> && !std::is_reference_v<T>)
class Int {
public:
  using value_type = T;
  static constexpr std::endian Endian = E;

private:
  value_type _raw = value_type{0};

  template<std::endian Rep>
  constexpr T _get() const noexcept {
    if constexpr (Endian == Rep)
      return _raw;
    else
      return std::byteswap(_raw);
  }

  constexpr void _set(T x) noexcept {
    if constexpr (Endian == std::endian::native)
      _raw = x;
    else
      _raw = std::byteswap(x);
  }

public:
  constexpr Int() noexcept = default;
  constexpr Int(const Int&) noexcept = default;
  constexpr Int& operator=(const Int&) = default;
  constexpr bool operator==(const Int&) const = default;

  constexpr explicit Int(T x) noexcept { _set(x); }

  // Allow Only non-narrowing assignment.
  template<std::integral U> requires NonNarrowing<U, T>
  constexpr Int& operator=(U x) noexcept { _set(T{x}); return *this; }

  template<std::integral U, std::endian E2> requires NonNarrowing<U, T>
  constexpr Int& operator=(Int<U, E2> x) noexcept
    { _set(T{U{x}}); return *this; }

  // Specifically delete narrowing assignment.
  template<std::integral U> requires (!NonNarrowing<U, T>)
  constexpr Int& operator=(U x) noexcept = delete;

  template<std::integral U, std::endian E2> requires (!NonNarrowing<U, T>)
  constexpr Int& operator=(Int<U, E2> x) noexcept = delete;

  // Accessors.
  constexpr       T& raw()       noexcept { return _raw;   }
  constexpr const T& raw() const noexcept { return _raw;   }

  constexpr       T* ptr()       noexcept
    requires (Endian == std::endian::native)
    { return std::addressof(_raw); }

  constexpr const T* ptr() const noexcept
    requires (Endian == std::endian::native)
    { return std::addressof(_raw); }

  [[nodiscard]] constexpr T value() const noexcept
    { return _get<std::endian::native>(); }

  [[nodiscard]] constexpr T big() const noexcept
    { return _get<std::endian::big>(); }

  [[nodiscard]] constexpr T little() const noexcept
    { return _get<std::endian::little>(); }

  constexpr operator T() const noexcept { return value(); }

  // Compares numerically, operator== compares raw storage, should work.
  constexpr auto operator<=>(const Int& rhs) const noexcept
    { return (value() <=> rhs.value()); }

  // Unary operations.
  constexpr Int operator+() const noexcept { return *this; }
  constexpr auto operator-() const noexcept { return -value(); }
  constexpr Int operator~()  const noexcept
    { Int x; x._raw = narrow_cast<T>(~_raw); return x; }
  constexpr bool operator!() const noexcept { return !_raw; }
  constexpr explicit operator bool() const noexcept { return bool(_raw); }

  // Increment/decrement
  constexpr Int& operator++() noexcept
    { return *this = static_cast<T>(value() + 1); }

  constexpr Int& operator--() noexcept
    { return *this = static_cast<T>(value() - 1); }

  constexpr T operator++(int) noexcept {
    T prev = value();
    *this = static_cast<T>(prev + 1);
    return prev;
  }

  constexpr T operator--(int) noexcept {
    T prev = value();
    *this = static_cast<T>(prev - 1);
    return prev;
  }

  // Assignment operators
  template<std::integral U> requires NonNarrowing<U, T>
  constexpr Int& operator+=(U rhs) noexcept
    { return *this = narrow_cast<T>(value() + rhs); }

  template<std::integral U> requires NonNarrowing<U, T>
  constexpr Int& operator-=(U rhs) noexcept
    { return *this = narrow_cast<T>(value() - rhs); }

  template<std::integral U> requires NonNarrowing<U, T>
  constexpr Int& operator*=(U rhs) noexcept
    { return *this = narrow_cast<T>(value() * rhs); }

  template<std::integral U> requires NonNarrowing<U, T>
  constexpr Int& operator/=(U rhs) noexcept
    { return *this = narrow_cast<T>(value() / rhs); }

  template<std::integral U> requires NonNarrowing<U, T>
  constexpr Int& operator%=(U rhs) noexcept
    { return *this = narrow_cast<T>(value() % rhs); }

  constexpr Int& operator<<=(std::integral auto rhs) noexcept
    { return *this = static_cast<T>(value() << rhs); }

  constexpr Int& operator>>=(std::integral auto rhs) noexcept
    { return *this = static_cast<T>(value() >> rhs); }

  constexpr Int& operator +=(Int rhs) noexcept { return *this  += T{rhs}; }
  constexpr Int& operator -=(Int rhs) noexcept { return *this  -= T{rhs}; }
  constexpr Int& operator *=(Int rhs) noexcept { return *this  *= T{rhs}; }
  constexpr Int& operator /=(Int rhs) noexcept { return *this  /= T{rhs}; }
  constexpr Int& operator %=(Int rhs) noexcept { return *this  %= T{rhs}; }
  constexpr Int& operator<<=(Int rhs) noexcept { return *this <<= T{rhs}; }
  constexpr Int& operator>>=(Int rhs) noexcept { return *this >>= T{rhs}; }

  constexpr Int& operator|=(Int rhs) noexcept
    { _raw |= rhs._raw; return *this; }
  constexpr Int& operator&=(Int rhs) noexcept
    { _raw &= rhs._raw; return *this; }
  constexpr Int& operator^=(Int rhs) noexcept
    { _raw ^= rhs._raw; return *this; }

  template<std::integral U> requires NonNarrowing<U, T>
  constexpr Int& operator|=(U rhs) noexcept { return *this |= Int{T{rhs}}; }

  template<std::integral U> requires NonNarrowing<U, T>
  constexpr Int& operator&=(U rhs) noexcept { return *this &= Int{T{rhs}}; }

  template<std::integral U> requires NonNarrowing<U, T>
  constexpr Int& operator^=(U rhs) noexcept { return *this ^= Int{T{rhs}}; }

  // Bitwise ops, same endian, don't require byteswap, can return Int.
  constexpr Int operator|(Int rhs) const noexcept { return Int{*this} |= rhs; }
  constexpr Int operator&(Int rhs) const noexcept { return Int{*this} &= rhs; }
  constexpr Int operator^(Int rhs) const noexcept { return Int{*this} ^= rhs; }
}; // Int

// Deduce integral type from constructor argument.
template<std::integral U>
Int(U) -> Int<U, std::endian::native>;

// Deduce from another Int keeps its T and E (copy/cross-endian cases)
template<std::integral U, std::endian E>
Int(Int<U, E>) -> Int<U, E>;

// For use in test code, if you can instantiate a VerifyInt<T>, then Int<T> is
// safe to use in spans and packed messages.
template<std::integral T>
requires (std::is_standard_layout_v<Int<T>>
      && std::is_trivially_copyable_v<Int<T>>
      && sizeof (Int<T>) == sizeof(T)
      && alignof(Int<T>) == alignof(T))
struct VerifyInt { };

// For boost::hash compatibility.
template<std::integral T, std::endian E>
constexpr std::size_t hash_value(const Int<T, E>& x) noexcept
  { return static_cast<std::size_t>(x.raw()); }

template<std::endian To, std::integral T, std::endian From>
constexpr Int<T, To> endian_cast(Int<T, From> x) noexcept
  { return Int<T, To>{x}; }

template<std::integral ToT, std::integral FmT, std::endian E>
requires (!std::same_as<ToT, FmT>)
constexpr Int<ToT, std::endian::native> narrow_cast(Int<FmT, E> x) noexcept
  { return Int<ToT, std::endian::native>{narrow_cast<ToT>(x.value())}; }

template<std::integral ToT, std::integral FmT, std::endian E>
requires std::same_as<ToT, FmT>
constexpr Int<ToT, E> narrow_cast(Int<FmT, E> x) noexcept
  { return x; }

// byteswap must always reverse the physical byte order.
template<std::integral T, std::endian E>
constexpr Int<T, ~E> byteswap(Int<T, E> x) noexcept
  { return Int<T, ~E>{x}; }

template<std::signed_integral   T=int>
using BigInt  = Int<T, std::endian::big>;

template<std::signed_integral   T=int>
using LilInt  = Int<T, std::endian::little>;

template<std::unsigned_integral T=unsigned>
using BigUint = Int<T, std::endian::big>;

template<std::unsigned_integral T=unsigned>
using LilUint = Int<T, std::endian::little>;

} // tjg

namespace std {

template<integral T, endian E>
struct hash<::tjg::Int<T, E>> {
  constexpr size_t operator()(const ::tjg::Int<T, E>& x) const noexcept
    { return ::tjg::hash_value(x); }
}; // hash

} // std
