// int_conv_onefile.cpp
// Compile one scenario at a time by defining exactly one macro via -D.
// If none or more than one are defined, we #error to catch it.

#include "Int.hpp"  // your tjg::Int header
#include <type_traits>
#include <bit>
#include <cstdint>

[[maybe_unused]] constexpr std::endian ENative    = std::endian::native;
[[maybe_unused]] constexpr std::endian ENonNative = ~ENative;

// ------------------- Scenarios -------------------

// 1) PASS: non-narrowing Int<U,E2> -> Int<V,E1> direct-init compiles (explicit).
#ifdef PASS_INT_TO_INT_CONSTRUCT
using FromT = std::uint16_t;
using ToT   = std::uint32_t;

int main() {
  using FromNat    = tjg::Int<FromT, ENative>;
  using FromNonNat = tjg::Int<FromT, ENonNative>;
  using ToNat      = tjg::Int<ToT,   ENative>;
  using ToNonNat   = tjg::Int<ToT,   ENonNative>;

  FromNat    a{FromT{0x1234}};
  FromNonNat b{FromT{0x5678}};

  // Direct-init is OK (constructor is explicit for different T).
  ToNat    x1{a};
  ToNat    x2{b};
  ToNonNat y1{a};
  ToNonNat y2{b};

  (void)x1; (void)x2; (void)y1; (void)y2;
}
#endif

// 2a) PASS: narrowing Int<U,E2> -> Int<V,E1> constructible with Int().
#ifdef PASS_INT_TO_INT_NARROW_PAREN
using FromT = std::uint32_t;
using ToT   = std::uint16_t;

int main() {
  using FromNat = tjg::Int<FromT, ENative>;
  using ToNat   = tjg::Int<ToT,   ENative>;

  FromNat a{FromT{0x12345678}};
  ToNat bad(a); // should compile
  (void)bad;
}
#endif

// 2b) FAIL: narrowing Int<U,E2> -> Int<V,E1> NOT constructible with Int{}.
#ifdef FAIL_INT_TO_INT_NARROW_BRACE
using FromT = std::uint32_t;
using ToT   = std::uint16_t;

int main() {
  using FromNat = tjg::Int<FromT, ENative>;
  using ToNat   = tjg::Int<ToT,   ENative>;

  FromNat a{FromT{0x12345678}};
  // This must be ill-formed (narrowing).
  ToNat bad{a}; // should not compile
  (void)bad;
}
#endif

// 3) FAIL: implicit copy-init on widening different-T should be ill-formed (ctor is explicit).
#ifdef FAIL_IMPLICIT_WIDEN_NATIVE
using FromT = std::uint16_t;
using ToT   = std::uint32_t;

int main() {
  using FromNat = tjg::Int<FromT, ENative>;
  using ToNat   = tjg::Int<ToT,   ENative>;

  FromNat a{FromT{123}};
  ToNat bad = a; // should fail: different T, ctor is explicit
  (void)bad;
}
#endif

// 4) FAIL: implicit copy-init on widening to non-native endian target.
#ifdef FAIL_IMPLICIT_WIDEN_NONNATIVE
using FromT = std::uint16_t;
using ToT   = std::uint32_t;

int main() {
  using FromNat  = tjg::Int<FromT, ENative>;
  using ToNonNat = tjg::Int<ToT,   ENonNative>;

  FromNat a{FromT{123}};
  ToNonNat bad = a; // should fail: different T, ctor is explicit
  (void)bad;
}
#endif

// 5) PASS: implicit copy-init is OK when T is the same.
#ifdef PASS_SAME_T_IMPLICIT_NONNATIVE_EXPLICIT
using T = std::uint32_t;

int main() {
  using FromNat = tjg::Int<T, ENative>;
  using FromNon = tjg::Int<T, ENonNative>;
  using ToNat   = tjg::Int<T, ENative>;
  using ToNon   = tjg::Int<T, ENonNative>;

  FromNat a{T{1234}};
  ToNat   x = a;        // OK: same T, same endian
  ToNon   y = ToNon{a}; // OK: same T, explicit cross-endian
  FromNon b{T{5678}};
  ToNat   w = ToNat{b}; // OK: same T, explicit cross-endian
  ToNon   z = b;        // OK: same T, non-native, no conversion

  (void)x; (void)y; (void)w; (void)z;
}
#endif

// 6) FAIL: same-T implicit cross-endian convert must be explicit.
#ifdef FAIL_SAME_T_IMPLICIT_NONNATIVE_IMPLICIT
using T = std::uint32_t;

int main() {
  using FromNat = tjg::Int<T, ENative>;
  using ToNat   = tjg::Int<T, ENative>;
  using ToNon   = tjg::Int<T, ENonNative>;

  FromNat a{T{1234}};
  ToNat   x = a; // OK
  ToNon   y = a; // FAIL: implicit cross-endian

  (void)x; (void)y;
}
#endif

