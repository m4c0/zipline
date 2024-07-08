module;
#include <algorithm>
#include <cstdint>
#include <exception>
#include <optional>
#include <string_view>

export module zipline:common;
import hai;
import yoyo;

export namespace zipline {
struct cdir_meta {
  uint16_t count;
  uint32_t size;
  uint32_t offset;
};
struct cdir_entry {
  uint16_t flags{};
  uint16_t method{};
  uint32_t crc{};
  uint32_t compressed_size{};
  uint32_t uncompressed_size{};
  uint32_t offset{};
  hai::array<uint8_t> filename{};
  hai::array<uint8_t> extra{};
};

struct zip_exception : std::exception {};

struct missing_eocd_error : zip_exception {};
struct truncated_eocd_error : zip_exception {};
struct multidisk_is_unsupported : zip_exception {};
struct zip64_is_unsupported : zip_exception {};

struct invalid_central_directory : zip_exception {};
struct truncated_central_directory : zip_exception {};
struct unsupported_zip_version : zip_exception {};

struct invalid_file_offset : zip_exception {};
struct invalid_local_directory : zip_exception {};
struct truncated_local_directory : zip_exception {};
struct local_directory_mismatch : zip_exception {};

constexpr const auto maximum_supported_version = 20; // 2.0 - Deflate

template <typename Exc, typename T> constexpr T unwrap(mno::req<T> v) {
  return v.take([](auto msg) { throw Exc{}; });
}

constexpr bool filename_matches(const cdir_entry &cd,
                                std::string_view filename) {
  return std::equal(filename.begin(), filename.end(), cd.filename.begin(),
                    cd.filename.end());
}
} // namespace zipline
