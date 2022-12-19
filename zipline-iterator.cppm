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
