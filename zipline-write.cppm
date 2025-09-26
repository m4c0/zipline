export module zipline:write;
import jute;
import traits;
import yoyo;

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

export uint32_t update_crc(uint32_t crc, unsigned char *buf, unsigned len) {
  auto c = crc ^ ~0U;
  for (auto n = 0; n < len; n++) {
    auto idx = (c ^ buf[n]) & 0xFF;
    c = table.data[idx] ^ (c >> 8);
  }
  return c ^ ~0U;
}
export uint32_t start_crc(unsigned char *buf, unsigned len) {
  return update_crc(0U, buf, len);
}

export struct cdfh {
  jute::view name;
  uint32_t crc32;
  uint32_t offset;
  uint32_t compressed_size;
  uint32_t uncompressed_size;
};

export auto write_lfh(yoyo::writer &w, const cdfh &cd) {
  return w
      .write_u32(0x04034b50)                 // PK34
      .fmap([&] { return w.write_u16(20); }) // Minimum version
      .fmap([&] { return w.write_u16(0); })  // Flags
      .fmap([&] { return w.write_u16(8); })  // Method (DEFLATE);
      .fmap([&] { return w.write_u16(0); })  // Modification date
      .fmap([&] { return w.write_u16(0); })  // Modification time
      .fmap([&] { return w.write_u32(cd.crc32); })
      .fmap([&] { return w.write_u32(cd.compressed_size); })
      .fmap([&] { return w.write_u32(cd.uncompressed_size); })
      .fmap([&] { return w.write_u16(cd.name.size()); })
      .fmap([&] { return w.write_u16(0); }) // Extra size
      .fmap([&] { return w.write(cd.name.begin(), cd.name.size()); });
}

export auto write_cd(yoyo::writer &w, const cdfh &cd) {
  return w
      .write_u32(0x02014b50)                 // PK12
      .fmap([&] { return w.write_u16(0); })  // Made by
      .fmap([&] { return w.write_u16(20); }) // Minimum version
      .fmap([&] { return w.write_u16(0); })  // Flags
      .fmap([&] { return w.write_u16(8); })  // Method (DEFLATE);
      .fmap([&] { return w.write_u16(0); })  // Modification date
      .fmap([&] { return w.write_u16(0); })  // Modification time
      .fmap([&] { return w.write_u32(cd.crc32); })
      .fmap([&] { return w.write_u32(cd.compressed_size); })
      .fmap([&] { return w.write_u32(cd.uncompressed_size); })
      .fmap([&] { return w.write_u16(cd.name.size()); })
      .fmap([&] { return w.write_u16(0); }) // Extra size
      .fmap([&] { return w.write_u16(0); }) // Comment size
      .fmap([&] { return w.write_u16(0); }) // Disk number
      .fmap([&] { return w.write_u16(0); }) // Internal file attributes
      .fmap([&] { return w.write_u32(0); }) // External file attributes
      .fmap([&] { return w.write_u32(cd.offset); })
      .fmap([&] { return w.write(cd.name.begin(), cd.name.size()); });
}
export auto write_eocd(yoyo::writer &w, unsigned num, unsigned offset,
                       unsigned size) {
  return w
      .write_u32(0x06054b50)                     // PK56
      .fmap([&] { return w.write_u16(0); })      // Disk
      .fmap([&] { return w.write_u16(0); })      // Disk where CD exist
      .fmap([&] { return w.write_u16(num); })    // Number of CD entries on disk
      .fmap([&] { return w.write_u16(num); })    // Total number of CD entries
      .fmap([&] { return w.write_u32(size); })   // Size of CD
      .fmap([&] { return w.write_u32(offset); }) // Offset of CD
      .fmap([&] { return w.write_u16(0); })      // Size of comment
      ;
}
} // namespace zipline
