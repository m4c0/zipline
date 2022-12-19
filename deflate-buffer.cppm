module;
#include <array>
#include <optional>
#include <string_view>

export module deflate:buffer;
import :symbols;

export namespace zipline {
class buffer {
  static constexpr const auto buf_size = 32768;

  std::array<uint8_t, buf_size> m_buf{};
  unsigned m_rd{};
  unsigned m_wr{};

  [[nodiscard]] static constexpr auto wrap(unsigned n, unsigned d) noexcept {
    return (n + d) % buf_size;
  }
  constexpr void put(uint8_t c) noexcept {
    m_buf.at(m_wr) = c;
    m_wr = wrap(m_wr, 1);
  }

public:
  [[nodiscard]] constexpr bool empty() const noexcept { return m_rd == m_wr; }
  [[nodiscard]] constexpr uint8_t read() noexcept {
    auto p = m_rd;
    m_rd = wrap(m_rd, 1);
    return m_buf.at(p);
  }

  constexpr bool operator()(symbols::raw r) noexcept {
    put(r.c);
    return true;
  }
  constexpr bool operator()(symbols::repeat r) noexcept {
    for (unsigned i = 0; i < r.len; i++) {
      put(m_buf.at(wrap(m_wr, -r.dist)));
    }
    m_rd = wrap(m_wr, -r.len);
    return true;
  }
  constexpr bool operator()(symbols::end /**/) noexcept { return false; }
};
} // namespace zipline

using namespace zipline;

static_assert(buffer{}.empty());

static constexpr auto visit(const symbols::symbol &s) noexcept {
  buffer b{};
  return std::visit(b, s) ? std::optional{b} : std::nullopt;
}
static_assert(!visit(symbols::end{}));
static_assert(visit(symbols::raw{'y'}));

static constexpr auto visit_and_read(buffer &b, uint8_t c) {
  if (!b(symbols::raw{c}))
    return false;
  if (b.empty())
    return false;
  if (b.read() != c)
    return false;
  return b.empty();
}
static constexpr auto visit_and_read(buffer &b, std::string_view str,
                                     unsigned dist) {
  if (!b(symbols::repeat{static_cast<unsigned int>(str.length()), dist}))
    return false;
  for (uint8_t c : str) {
    if (b.empty())
      return false;
    if (b.read() != c)
      return false;
  }
  return b.empty();
}

static_assert([] {
  buffer b{};
  if (!visit_and_read(b, 'y'))
    return false;
  if (!visit_and_read(b, 'u'))
    return false;
  if (!visit_and_read(b, 'p'))
    return false;
  return true;
}());
static_assert([] {
  buffer b{};
  if (!visit_and_read(b, 't'))
    return false;
  if (!visit_and_read(b, 'e'))
    return false;
  if (!visit_and_read(b, 's'))
    return false;
  if (!visit_and_read(b, "te", 3))
    return false;
  return true;
}());
static_assert([] {
  buffer b{};
  if (!visit_and_read(b, 't'))
    return false;
  if (!visit_and_read(b, 'e'))
    return false;
  if (!visit_and_read(b, 's'))
    return false;
  if (!visit_and_read(b, "te", 3))
    return false;
  if (!visit_and_read(b, ' '))
    return false;
  if (!visit_and_read(b, "test", 6))
    return false; // NOLINT
  return true;
}());
