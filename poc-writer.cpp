#pragma leco tool
#include <stdio.h>
#include <stdlib.h>

import hay;
import zipline;

struct eocd {
  uint32_t magic = 0x06054b50; // PK56
  uint16_t disk = 0;
  uint16_t cd_disk = 0;                  // Disk where CD exist
  uint16_t cd_entries_disk = 0;          // Number of CD entries on disk
  uint16_t cd_entries = cd_entries_disk; // Total number of CD entries
  uint32_t cd_size = 0;
  uint32_t cd_offset = 0;
  uint16_t comment_size = 2;
  uint16_t comment_aka_padding = 0;
};
static_assert(sizeof(eocd) == 24);

static void create_zip() {
  hay<FILE *, fopen, fclose> f { "out/test.zip", "wb" };

  eocd v {};
  fwrite(&v, sizeof(eocd), 1, f);
}

int main() {
  create_zip();
  return system("unzip -t out/test.zip");
}
