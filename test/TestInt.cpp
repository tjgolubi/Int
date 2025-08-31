/// @file
/// @copyright 2025 Terry Golubiewski, all rights reserved.
/// @author Terry Golubiewski

// TestInt.cpp — consolidated runtime tests for tjg::Int<T,E>
// Covers containers, algorithms, memcpy/ptr(), endian round-trips, and
// randomized properties.
// NOTE: Compile-/SFINAE-only checks live in IntConv.cpp; this file avoids
// duplicate compile-time assertions and focuses on runtime behavior.
//
// Build: link with GoogleTest and pthread.
//  g++ -std=c++23 -O2 -I. TestInt.cpp -lgtest -lgtest_main -lpthread -o TestInt

#include "Int.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <random>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <compare> // for std::strong_ordering

namespace tjg_test {

using std::endian;
using tjg::Int;

// ---- Test parameter carrier ----
template <class T_, endian E_>
struct P {
  using T = T_;
  static constexpr endian E = E_;
  using I  = Int<T,E>;
  using IN = Int<T, endian::native>;
  using IO = Int<T,~endian::native>;
};

template <class P> class IntRT : public ::testing::Test {};

using Cases = ::testing::Types<
  P<std::uint8_t,   endian::native>,
  P<std::int8_t,    endian::native>,
  P<std::uint16_t,  endian::native>,
  P<std::int16_t,   endian::native>,
  P<std::uint32_t,  endian::native>,
  P<std::int32_t,   endian::native>,
  P<std::uint64_t,  endian::native>,
  P<std::int64_t,   endian::native>,
  P<std::uint32_t, ~endian::native>,
  P<std::int32_t,  ~endian::native>,
  P<std::uint64_t, ~endian::native>,
  P<std::int64_t,  ~endian::native>
>;
TYPED_TEST_SUITE(IntRT, Cases);

// ------ Construction/value()/big()/little()/raw() ----------
TYPED_TEST(IntRT, ConstructionAndValue) {
  using P = TypeParam;
  using T = P::T;
  using I = P::I;

  constexpr T a = T{42};
  I x{a};
  EXPECT_EQ(static_cast<T>(x), a);
  EXPECT_EQ(x.value(), a);
  if constexpr (P::E == endian::native) {
    EXPECT_EQ(x.raw(), a);
  } else {
    EXPECT_EQ(x.raw(), std::byteswap(a));
  }
  constexpr T b = static_cast<T>(0x123456789abcdef0);
  I y{b};
  T big;
  T lil;
  if constexpr (std::endian::native == std::endian::big) {
    big = b;
    lil = std::byteswap(b);
  } else {
    lil = b;
    big = std::byteswap(b);
  }
  EXPECT_EQ(y.value() , b  );
  EXPECT_EQ(y.little(), lil);
  EXPECT_EQ(y.big()   , big);
}

// ---------- Equality/ordering is numeric ----------
TYPED_TEST(IntRT, EqualityVsOrdering) {
  using P = TypeParam;
  using T = P::T;
  using I = P::I;

  I a{T{7}}, b{T{7}}, c{T{5}};
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a == c);

  using J = Int<T, ~P::E>;
  EXPECT_TRUE(I{T{7}} == J{T{7}}); // equal value across endianness

  // Return-type check for <=> (should match integral's strong_ordering)
  static_assert(std::is_same_v<decltype(I{T{1}} <=> I{T{2}}), std::strong_ordering>);

  EXPECT_TRUE(std::is_lt(I{T{1}} <=> I{T{2}}));
  EXPECT_TRUE(std::is_gt(I{T{3}} <=> I{T{2}}));
  EXPECT_TRUE(std::is_eq(I{T{2}} <=> I{T{2}}));
}

