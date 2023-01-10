module;
#include <array>
#include <span>

export module deflate:tables;
import huffman;

namespace zipline::tables {
struct bit_pair {
  unsigned bits;
  unsigned second;
};
constexpr bool operator==(const bit_pair &s, const bit_pair &r) {
  return s.bits == r.bits && s.second == r.second;
}

constexpr const auto max_lens_code = 285U;
constexpr const auto bitlens = [] {
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
static_assert(bitlens[257] == bit_pair{0, 3});   // NOLINT
static_assert(bitlens[258] == bit_pair{0, 4});   // NOLINT
static_assert(bitlens[259] == bit_pair{0, 5});   // NOLINT
static_assert(bitlens[260] == bit_pair{0, 6});   // NOLINT
static_assert(bitlens[261] == bit_pair{0, 7});   // NOLINT
static_assert(bitlens[264] == bit_pair{0, 10});  // NOLINT
static_assert(bitlens[280] == bit_pair{4, 115}); // NOLINT
static_assert(bitlens[281] == bit_pair{5, 131}); // NOLINT
static_assert(bitlens[284] == bit_pair{5, 227}); // NOLINT
static_assert(bitlens[285] == bit_pair{0, 258}); // NOLINT

constexpr const auto max_dists_code = 29U;
static constexpr const auto bitdists = [] {
  std::array<bit_pair, max_dists_code + 1> res{};
  res[0] = {0, 1};
  res[1] = {0, 2};
  for (auto i = 2U; i <= max_dists_code; i++) {
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
template <typename LenDistBits>
[[nodiscard]] constexpr huff_tables
create_tables(const LenDistBits &hlist_hdist, unsigned hlist_len) {
  std::span sp{hlist_hdist.begin(), hlist_hdist.end()};
  return huff_tables{
      .hlist = create_huffman_codes(sp.subspan(0, hlist_len)),
      .hdist = create_huffman_codes(sp.subspan(hlist_len)),
  };
}
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

static constexpr const auto fixed_hlist = [] {
  std::array<unsigned, 288> res{};
  std::fill(&res.at(0), &res.at(144), 8);
  std::fill(&res.at(144), &res.at(256), 9);
  std::fill(&res.at(256), &res.at(280), 7);
  std::fill(&res.at(280), res.end(), 8);
  return res;
}();
constexpr auto create_fixed_huffman_table() {
  return huff_tables{
      .hlist = create_huffman_codes(fixed_hlist),
      .hdist = {},
  };
}
static_assert(create_fixed_huffman_table().hlist.counts[0] == 0);
static_assert(create_fixed_huffman_table().hlist.counts[7] == 24);
static_assert(create_fixed_huffman_table().hlist.counts[8] == 144 + 8);
static_assert(create_fixed_huffman_table().hlist.counts[9] == 112);
} // namespace zipline::tables
