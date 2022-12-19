module;
#include <algorithm>
#include <array>
#include <optional>
#include <string_view>

export module zipline:cdir;
import :common;
import containers;
import yoyo;

namespace zipline {
export constexpr auto read_cd(yoyo::reader *r) {
  constexpr const auto cdir_magic = 0x02014b50; // PK\1\2

  if (unwrap<truncated_central_directory>(r->read_u32()) != cdir_magic) {
    throw invalid_central_directory{};
  }
  unwrap<truncated_central_directory>(r->read_u16()); // Version made by
  if (unwrap<truncated_central_directory>(r->read_u16()) >
      maximum_supported_version) {
    throw unsupported_zip_version{};
  }

  cdir_entry result;
  result.flags = unwrap<truncated_central_directory>(r->read_u16());
  result.method = unwrap<truncated_central_directory>(r->read_u16());

  unwrap<truncated_central_directory>(r->read_u16()); // Modification time
  unwrap<truncated_central_directory>(r->read_u16()); // Modification date

  result.crc = unwrap<truncated_central_directory>(r->read_u32());
  result.compressed_size = unwrap<truncated_central_directory>(r->read_u32());
  result.uncompressed_size = unwrap<truncated_central_directory>(r->read_u32());

  auto filename_len = unwrap<truncated_central_directory>(r->read_u16());
  auto extra_len = unwrap<truncated_central_directory>(r->read_u16());
  auto comment_len = unwrap<truncated_central_directory>(r->read_u16());

  if (unwrap<truncated_central_directory>(r->read_u16()) != 0) {
    throw multidisk_is_unsupported{};
  }

  unwrap<truncated_central_directory>(r->read_u16()); // Internal file attr
  unwrap<truncated_central_directory>(r->read_u32()); // External file attr

  result.offset = unwrap<truncated_central_directory>(r->read_u32());

  result.filename = containers::unique_array<uint8_t>{filename_len};
  if (!r->read(result.filename.begin(), filename_len)) {
    throw truncated_central_directory{};
  }

  result.extra = containers::unique_array<uint8_t>{extra_len};
  if (!r->read(result.extra.begin(), extra_len)) {
    throw truncated_central_directory{};
  }

  if (!r->seekg(comment_len, yoyo::seek_mode::current)) {
    throw truncated_central_directory{};
  }

  return result;
}
} // namespace zipline

static constexpr auto cd_data = yoyo::ce_reader{
    "\x50\x4b\x01\x02"                 // magic
    "\x1e\x03\x14\x00"                 // version (by + needed)
    "\x00\x00\x08\x00"                 // flags + method
    "\xb7\x98\x2d\x54"                 // modification time + date
    "\xba\x06\x4c\xba"                 // crc32
    "\x80\x01\x00\x00\x0e\x03\x00\x00" // compressed + uncompressed sizes
    "\x0c\x00\x18\x00\x00\x00"         // sizes (file, extra, comment)
    "\x00\x00\x01\x00\x00\x00\xa4\x81" // disk + attrs
    "\x42\x00\x00\x00"                 // offset

    "\x7a\x69\x70\x2e\x65\x6f\x63\x64\x2e\x63\x70\x70" // filename

    // extra
    "\x55\x54\x05\x00\x03\x0a\x78\xe0\x61\x75\x78\x0b\x00\x01\x04\xf6\x01\x00"
    "\x00\x04\x14\x00\x00\x00"};

using namespace zipline;

static_assert([] {
  constexpr const auto comp_size = 0x180;
  constexpr const auto uncomp_size = 0x030e;
  constexpr const auto crc = 0xba4c06baU;
  constexpr const auto offset = 0x42;
  constexpr const auto extra_size = 0x18;
  constexpr const std::string_view filename{"zip.eocd.cpp"};

  auto r = cd_data;
  auto cd = read_cd(&r);
  if (cd.compressed_size != comp_size)
    return false;
  if (cd.uncompressed_size != uncomp_size)
    return false;
  if (cd.crc != crc)
    return false;
  if (cd.offset != offset)
    return false;
  if (cd.filename.size() != filename.size())
    return false;
  if (cd.extra.size() != extra_size)
    return false;
  if (!filename_matches(cd, filename))
    return false;
  return r.eof();
}());