// ---------- Arithmetic with T and Int returns built-in promoted type ----------
TYPED_TEST(IntRT, ArithmeticBasics) {
  using P = TypeParam;
  using T = P::T;
  using I = P::I;
  using R = decltype(std::declval<T>() + std::declval<T>()); // built-in promotion

  I x{T{10}}; T y{3};
  auto p = x + y;
  auto m = x - y;
  auto mul = x * y;
  auto d = x / y;
  auto r = x % y;

  static_assert(std::is_same_v<decltype(p),   R>);
  static_assert(std::is_same_v<decltype(m),   R>);
  static_assert(std::is_same_v<decltype(mul), R>);
  static_assert(std::is_same_v<decltype(d),   R>);
  static_assert(std::is_same_v<decltype(r),   R>);

  EXPECT_EQ(p, 13);
  EXPECT_EQ(m,  7);
  EXPECT_EQ(mul,30);
  EXPECT_EQ(d,  10 / 3);
  EXPECT_EQ(r,  10 % 3);

  I a{T{11}}, b{T{2}};
  auto p2 = a + b;
  auto m2 = a - b;
  auto mul2 = a * b;

  static_assert(std::is_same_v<decltype(p2),   R>);
  static_assert(std::is_same_v<decltype(m2),   R>);
  static_assert(std::is_same_v<decltype(mul2), R>);

  EXPECT_EQ(p2, 13);
  EXPECT_EQ(m2,  9);
  EXPECT_EQ(mul2,22);
}

// ---------- Shifts ----------
TYPED_TEST(IntRT, Shifts) {
  using P = TypeParam;
  using T = P::T;
  using I = P::I;
  using R = decltype(std::declval<T>() << std::declval<T>());

  I a{T{2}};
  auto l = a << T{2};
  auto r = a >> T{1};

  static_assert(std::is_same_v<decltype(l), R>);
  static_assert(std::is_same_v<decltype(r), R>);

  EXPECT_EQ(l, 8);
  EXPECT_EQ(r, 1);

  I s{T{8}};
  auto l2 = s << I{T{1}};
  auto r2 = s >> I{T{1}};

  static_assert(std::is_same_v<decltype(l2), R>);
  static_assert(std::is_same_v<decltype(r2), R>);

  EXPECT_EQ(l2, 16);
  EXPECT_EQ(r2,  4);
}

// ---------- Bitwise Int ⊕ Int (no swap path) ----------
TYPED_TEST(IntRT, BitwiseIntInt) {
  using P = TypeParam;
  using T = P::T;
  using I = P::I;

  I a{static_cast<T>(0xF0)}, b{static_cast<T>(0x3F)};
  auto o = (a | b);
  auto n = (a & b);
  auto x = (a ^ b);

  static_assert(std::is_same_v<decltype(o), I>);
  static_assert(std::is_same_v<decltype(n), I>);
  static_assert(std::is_same_v<decltype(x), I>);

  EXPECT_EQ(o, static_cast<T>(0xFF));
  EXPECT_EQ(n, static_cast<T>(0x30));
  EXPECT_EQ(x, static_cast<T>(0xCF));
}

// ---------- Bitwise Int ⊕ T / T ⊕ Int (promote to native) ----------
TYPED_TEST(IntRT, BitwiseMixed) {
  using P = TypeParam;
  using T = P::T;
  using I = P::I;
  using R = decltype(std::declval<T>() | std::declval<T>());

  I a{static_cast<T>(0xF0)}; T t{static_cast<T>(0x0F)};
  auto o1 = a | t;
  auto n1 = a & t;
  auto x1 = a ^ t;

  auto o2 = t | a;
  auto n2 = t & a;
  auto x2 = t ^ a;

  static_assert(std::is_same_v<decltype(o1), R>);
  static_assert(std::is_same_v<decltype(n1), R>);
  static_assert(std::is_same_v<decltype(x1), R>);
  static_assert(std::is_same_v<decltype(o2), R>);
  static_assert(std::is_same_v<decltype(n2), R>);
  static_assert(std::is_same_v<decltype(x2), R>);

  EXPECT_EQ(o1, (T{a} | t));
  EXPECT_EQ(n1, (T{a} & t));
  EXPECT_EQ(x1, (T{a} ^ t));
  EXPECT_EQ(o2, (t | T{a}));
  EXPECT_EQ(n2, (t & T{a}));
  EXPECT_EQ(x2, (t ^ T{a}));
}

