export module zipline:write;
import jute;
import hai;
import traits;

using namespace traits::ints;

namespace zipline {
  struct crc_table {
    uint32_t data[256];
  };
  static constexpr const crc_table table = [] {
    crc_table res{};
    for (auto n = 0; n < 256; n++) {
      uint32_t c = n;
      for (auto k = 0; k < 8; k++) {
        if (c & 1) {
          c = 0xedb88320 ^ (c >> 1);
        } else {
          c >>= 1;
        }
      }
      res.data[n] = c;
    }
    return res;
  }();
  
  static constexpr uint32_t update_crc(uint32_t crc, unsigned char *buf, unsigned len) {
    auto c = crc ^ ~0U;
    for (auto n = 0; n < len; n++) {
      auto idx = (c ^ buf[n]) & 0xFF;
      c = table.data[idx] ^ (c >> 8);
    }
    return c ^ ~0U;
  }
  static constexpr uint32_t start_crc(unsigned char *buf, unsigned len) {
    return update_crc(0U, buf, len);
  }

  enum class comp_method : uint16_t {
    stored = 0,
    deflated = 8,
  };

  struct __attribute__((packed)) eocd {
    uint32_t magic = 0x06054b50; // PK56
    uint16_t disk = 0;
    uint16_t cd_disk = 0;                  // Disk where CD exist
    uint16_t cd_entries_disk = 0;          // Number of CD entries on disk
    uint16_t cd_entries = cd_entries_disk; // Total number of CD entries
    uint32_t cd_size = 0;
    uint32_t cd_offset = 0;
    uint16_t comment_size = 0;
  };
  static_assert(sizeof(eocd) == 22);

  struct __attribute__((packed)) cdfh {
    uint32_t magic = 0x02014b50; // PK12
    uint16_t made_by_version = 20; // 2.0, enables DEFLATE
    uint16_t min_version = made_by_version;
    uint16_t flags = 0;
    comp_method method = comp_method::stored;
    uint16_t mod_date = 0;
    uint16_t mod_time = 0;
    uint32_t crc32 = 0;
    uint32_t compressed_size = 0;
    uint32_t uncompressed_size = 0;
    uint16_t name_size = 0;
    uint16_t extra_size = 0;
    uint16_t comment_size = 0;
    uint16_t disk = 0;
    uint16_t int_file_attr = 1; // 1 for binary files
    uint32_t ext_file_attr = 0;
    uint32_t rel_offset = 0;
  };
  static_assert(sizeof(cdfh) == 46);

  struct __attribute__((packed)) fh {
    uint32_t magic = 0x04034b50; // PK34
    uint16_t min_version = 20;
    uint16_t flags = 0;
    comp_method method = comp_method::stored;
    uint16_t mod_date = 0;
    uint16_t mod_time = 0;
    uint32_t crc32 = 0;
    uint32_t compressed_size = 0;
    uint32_t uncompressed_size = 0;
    uint16_t name_size = 0;
    uint16_t extra_size = 0;
  };
  static_assert(sizeof(fh) == 30);

  struct cdfhfn : cdfh {
    char filename[0xFFFF];
  };
  static_assert(sizeof(cdfhfn) == sizeof(cdfh) + 0xFFFF);

  export struct writer {
    virtual constexpr void write(const void *, unsigned) = 0;
    virtual constexpr uint32_t tell() = 0;

    template<typename T> constexpr void obj_write(const T & t) { write(&t, sizeof(T)); }
  };

  export class zipwriter {
    hai::varray<cdfhfn> m_cd {};
    writer * m_w;

  public:
    explicit constexpr zipwriter(writer * w) : m_w { w } {}

    void write_file(jute::view filename, jute::view content) {
      cdfhfn cd = {{
        .crc32 = zipline::start_crc((unsigned char *)content.data(), content.size()),
        .compressed_size = static_cast<uint32_t>(content.size()),
        .uncompressed_size = static_cast<uint32_t>(content.size()),
        .name_size = static_cast<uint16_t>(filename.size()),
        .rel_offset = m_w->tell(),
      }};
      auto p = cd.filename;
      for (auto c : filename) *p++ = c; 
      m_w->obj_write(fh {
        .crc32 = cd.crc32,
        .compressed_size = cd.compressed_size,
        .uncompressed_size = cd.uncompressed_size,
        .name_size = cd.name_size,
      });
      m_w->write(filename.data(), filename.size());
      m_w->write(content.begin(), content.size());
      m_cd.push_back_doubling(cd);
    }

    void write_cd() {
      uint32_t cd_offset = m_w->tell();
      for (auto & cd : m_cd) m_w->write(&cd, sizeof(cdfh) + cd.name_size);
      uint32_t cd_size = m_w->tell() - cd_offset;

      m_w->obj_write(eocd {
        .cd_entries_disk = static_cast<uint16_t>(m_cd.size()),
        .cd_size = cd_size,
        .cd_offset = cd_offset,
      });
    }
  };
}
