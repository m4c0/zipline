module;
#include <array>
#include <cassert>
#include <exception>
#include <optional>

export module bitstream;
import yoyo;

export namespace zipline {
struct truncated_stream : std::exception {};

class bitstream {
  static constexpr const auto max_bits_at_once = 8;
  static constexpr const auto bits_per_byte = 8U;

  yoyo::reader *m_reader;

  unsigned m_rem{};
  unsigned m_buf{};

public:
  explicit constexpr bitstream(yoyo::reader *r) : m_reader{r} {}

  [[nodiscard]] constexpr auto next(unsigned n) {
    assert(n <= max_bits_at_once);

    if (m_rem < n) {
      auto next = m_reader->read_u8();
      if (!next)
        throw truncated_stream{};
      m_buf = m_buf + (*next << m_rem);
      m_rem += bits_per_byte;
    }

    auto res = m_buf & ((1U << n) - 1U);
    m_rem -= n;
    m_buf >>= n;
    return res;
  }

  template <size_t N>
    requires(N <= max_bits_at_once)
  [[nodiscard]] constexpr auto next() {
    if (m_rem < N) {
      auto next = m_reader->read_u8();
      if (!next)
        throw truncated_stream{};
      m_buf = m_buf + (*next << m_rem);
      m_rem += bits_per_byte;
    }

    auto res = m_buf & ((1U << N) - 1U);
    m_rem -= N;
    m_buf >>= N;
    return res;
  }

  template <size_t N> constexpr void skip() {
    auto rem = N;
    while (rem >= max_bits_at_once) {
      auto r = next<max_bits_at_once>();
      rem -= max_bits_at_once;
    }
    auto r = next<N % max_bits_at_once>();
  }

  [[nodiscard]] constexpr auto eof() const noexcept {
    return m_reader->eof() && m_rem == 0;
  }
};

template <typename Reader> class ce_bitstream : public bitstream {
  Reader m_real_reader;

public:
  explicit constexpr ce_bitstream(Reader r)
      : bitstream{&m_real_reader}, m_real_reader{r} {};
};
} // namespace zipline

using namespace zipline;

static constexpr const yoyo::ce_reader data{0x8d, 0x52, 0x4d};

static_assert([] {
  auto r = data;
  bitstream b{&r};
  if (b.next<1>() != 1)
    return false;
  if (b.next<2>() != 0b10)
    return false; // NOLINT
  if (b.next<5>() != 17)
    return false; // NOLINT
  if (b.next<5>() != 18)
    return false; // NOLINT
  if (b.next<4>() != 10)
    return false; // NOLINT
  if (b.next<3>() != 6)
    return false; // NOLINT
  if (b.next<3>() != 4)
    return false; // NOLINT
  return true;
}());
// Skip nothing from beginning
static_assert([] {
  auto r = data;
  bitstream b{&r};
  b.skip<0>();
  return b.next<1>() == 1;
}());
// Skip nothing from somewhere
static_assert([] {
  auto r = data;
  bitstream b{&r};
  if (b.next<1>() != 1)
    return false;
  b.skip<2>();
  return b.next<5>() == 17; // NOLINT
}());
// Skip from beginning
static_assert([] {
  auto r = data;
  bitstream b{&r};
  b.skip<3>();
  return b.next<5>() == 17; // NOLINT
}());
static_assert([] {
  constexpr const yoyo::ce_reader data{0x8d, 0x52, 0x4d};
  constexpr const auto bits_to_skip = 1 + 2 + 5 + 5 + 4;
  auto r = data;
  bitstream b{&r};
  b.skip<bits_to_skip>();
  return b.next<3>() == 6 && b.next<3>() == 4; // NOLINT
}());
static_assert([] {
  auto r = data;
  bitstream b{&r};
  return b.next(1) == 1 && b.next(2) == 2 && b.next(5) == 17;
}());
