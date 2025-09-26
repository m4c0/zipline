#pragma leco tool
#include <stdio.h>
#include <stdlib.h>

import hay;
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

static void create_zip() {
  hay<FILE *, fopen, fclose> f { "out/test.zip", "wb" };

  fh x {
    .crc32 = zipline::start_crc((unsigned char *)"hello\n", 6),
    .compressed_size = 6,
    .uncompressed_size = 6,
    .name_size = 5,
  };
  fwrite(&x, sizeof(fh), 1, f);
  fprintf(f, "a.txt");
  fprintf(f, "hello\n");

  uint32_t b_ofs = ftell(f);
  x.crc32 = zipline::start_crc((unsigned char *)"world\n", 6),
  fwrite(&x, sizeof(fh), 1, f);
  fprintf(f, "b.txt");
  fprintf(f, "world\n");

  cdfh w {
    .crc32 = zipline::start_crc((unsigned char *)"hello\n", 6),
    .compressed_size = 6,
    .uncompressed_size = 6,
    .name_size = 5,
  };
  uint32_t cd_offset = ftell(f);
  fwrite(&w, sizeof(cdfh), 1, f);
  fprintf(f, "a.txt");

  w.rel_offset = b_ofs;
  w.crc32 = zipline::start_crc((unsigned char *)"world\n", 6),
  fwrite(&w, sizeof(cdfh), 1, f);
  fprintf(f, "b.txt");

  eocd v {
    .cd_entries_disk = 2,
    .cd_size = 2 * (sizeof(w) + 5),
    .cd_offset = cd_offset,
  };
  fwrite(&v, sizeof(eocd), 1, f);
}

int main() {
  create_zip();
  return system("unzip -c out/test.zip");
}
