module;
#include <algorithm>
#include <array>
#include <exception>
#include <optional>

export module huffman;
import bitstream;
import containers;
import yoyo;

export namespace zipline {
struct invalid_huffman_code : std::exception {};

struct huffman_codes {
  // Counts per bit length
  containers::unique_array<unsigned> counts{};
  // Symbol per offset
  containers::unique_array<unsigned> indexes{};
};

// section 3.2.2 of RFC 1951 - using a variant based on ZLIB algorithms
template <typename LengthArray>
[[nodiscard]] constexpr auto create_huffman_codes(const LengthArray &lengths) {
  const auto max_codes = lengths.size();
  const auto max_bits = *std::max_element(lengths.begin(), lengths.end());
  huffman_codes res;
  res.counts = containers::unique_array<unsigned>{max_bits + 1};

  for (auto &e : res.counts) {
    e = 0;
  }
  for (auto len : lengths) {
    res.counts.at(len)++;
  }

  containers::unique_array<unsigned> offsets{max_bits + 1};
  offsets.at(1) = 0;
  for (auto bits = 1; bits < max_bits; bits++) {
    offsets.at(bits + 1) = offsets.at(bits) + res.counts.at(bits);
  }

  res.indexes = containers::unique_array<unsigned>{offsets.at(max_bits) +
                                                   res.counts.at(max_bits)};
  for (auto n = 0; n < max_codes; n++) {
    auto len = lengths[n];
    if (len != 0) {
      res.indexes.at(offsets.at(len)) = n;
      offsets.at(len)++;
    }
  }

  return res;
}

[[nodiscard]] constexpr auto decode_huffman(const huffman_codes &hc,
                                            bitstream *bits) {
  unsigned code = 0;
  unsigned first = 0;
  unsigned index = 0;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  for (const auto *it = hc.counts.begin() + 1; it != hc.counts.end(); ++it) {
    auto count = *it;

    code |= bits->next<1>();
    if (code < first + count) {
      return hc.indexes.at(index + code - first);
    }
    index += count;
    first = (first + count) << 1U;
    code <<= 1U;
  }
  throw invalid_huffman_code{};
}
} // namespace zipline

using namespace zipline;

static constexpr bool operator==(const auto &a, const auto &b) noexcept {
  return std::equal(a.begin(), a.end(), b.begin(), b.end());
}
// Checks huffman table construction
static_assert([] {
  constexpr const auto expected_counts = std::array{0, 0, 1, 5, 2};
  constexpr const auto expected_symbols =
      std::array<unsigned, 8>{5, 0, 1, 2, 3, 4, 6, 7};

  const auto hfc =
      create_huffman_codes(std::array<unsigned, 8>{3, 3, 3, 3, 3, 2, 4, 4});
  if (hfc.counts != expected_counts)
    return false;
  if (hfc.indexes != expected_symbols)
    return false;
  return true;
}());

// Checks again with unused symbols
static_assert([] {
  constexpr const auto expected_counts = std::array{4, 0, 0, 5};
  constexpr const auto expected_symbols = std::array{0, 2, 4, 6, 8};

  const auto hfc =
      create_huffman_codes(std::array{3U, 0U, 3U, 0U, 3U, 0U, 3U, 0U, 3U});
  if (hfc.counts != expected_counts)
    return false;
  if (hfc.indexes != expected_symbols)
    return false;
  return true;
}());

// Checks symbol lookup
// 0 F 00
// 1 A 010
// 2 B 011
// 3 C 100
// 4 D 101
// 5 E 110
// 6 G 1110
// 7 H 1111
static_assert([] {
  const auto hfc =
      create_huffman_codes(std::array<unsigned, 8>{3, 3, 3, 3, 3, 2, 4, 4});
  auto r = yoyo::ce_reader{0b11100100, 0b01111011}; // NOLINT
  bitstream b{&r};

  constexpr const auto expected_result = std::array{5, 2, 7, 3, 6};
  for (auto er : expected_result) {
    if (decode_huffman(hfc, &b) != er)
      return false;
  }
  return true;
}());
