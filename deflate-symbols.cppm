module;
#include <array>
#include <cassert>
#include <optional>
#include <span>
#include <variant>

export module deflate:symbols;
import :tables;
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

[[nodiscard]] static constexpr auto read_fixed_dist(bitstream *bits) {
  return (bits->next<1>() << 4) | (bits->next<1>() << 3) |
         (bits->next<1>() << 2) | (bits->next<1>() << 1) | bits->next<1>();
}
static constexpr void test_read_fixed_dist(unsigned input, unsigned expected) {
  ce_bitstream b{yoyo::ce_reader{input}};
  assert(read_fixed_dist(&b) == expected);
}
static_assert([] { test_read_fixed_dist(0b00000, 0); });
static_assert([] { test_read_fixed_dist(0b10000, 16); });
static_assert([] { test_read_fixed_dist(0b1000, 8); });
static_assert([] { test_read_fixed_dist(0b100, 4); });
static_assert([] { test_read_fixed_dist(0b10, 2); });
static_assert([] { test_read_fixed_dist(0b1, 1); });
static_assert([] { test_read_fixed_dist(0b11111, 31); });

[[nodiscard]] static constexpr symbol
read_next_symbol(const tables::huff_tables &huff, bitstream *bits) {
  constexpr const auto end_code = 256;

  const auto code = decode_huffman(huff.hlist, bits);
  if (code < end_code)
    return raw{static_cast<uint8_t>(code)};
  if (code == end_code)
    return end{};
  assert(code <= tables::max_lens_code);

  const auto len_bits = tables::bitlens[code];
  const auto len = len_bits.second + bits->next(len_bits.bits);

  const auto dist_code = (huff.hdist.counts.size() > 0)
                             ? decode_huffman(huff.hdist, bits)
                             : read_fixed_dist(bits);
  assert(dist_code <= tables::max_dists_code);

  const auto dist_bits = tables::bitdists[dist_code];
  const auto dist = dist_bits.second + bits->next(dist_bits.bits);
  return repeat{len, dist};
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

} // namespace zipline::symbols

using namespace zipline::symbols;

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

  return zipline::tables::huff_tables{
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

static constexpr auto test_fixed_table(uint8_t first_byte, uint8_t second_byte,
                                       symbol expected) {
  auto bits =
      zipline::ce_bitstream{yoyo::ce_reader{first_byte, second_byte, 0}};
  auto sym =
      read_next_symbol(zipline::tables::create_fixed_huffman_table(), &bits);
  return sym == expected;
}
static_assert(test_fixed_table(0b01001100, 0, raw{2}));
static_assert(test_fixed_table(0b10010011, 0, raw{146}));
static_assert(test_fixed_table(0b0000000, 0, end{})); // 256
static_assert(test_fixed_table(0b10100000, 0,
                               repeat{4, 257})); // 258, dist=16,0
static_assert(test_fixed_table(0b01000011, 0, repeat{163, 1})); // 282
static_assert(test_fixed_table(0b01100000, 0b1000, repeat{5, 2}));
