export module zipline:objects;
import traits;

using namespace traits::ints;

export namespace zipline {
  constexpr const auto eocd_magic = 0x06054b50; // PK56

  enum class comp_method : uint16_t {
    stored = 0,
    deflated = 8,
  };

  struct __attribute__((packed)) eocd {
    uint32_t magic = eocd_magic;
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
}
