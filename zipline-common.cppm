export module zipline:common;
import hai;
import jute;
import traits;
import yoyo;

using namespace traits::ints;

namespace zipline {
struct cdir_meta {
  uint16_t count;
  uint32_t size;
  uint32_t offset;
};
export struct cdir_entry {
  uint16_t flags{};
  uint16_t method{};
  uint32_t crc{};
  uint32_t compressed_size{};
  uint32_t uncompressed_size{};
  uint32_t offset{};
  hai::array<uint8_t> filename{};
};

struct zip_exception {};

struct missing_eocd_error {};
struct truncated_eocd_error {};
struct multidisk_is_unsupported {};
struct zip64_is_unsupported {};

struct invalid_central_directory {};
struct truncated_central_directory {};
struct unsupported_zip_version {};

struct invalid_file_offset {};
struct invalid_local_directory {};
struct truncated_local_directory {};
struct local_directory_mismatch {};

constexpr const auto maximum_supported_version = 20; // 2.0 - Deflate

template <typename Exc, typename T> constexpr T unwrap(mno::req<T> v) {
  return v.take([](auto msg) { throw Exc{}; });
}

constexpr bool filename_matches(const cdir_entry &cd, jute::view fn) {
  if (cd.filename.size() != fn.size())
    return false;
  for (auto i = 0; i < fn.size(); i++) {
    if (cd.filename[i] != fn[i])
      return false;
  }
  return true;
}
} // namespace zipline
