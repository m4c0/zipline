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
static constexpr bool operator==(const bit_pair &s, const bit_pair &r) {
  return s.bits == r.bits && s.second == r.second;
}

static constexpr const auto bitlens = [] {
  constexpr const auto max_lens_code = 285U;
  constexpr const auto first_parametric_code = 261U;
  constexpr const auto min_lens_code = 257U;

  std::array<bit_pair, max_lens_code + 1> res{};
  for (auto i = min_lens_code; i < first_parametric_code; i++) {
    res[i] = {0, i - min_lens_code + 3U};
  }
  for (auto i = first_parametric_code; i < max_lens_code; i++) {
    const auto bits = (i - first_parametric_code) / 4U;
    res[i] = {bits, res[i - 1].second + (1U << res[i - 1].bits)};
  }

  // For some reason, this does not follow the pattern
  res[max_lens_code] = {0, 258};
  return res;
}();
static_assert(bitlens[258] == bit_pair{0, 4});   // NOLINT
static_assert(bitlens[280] == bit_pair{4, 115}); // NOLINT
static_assert(bitlens[281] == bit_pair{5, 131}); // NOLINT
static_assert(bitlens[284] == bit_pair{5, 227}); // NOLINT
static_assert(bitlens[285] == bit_pair{0, 258}); // NOLINT

static constexpr const auto bitdists = [] {
  constexpr const auto last = 29;
  std::array<bit_pair, last + 1> res{};
  res[0] = {0, 1};
  res[1] = {0, 2};
  for (auto i = 2U; i <= last; i++) {
    const auto bits = (i - 2U) / 2U;
    res[i] = {bits, res[i - 1].second + (1U << res[i - 1].bits)};
  }
  return res;
}();
static_assert(bitdists[0] == bit_pair{0, 1});       // NOLINT
static_assert(bitdists[1] == bit_pair{0, 2});       // NOLINT
static_assert(bitdists[2] == bit_pair{0, 3});       // NOLINT
static_assert(bitdists[3] == bit_pair{0, 4});       // NOLINT
static_assert(bitdists[4] == bit_pair{1, 5});       // NOLINT
static_assert(bitdists[5] == bit_pair{1, 7});       // NOLINT
static_assert(bitdists[6] == bit_pair{2, 9});       // NOLINT
static_assert(bitdists[29] == bit_pair{13, 24577}); // NOLINT

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

  const auto len_bits = bitlens[code];
  const auto len = len_bits.second + bits->next(len_bits.bits);

  const auto dist_code = (huff.hdist.counts.size() > 0)
                             ? decode_huffman(huff.hdist, bits)
                             : bits->next<5>();

  const auto dist_bits = bitdists[dist_code];
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
static constexpr const auto fixed_hlist = [] {
  std::array<unsigned, 288> res{};
  std::fill(&res.at(0), &res.at(144), 8);
  std::fill(&res.at(144), &res.at(256), 9);
  std::fill(&res.at(256), &res.at(280), 7);
  std::fill(&res.at(280), res.end(), 8);
  return res;
}();
static constexpr auto create_fixed_huffman_table() {
  return huff_tables{
      .hlist = create_huffman_codes(fixed_hlist),
      .hdist = {},
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

static_assert(create_fixed_huffman_table().hlist.counts[0] == 0);
static_assert(create_fixed_huffman_table().hlist.counts[7] == 24);
static_assert(create_fixed_huffman_table().hlist.counts[8] == 144 + 8);
static_assert(create_fixed_huffman_table().hlist.counts[9] == 112);
static constexpr auto test_fixed_table(uint8_t first_byte, symbol expected) {
  auto bits = zipline::ce_bitstream{yoyo::ce_reader{first_byte, 0, 0}};
  auto sym = read_next_symbol(create_fixed_huffman_table(), &bits);
  return sym == expected;
}
static_assert(test_fixed_table(0b01001100, raw{2}));
static_assert(test_fixed_table(0b10010011, raw{146}));
static_assert(test_fixed_table(0b0000000, end{}));           // 256
static_assert(test_fixed_table(0b10100000, repeat{4, 2}));   // 258
static_assert(test_fixed_table(0b01000011, repeat{163, 1})); // 282
