module;
#include <optional>
#include <string_view>

export module zipline:iterator;
import :common;
import :cdir;
import :eocd;
import yoyo;

namespace zipline {
class zip_iterator {
  yoyo::subreader m_reader;

  static constexpr auto subreader(yoyo::reader *r) {
    auto eocd = read_eocd(r);
    return unwrap<truncated_central_directory>(
        yoyo::subreader::seek_and_create(r, eocd.offset, eocd.size));
  }

public:
  constexpr explicit zip_iterator(yoyo::reader *r) : m_reader{subreader(r)} {}

  [[nodiscard]] constexpr explicit operator bool() const noexcept {
    return !m_reader.eof();
  }

  [[nodiscard]] constexpr auto next() { return read_cd(&m_reader); }

  [[nodiscard]] constexpr cdir_entry find(std::string_view filename) {
    if (!m_reader.seekg(0))
      return {};

    while (*this) {
      auto cd = next();
      if (filename_matches(cd, filename)) {
        return cd;
      }
    }

    return {};
  }
};
} // namespace zipline

static constexpr auto zip = yoyo::ce_reader{
    0x50, 0x4b, 0x03, 0x04, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x4e,
    0x2d, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x08, 0x00, 0x1c, 0x00, 0x66, 0x69, 0x6c, 0x65, 0x2e, 0x74,
    0x78, 0x74, 0x55, 0x54, 0x09, 0x00, 0x03, 0xd8, 0xf5, 0xdf, 0x61, 0xd8,
    0xf5, 0xdf, 0x61, 0x75, 0x78, 0x0b, 0x00, 0x01, 0x04, 0xf6, 0x01, 0x00,
    0x00, 0x04, 0x14, 0x00, 0x00, 0x00, 0x50, 0x4b, 0x03, 0x04, 0x14, 0x00,
    0x00, 0x00, 0x08, 0x00, 0xb7, 0x98, 0x2d, 0x54, 0xba, 0x06, 0x4c, 0xba,
    0x80, 0x01, 0x00, 0x00, 0x0e, 0x03, 0x00, 0x00, 0x0c, 0x00, 0x1c, 0x00,
    0x7a, 0x69, 0x70, 0x2e, 0x65, 0x6f, 0x63, 0x64, 0x2e, 0x63, 0x70, 0x70,
    0x55, 0x54, 0x09, 0x00, 0x03, 0x0a, 0x78, 0xe0, 0x61, 0x0b, 0x78, 0xe0,
    0x61, 0x75, 0x78, 0x0b, 0x00, 0x01, 0x04, 0xf6, 0x01, 0x00, 0x00, 0x04,
    0x14, 0x00, 0x00, 0x00, 0x8d, 0x52, 0x4d, 0x6b, 0x83, 0x40, 0x10, 0xbd,
    0xfb, 0x2b, 0x06, 0x0b, 0x21, 0xa1, 0x98, 0x98, 0xd4, 0xe4, 0xa0, 0x78,
    0x28, 0xb4, 0xf7, 0xde, 0x9b, 0x22, 0xdb, 0xdd, 0x31, 0x91, 0xe8, 0xae,
    0xcc, 0xae, 0xc5, 0xa6, 0xe4, 0xbf, 0x77, 0xd4, 0x7c, 0x11, 0xda, 0xd2,
    0xc3, 0xb0, 0xcb, 0x9b, 0x37, 0xfb, 0xde, 0x1b, 0xbd, 0x2b, 0xb4, 0x2c,
    0x1b, 0x85, 0xe0, 0x57, 0x91, 0x0c, 0x67, 0x82, 0x76, 0xb3, 0x7d, 0x51,
    0x4f, 0xd1, 0x48, 0x35, 0xdd, 0xd6, 0xb5, 0xef, 0x79, 0xb3, 0x19, 0x3c,
    0x6b, 0x15, 0x98, 0x3c, 0x90, 0xa8, 0x1d, 0x89, 0x32, 0x50, 0x05, 0xa1,
    0x74, 0x86, 0x3e, 0x61, 0x2b, 0x2c, 0x08, 0x0d, 0xd8, 0x32, 0x0e, 0xd2,
    0x54, 0x15, 0x33, 0xc0, 0x19, 0x70, 0x68, 0xf9, 0xdc, 0x22, 0x10, 0x7e,
    0x20, 0x59, 0x04, 0x8b, 0xb8, 0x63, 0xa6, 0x3a, 0x52, 0x6b, 0xa1, 0x54,
    0xa1, 0x37, 0x20, 0x1c, 0xe4, 0x64, 0xb4, 0xf3, 0xac, 0x13, 0xae, 0x90,
    0xfc, 0x84, 0xb6, 0x0e, 0xdb, 0x9a, 0x40, 0x34, 0xfc, 0x4c, 0xe7, 0x22,
    0x53, 0xc2, 0x09, 0x48, 0xa1, 0xb3, 0x17, 0xc7, 0x85, 0x89, 0x63, 0x89,
    0x19, 0xa1, 0x50, 0x48, 0xf0, 0xe5, 0x01, 0xf8, 0xeb, 0x76, 0x19, 0xae,
    0xdb, 0xe8, 0x7d, 0xdd, 0x86, 0x73, 0xae, 0x05, 0x57, 0xe8, 0x03, 0xbb,
    0x66, 0x91, 0x9b, 0xfe, 0x92, 0x6b, 0xd5, 0xf5, 0xaf, 0xcb, 0x1f, 0x48,
    0xfd, 0x70, 0x78, 0x39, 0x23, 0xfc, 0x81, 0x14, 0x2d, 0x6e, 0xa7, 0xb9,
    0x1e, 0xfa, 0x66, 0x10, 0x04, 0x67, 0xd5, 0x43, 0xe2, 0x79, 0x8d, 0xed,
    0x02, 0x6a, 0x51, 0xa1, 0xad, 0x85, 0xc4, 0xa3, 0x7f, 0xde, 0x6f, 0x1c,
    0xf3, 0x82, 0x99, 0x30, 0x44, 0xce, 0x84, 0xb5, 0x48, 0x6e, 0xfc, 0xfa,
    0xd6, 0xa7, 0xb9, 0x2c, 0xa0, 0xbf, 0x0d, 0x6b, 0x10, 0xb9, 0x43, 0xca,
    0xea, 0xdd, 0x72, 0x95, 0xd5, 0xc6, 0xf2, 0x2e, 0x96, 0x70, 0x0f, 0x51,
    0xd2, 0xa9, 0xbd, 0x3c, 0x3e, 0xf1, 0xbd, 0x79, 0x58, 0x78, 0x3c, 0xdc,
    0x93, 0x89, 0xfb, 0xe7, 0xbd, 0x25, 0x8c, 0xe6, 0x85, 0x56, 0x59, 0x8f,
    0xb0, 0x22, 0x2b, 0x8d, 0x68, 0xd2, 0xc1, 0x84, 0xae, 0x21, 0x0d, 0x34,
    0x75, 0x58, 0x96, 0x9b, 0xf1, 0x04, 0xd2, 0xf4, 0x46, 0x28, 0xf1, 0x0e,
    0xe3, 0x09, 0x73, 0xff, 0x6f, 0x54, 0xaa, 0x2a, 0xb3, 0xc5, 0x1e, 0xd9,
    0x42, 0xc8, 0x0b, 0x4c, 0xfe, 0xe2, 0x99, 0x3c, 0xb7, 0xe8, 0x06, 0xe6,
    0x22, 0xf9, 0xdd, 0xfe, 0x69, 0x80, 0xf1, 0xee, 0xab, 0xf7, 0x49, 0x6e,
    0x32, 0x70, 0x77, 0x2a, 0x4d, 0xc3, 0xbf, 0x1e, 0x67, 0x98, 0xc3, 0x68,
    0xd4, 0x23, 0x83, 0x91, 0xf4, 0x62, 0xea, 0x88, 0x9f, 0x84, 0xd3, 0x2b,
    0x1b, 0xa7, 0xa8, 0xdf, 0x50, 0x4b, 0x01, 0x02, 0x1e, 0x03, 0x0a, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x48, 0x4e, 0x2d, 0x54, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x18, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa4, 0x81, 0x00, 0x00,
    0x00, 0x00, 0x66, 0x69, 0x6c, 0x65, 0x2e, 0x74, 0x78, 0x74, 0x55, 0x54,
    0x05, 0x00, 0x03, 0xd8, 0xf5, 0xdf, 0x61, 0x75, 0x78, 0x0b, 0x00, 0x01,
    0x04, 0xf6, 0x01, 0x00, 0x00, 0x04, 0x14, 0x00, 0x00, 0x00, 0x50, 0x4b,
    0x01, 0x02, 0x1e, 0x03, 0x14, 0x00, 0x00, 0x00, 0x08, 0x00, 0xb7, 0x98,
    0x2d, 0x54, 0xba, 0x06, 0x4c, 0xba, 0x80, 0x01, 0x00, 0x00, 0x0e, 0x03,
    0x00, 0x00, 0x0c, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0xa4, 0x81, 0x42, 0x00, 0x00, 0x00, 0x7a, 0x69, 0x70, 0x2e,
    0x65, 0x6f, 0x63, 0x64, 0x2e, 0x63, 0x70, 0x70, 0x55, 0x54, 0x05, 0x00,
    0x03, 0x0a, 0x78, 0xe0, 0x61, 0x75, 0x78, 0x0b, 0x00, 0x01, 0x04, 0xf6,
    0x01, 0x00, 0x00, 0x04, 0x14, 0x00, 0x00, 0x00, 0x50, 0x4b, 0x05, 0x06,
    0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0xa0, 0x00, 0x00, 0x00,
    0x08, 0x02, 0x00, 0x00, 0x00, 0x00};

constexpr const auto crc = 0xba4c06baU;

static_assert([] {
  auto r = zip;
  zipline::zip_iterator it{&r};
  if (it.find("file.txt").crc != 0)
    return false;
  if (it.find("zip.eocd.cpp").crc != crc)
    return false;
  return true;
}());