// ---------- Unary / bool ----------
TYPED_TEST(IntRT, UnaryOps) {
  using P = TypeParam;
  using T = P::T;
  using I = P::I;
  using NegR = decltype(-std::declval<T>());

  I a{T{5}};
  auto pos  = +a;
  auto neg  = -a;
  auto notb = ~a;

  static_assert(std::is_same_v<decltype(pos),  I>);
  static_assert(std::is_same_v<decltype(neg),  NegR>);
  static_assert(std::is_same_v<decltype(notb), I>);

  (void)pos; (void)neg; (void)notb;

  EXPECT_TRUE(static_cast<bool>(a));
  EXPECT_FALSE(static_cast<bool>(I{T{0}}));
}

// ---------- Pre/Post inc/dec ----------
TYPED_TEST(IntRT, IncDec) {
  using P = TypeParam;
  using T = P::T;
  using I = P::I;

  I a{T{5}};

  // Return-type checks
  static_assert(std::is_same_v<decltype(++std::declval<I&>()), I&>);
  static_assert(std::is_same_v<decltype(--std::declval<I&>()), I&>);
  static_assert(std::is_same_v<decltype(std::declval<I&>()++), T>);
  static_assert(std::is_same_v<decltype(std::declval<I&>()--), T>);

  auto preInc = ++a; (void)preInc;
  EXPECT_EQ(static_cast<T>(a), T(6));
  auto postInc = a++; (void)postInc;
  EXPECT_EQ(static_cast<T>(a), T(7));
  auto preDec = --a; (void)preDec;
  EXPECT_EQ(static_cast<T>(a), T(6));
  auto postDec = a--; (void)postDec;
  EXPECT_EQ(static_cast<T>(a), T(5));
}

// ---------- ptr(): only when E == native ----------
TYPED_TEST(IntRT, NativePtrBehavior) {
  using P = TypeParam;
  using T = P::T;
  using I = P::I;

  if constexpr (P::E == endian::native) {
    I x{T{42}};
    auto p = x.ptr();
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p, std::addressof(x.raw()));
    *p = T{99};
    EXPECT_EQ(x.value(), T{99});
  } else {
    SUCCEED();
  }
}

// ---------- Containers: hash/equality coherence ----------
TYPED_TEST(IntRT, UnorderedMapSetRoundTrip) {
  using P = TypeParam;
  using T = P::T;
  using I = P::I;

  std::unordered_map<I, int> m;
  std::unordered_set<I> s;
  for (int i = 0; i < 100; ++i) {
    I key{T(i)};
    m.emplace(key, i * 3);
    s.emplace(key);
  }
  for (int i = 0; i < 100; ++i) {
    I key{T(i)};
    auto it = m.find(key);
    ASSERT_NE(it, m.end());
    EXPECT_EQ(it->second, i * 3);
    EXPECT_EQ(s.count(key), 1u);
  }

  using J = Int<T, ~P::E>;
  for (int i = 0; i < 5; ++i) {
    J other{T(i)};
    EXPECT_EQ(s.count(I{other}), 1u);
  }
}

// ---------- Cross-endian numeric equality; raw difference for multi-byte ----------
TYPED_TEST(IntRT, CrossEndianValueEquality) {
  using P = TypeParam;
  using T = P::T;
  using I = P::I;
  using J = Int<T, ~P::E>;

  for (int i = 1; i <= 5; ++i) {
    I a{T(i)};
    J b{T(i)};
    EXPECT_TRUE(a == b);
    EXPECT_EQ(a.raw(), std::byteswap(b.raw()));
  }
}

// ---------- EndianCast bridge for lookups ----------
TYPED_TEST(IntRT, EndianCastBridgedLookup) {
  using P = TypeParam;
  using T = P::T;
  using I = P::I;
  using J = Int<T, ~P::E>;

  std::unordered_set<I> s;
  for (int i = 1; i <= 100; ++i) s.emplace(I{T(i)});

  for (int i = 1; i <= 5; ++i) {
    J other{T(i)};
    I bridged = tjg::endian_cast<P::E>(other);
    EXPECT_EQ(s.count(bridged), 1u);
  }
}

