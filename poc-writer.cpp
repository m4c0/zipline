#pragma leco tool
#include <stdio.h>
#include <stdlib.h>

import hai;
import hay;
import jute;
import zipline;

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

struct writer {
  virtual constexpr void write(const void *, unsigned) = 0;
  virtual constexpr uint32_t tell() = 0;

  template<typename T>
  constexpr void obj_write(const T & t) { write(&t, sizeof(T)); }
};

struct hay_writer : writer {
  hay<FILE *, fopen, fclose> f { "out/test.zip", "wb" };

  // TODO: treat errors
  void write(const void * ptr, unsigned sz) override { fwrite(ptr, sz, 1, f); }
  uint32_t tell() override { return ftell(f); }
};

struct cdfhfn : cdfh {
  char filename[0xFFFF];
};
static_assert(sizeof(cdfhfn) == sizeof(cdfh) + 0xFFFF);

static void create_zip() {
  hay_writer h {};

  hai::array<cdfhfn> cd { 2 };

  // Write A

  jute::view filename = "this-is-a.txt";
  jute::view content = "hello wonderful world\n";
  cd[0] = {{
    .crc32 = zipline::start_crc((unsigned char *)content.data(), content.size()),
    .compressed_size = static_cast<uint32_t>(content.size()),
    .uncompressed_size = static_cast<uint32_t>(content.size()),
    .name_size = static_cast<uint16_t>(filename.size()),
    .rel_offset = h.tell(),
  }};
  auto p = cd[0].filename;
  for (auto c : filename) *p++ = c; 
  h.obj_write(fh {
    .crc32 = cd[0].crc32,
    .compressed_size = cd[0].compressed_size,
    .uncompressed_size = cd[0].uncompressed_size,
    .name_size = cd[0].name_size,
  });
  h.write(filename.data(), filename.size());
  h.write(content.begin(), content.size());

  // Write B

  cd[1] = {{
    .crc32 = zipline::start_crc((unsigned char *)"world\n", 6),
    .compressed_size = 6,
    .uncompressed_size = 6,
    .name_size = 5,
    .rel_offset = h.tell(),
  }};
  h.obj_write(fh {
    .crc32 = cd[1].crc32,
    .compressed_size = cd[1].compressed_size,
    .uncompressed_size = cd[1].uncompressed_size,
    .name_size = cd[1].name_size,
  });
  h.write("b.txt", 5);
  h.write("world\n", 6);

  // Write CD

  uint32_t cd_offset = h.tell();
  h.write(&cd[0], sizeof(cdfh) + cd[0].name_size);

  h.write(&cd[1], sizeof(cdfh));
  h.write("b.txt", 5);
  uint32_t cd_size = h.tell() - cd_offset;

  eocd v {
    .cd_entries_disk = static_cast<uint16_t>(cd.size()),
    .cd_size = cd_size,
    .cd_offset = cd_offset,
  };
  h.write(&v, sizeof(eocd));
}

int main() {
  create_zip();
  return system("unzip -c out/test.zip");
}