// 7) PASS: assignment operator handles widening from Int<U,E2> to Int<T,E1>.
#ifdef PASS_ASSIGN_WIDEN_FROM_INT
using FromT = std::uint16_t;
using ToT   = std::uint32_t;

int main() {
  using FromNat = tjg::Int<FromT, ENative>;
  using FromNon = tjg::Int<FromT, ENonNative>;
  using ToNat   = tjg::Int<ToT,   ENative>;

  FromNat a{FromT{321}};
  FromNon b{FromT{654}};

  ToNat dest{};
  dest = a; // OK widening assign
  dest = b; // OK widening assign (cross-endian)

  (void)dest;
}
#endif

// 8) FAIL: assignment from a wider Int to a narrower Int must be ill-formed.
#ifdef FAIL_ASSIGN_NARROW_FROM_INT
using FromT = std::uint32_t;
using ToT   = std::uint16_t;

int main() {
  using FromNat = tjg::Int<FromT, ENative>;
  using ToNat   = tjg::Int<ToT,   ENative>;

  FromNat a{FromT{0x12345678}};
  ToNat dest{};
  dest = a; // should fail (narrowing)
  (void)dest;
}
#endif

// 8a) PASS: assignment from a wider Int to a narrower Int solutions
#ifdef PASS_ASSIGN_NARROW_FROM_INT
using FromT = std::uint32_t;
using ToT   = std::uint16_t;

int main() {
  using FromNat = tjg::Int<FromT, ENative>;
  using ToNat   = tjg::Int<ToT,   ENative>;

  FromNat a{FromT{0x12345678}};
  ToNat dest{};
  dest = tjg::narrow_cast<std::uint16_t>(a);
  dest = static_cast<std::uint16_t>(a);
  (void)dest;
}
#endif

// 9) PASS: scalar widening U -> Int<T,E> must compile.
#ifdef PASS_SCALAR_IN_WIDEN
using FromT = std::uint16_t;
using ToT   = std::uint32_t;

int main() {
  using ToNat = tjg::Int<ToT, ENative>;
  using ToNon = tjg::Int<ToT, ENonNative>;

  FromT v{123};
  ToNat a(v);
  ToNon b(v);
  (void)a; (void)b;
}
#endif

// 10a) PASS: scalar narrowing U -> Int<T,E> must be ill-formed.
#ifdef PASS_SCALAR_IN_NARROW_PAREN
using FromT = std::uint32_t;
using ToT   = std::uint16_t;

int main() {
  using ToNat = tjg::Int<ToT, ENative>;
  FromT v{0x12345678};
  ToNat bad(v); // should compile
  (void)bad;
}
#endif

// 10b) FAIL: scalar narrowing U -> Int<T,E> must be ill-formed.
#ifdef FAIL_SCALAR_IN_NARROW_BRACE
using FromT = std::uint32_t;
using ToT   = std::uint16_t;

int main() {
  using ToNat = tjg::Int<ToT, ENative>;
  FromT v{0x12345678};
  ToNat bad{v}; // should fail
  (void)bad;
}
#endif

// 10c) PASS: assignment operator handles widening from U to T.
#ifdef PASS_ASSIGN_WIDEN_FROM_SCALAR
using FromT = std::uint16_t;
using ToT   = std::uint32_t;

int main() {
  using FromNat = tjg::Int<FromT, ENative>;
  using FromNon = tjg::Int<FromT, ENonNative>;
  using ToNat   = tjg::Int<ToT,   ENative>;

  FromNat a{FromT{0x1234}};
  FromNon b{FromT{0x8765}};

  ToNat dest{};
  dest = a; // ok widening assign
  dest = b; // ok widening assign (cross-endian)

  (void)dest;
}
#endif

// 10d) FAIL: assignment operator rejects narrowing U to T.
#ifdef FAIL_ASSIGN_NARROW_FROM_SCALAR
using FromT = std::uint32_t;
using ToT   = std::uint16_t;

int main() {
  using FromNat = tjg::Int<FromT, ENative>;
  using FromNon = tjg::Int<FromT, ENonNative>;
  using ToNat   = tjg::Int<ToT,   ENative>;

  FromNat a{FromT{0x12345678}};
  FromNon b{FromT{0x87654321}};

  ToNat dest{};
  dest = a; // FAIL narrowing assign
  dest = b; // FAIL narrowing assign (cross-endian)

  (void)dest;
}
#endif

// 11) PASS: Int<T,E> -> scalar U (via operator T()).
#ifdef PASS_SCALAR_OUT
using FromT = std::uint16_t;
using ToT   = std::uint32_t;

int main() {
  using FromNat = tjg::Int<FromT, ENative>;
  using FromNon = tjg::Int<FromT, ENonNative>;
  FromNat a{FromT{0x1234}};
  FromNon b{FromT{0x5678}};

  ToT u1 = a; // widening, allowed
  ToT u2 = b; // widening, allowed

  (void)u1; (void)u2;
}
#endif