// ---------- Algorithms: sorting by numeric order ----------
TYPED_TEST(IntRT, SortingNumeric) {
  using P = TypeParam;
  using T = P::T;
  using I = P::I;

  std::vector<I> v;
  for (int i = 32; i >= 0; --i) v.emplace_back(T(i));
  std::stable_sort(v.begin(), v.end(), [](const I& a, const I& b){
    return std::is_lt(a <=> b);
  });
  for (int i = 0; i <= 32; ++i) {
    ASSERT_EQ(static_cast<T>(v[static_cast<size_t>(i)]), T(i));
  }
}

// ---------- Memops: memcpy round-trip ----------
TYPED_TEST(IntRT, MemcpyRoundTrip) {
  using P = TypeParam;
  using T = P::T;
  using I = P::I;

  std::vector<I> src;
  for (int i = 0; i < 64; ++i) src.emplace_back(T(i * 7 + 3));

  std::vector<I> dst(src.size());
  std::memcpy(dst.data(), src.data(), src.size() * sizeof(I));

  ASSERT_EQ(dst.size(), src.size());
  for (size_t i = 0; i < dst.size(); ++i) {
    EXPECT_EQ(dst[i].raw(), src[i].raw());
    EXPECT_EQ(static_cast<T>(dst[i]), static_cast<T>(src[i]));
  }
}

// ---------- Endian round-trips ----------
TYPED_TEST(IntRT, EndianRoundTrips) {
  using P = TypeParam;
  using T = P::T;
  using I = P::I;
  using IN = P::IN;

  I a{T{123}};
  auto b = tjg::byteswap(a);
  auto c = tjg::byteswap(b);
  EXPECT_EQ(static_cast<T>(c), static_cast<T>(a));

  auto d = tjg::endian_cast<endian::native>(a);
  auto e = tjg::endian_cast<P::E>(d);
  EXPECT_EQ(static_cast<T>(e), static_cast<T>(a));
}

// ---------- Randomized properties ----------
TYPED_TEST(IntRT, RandomizedProperties) {
  using P = TypeParam;
  using T = P::T;
  using I = P::I;

  std::mt19937_64 rng(0xC0FFEE1234ULL);
  auto randT = [&]{
    if constexpr (std::is_signed_v<T>) {
      using U = std::make_unsigned_t<T>;
      U u = static_cast<U>(rng());
      return static_cast<T>(u);
    } else {
      using U = T;
      return static_cast<T>(static_cast<U>(rng()));
    }
  };

  constexpr int N = 2000;
  for (int i = 0; i < N; ++i) {
    T a = randT();
    T b = randT();
    auto w = std::numeric_limits<std::make_unsigned_t<T>>::digits;
    int sc = w ? static_cast<int>(rng() % w) : 0;
    T nb = (b == T{0}) ? T{1} : b;

    const I xa{a};

    // XOR property: (x ^ y) ^ y == x
    auto tmp1 = xa ^ b;
    auto tmp2 = tmp1 ^ b;
    EXPECT_EQ(static_cast<T>(tmp2), static_cast<T>(xa));

    // Addition inverse: (x + y) - y == x
    auto s  = xa + b;
    auto s2 = s - b;
    EXPECT_EQ(static_cast<T>(s2), static_cast<T>(xa));

    // Byteswap involutive
    auto sw1 = tjg::byteswap(xa);
    auto sw2 = tjg::byteswap(sw1);
    EXPECT_EQ(static_cast<T>(sw2), static_cast<T>(xa));

    // Logical-shift round-trip using unsigned
    // Shift round-trip exactly like T (including any UB that T would have).
    // We sanitize sc to be < width above, just like for T.
    auto rt = (xa << sc) >> sc;                // Int path
    auto eq = (static_cast<T>(a) << sc) >> sc; // T path
    EXPECT_EQ(rt, eq);

    // Inc/Dec cancels
    I xi{a};
    ++xi; xi--;
    EXPECT_EQ(static_cast<T>(xi), a);

    // Division/mod identity
    auto q = xa / nb;
    auto r = xa % nb;
    EXPECT_EQ(static_cast<T>(q) * nb + static_cast<T>(r), a);
  }
}

} // tjg_test
