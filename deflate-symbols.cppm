module;
#include <array>
#include <optional>
#include <span>
#include <variant>

export module deflate:symbols;
import bitstream;
import huffman;
import yoyo;

namespace zipline::symbols {
struct raw {
  uint8_t c;
};
struct repeat {
  unsigned len;
  unsigned dist;
};
struct end {};
using symbol = std::variant<end, raw, repeat>;

struct bit_pair {
  unsigned bits;
  unsigned second;
};

static constexpr const auto max_lens_code = 285;
static constexpr const auto min_lens_code = 257;
[[nodiscard]] static constexpr auto build_lens_arrays() noexcept {
  constexpr const auto last = max_lens_code - min_lens_code;
  std::array<unsigned, last + 1> res{};
  for (auto i = 0U; i < 4; i++) {
    res.at(i + 4) = i + 7; // NOLINT
  }
  for (auto i = 8U; i <= last - 1; i += 4) { // NOLINT
    const auto bits = (i - 4U) / 4U;
    res.at(i) = res.at(i - 4) + (2U << bits);
    res.at(i + 1) = res.at(i) + (1U << bits);
    res.at(i + 2) = res.at(i + 1) + (1U << bits);
    res.at(i + 3) = res.at(i + 2) + (1U << bits);
  }
  return res;
}
[[nodiscard]] static constexpr auto bitlen_for_code(unsigned code) {
  constexpr const auto last_pair = bit_pair{0, 258};
  if (code == max_lens_code)
    return last_pair;

  const auto index = code - min_lens_code;
  if (index < 4)
    return bit_pair{0, index + 3};

  constexpr const auto lens = build_lens_arrays();
  const auto bits = (index - 4U) / 4U;
  return bit_pair{bits, lens.at(index)};
}
[[nodiscard]] static constexpr auto build_dist_arrays() noexcept {
  constexpr const auto last = 29;
  std::array<unsigned, last + 1> res{};
  res[2] = 3;
  res[3] = 4;
  for (auto i = 4U; i <= last - 1; i += 2) {
    const auto bits = (i - 2U) / 2U;
    res.at(i) = res.at(i - 2) + (1U << bits);
    res.at(i + 1) = res.at(i) + (1U << bits);
  }
  return res;
}
[[nodiscard]] static constexpr auto bitdist_for_code(unsigned code) noexcept {
  constexpr const auto dists = build_dist_arrays();
  if (code < 2)
    return bit_pair{code, code + 1};
  const auto bits = (code - 2U) / 2U;
  return bit_pair{bits, dists.at(code)};
}

struct huff_tables {
  huffman_codes hlist;
  huffman_codes hdist;
};
[[nodiscard]] static constexpr symbol read_next_symbol(const huff_tables &huff,
                                                       bitstream *bits) {
  constexpr const auto end_code = 256;

  const auto code = decode_huffman(huff.hlist, bits);
  if (code < end_code)
    return raw{static_cast<uint8_t>(code)};
  if (code == end_code)
    return end{};

  const auto len_bits = bitlen_for_code(code);
  const auto len = len_bits.second + bits->next(len_bits.bits);

  const auto dist_code = decode_huffman(huff.hdist, bits);
  const auto dist_bits = bitdist_for_code(dist_code);
  const auto dist = dist_bits.second + bits->next(dist_bits.bits);

  return repeat{len, dist};
}

export template <typename LenDistBits>
[[nodiscard]] constexpr huff_tables
create_tables(const LenDistBits &hlist_hdist, unsigned hlist_len) {
  std::span sp{hlist_hdist.begin(), hlist_hdist.end()};
  return huff_tables{
      .hlist = create_huffman_codes(sp.subspan(0, hlist_len)),
      .hdist = create_huffman_codes(sp.subspan(hlist_len)),
  };
}

static constexpr bool operator==(const raw &s, const raw &r) {
  return s.c == r.c;
}
static constexpr bool operator==(const repeat &s, const repeat &r) {
  return s.dist == r.dist && s.len == r.len;
}
static constexpr bool operator==(const end & /**/, const end & /**/) {
  return true;
}

static constexpr bool operator==(const bit_pair &s, const bit_pair &r) {
  return s.bits == r.bits && s.second == r.second;
}
} // namespace zipline::symbols

using namespace zipline::symbols;

// More magic number fest - All straight from RFC 1951 - sec 3.2.5
static_assert(bitlen_for_code(258) == bit_pair{0, 4});      // NOLINT
static_assert(bitlen_for_code(280) == bit_pair{4, 115});    // NOLINT
static_assert(bitlen_for_code(281) == bit_pair{5, 131});    // NOLINT
static_assert(bitlen_for_code(285) == bit_pair{0, 258});    // NOLINT
static_assert(bitdist_for_code(0) == bit_pair{0, 1});       // NOLINT
static_assert(bitdist_for_code(2) == bit_pair{0, 3});       // NOLINT
static_assert(bitdist_for_code(3) == bit_pair{0, 4});       // NOLINT
static_assert(bitdist_for_code(4) == bit_pair{1, 5});       // NOLINT
static_assert(bitdist_for_code(5) == bit_pair{1, 7});       // NOLINT
static_assert(bitdist_for_code(6) == bit_pair{2, 9});       // NOLINT
static_assert(bitdist_for_code(29) == bit_pair{13, 24577}); // NOLINT

static constexpr auto build_huffman_codes(const auto &indexes,
                                          const auto &counts) {
  zipline::huffman_codes huff{
      .counts{counts.size()},
      .indexes{indexes.size()},
  };
  std::copy(counts.begin(), counts.end(), huff.counts.begin());
  std::copy(indexes.begin(), indexes.end(), huff.indexes.begin());
  return huff;
}
static constexpr auto build_sparse_huff() {
  // 2-bit alignment to match a half-hex and simplify the assertion
  constexpr const auto lit_indexes =
      std::array<unsigned, 4>{'?', 256, 270, 285};
  constexpr const auto dist_indexes = std::array<unsigned, 4>{2, 6};

  constexpr const auto lit_counts =
      std::array<unsigned, 3>{0, 0, lit_indexes.size()};
  constexpr const auto dist_counts =
      std::array<unsigned, 3>{0, 0, dist_indexes.size()};

  return huff_tables{
      .hlist = build_huffman_codes(lit_indexes, lit_counts),
      .hdist = build_huffman_codes(dist_indexes, dist_counts),
  };
}
static constexpr auto test_read_next_symbol(uint8_t data, symbol expected) {
  auto bits = zipline::ce_bitstream{yoyo::ce_reader{data}};
  auto sym = read_next_symbol(build_sparse_huff(), &bits);
  return sym == expected;
}
static_assert(test_read_next_symbol(0b00, raw{'?'}));             // NOLINT
static_assert(test_read_next_symbol(0b10, end{}));                // NOLINT
static_assert(test_read_next_symbol(0b11100101, repeat{24, 12})); // NOLINT

static_assert([] {
  constexpr const auto data = std::array<unsigned, 5>{2, 2, 2, 1, 1};
  const auto tables = create_tables(data, 3);
  if (tables.hlist.counts[2] != 3)
    return false;
  if (tables.hlist.counts[1] != 0)
    return false;
  if (tables.hdist.counts.size() == 3)
    return false;
  if (tables.hdist.counts[1] != 2)
    return false;
  return true;
}());
