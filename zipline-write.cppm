export module zipline:write;
import jute;
import traits;
import yoyo;

using namespace traits::ints;

namespace zipline {
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
