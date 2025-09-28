#pragma leco test
#define MCT_SYSCALL_IMPLEMENTATION
#include "../mct/mct-syscall.h"
#include <stdio.h>
#include <stdlib.h>

import hay;
import jute;
import mtime;
import print;
import traits;
import zipline;

using namespace traits::ints;

static void fail(jute::view err) { die(err); }

static void process(jute::view file) {
  hay<FILE *, mct_syscall_fopen, fclose> f { "out/test.zip", "rb" };
  
  fseek(f, 0, SEEK_END);
  auto fsize = ftell(f);

  zipline::eocd e {};
  for (int64_t p = fsize - sizeof(zipline::eocd); p >= 0; p--) {
    fseek(f, p, SEEK_SET);
    fread(&e, sizeof(zipline::eocd), 1, f);
    if (e.magic == zipline::eocd_magic) break;
  }
  if (e.magic != zipline::eocd_magic)
    return fail("end-of-central-directory not found");
  if (e.disk != 0 || e.cd_disk != 0)
    return fail("multi-disk is not supported");

  hay<char[]> name_buf { 0xFFFF };
  hay<char[]> name2_buf { 0xFFFF };
  unsigned offs = 0;
  while (offs < e.cd_size) {
    zipline::cdfh c {};
    fseek(f, e.cd_offset + offs, SEEK_SET);
    fread(&c, sizeof(zipline::cdfh), 1, f);
    if (c.magic != zipline::cdfh_magic)
      return fail("invalid entry in central-directory");
    if (c.min_version > 20)
      return fail("file contains unsupported version");
    if (c.disk != 0)
      return fail("multi-disk is not supported");

    fread(static_cast<char *>(name_buf), c.name_size, 1, f);
    jute::view name { name_buf, c.name_size };

    zipline::fh h {};
    fseek(f, c.rel_offset, SEEK_SET);
    fread(&h, sizeof(zipline::fh), 1, f);
    if (h.magic != zipline::fh_magic)
      return fail("invalid entry in file header");
    if (h.min_version > 20)
      return fail("file contains unsupported version");
    if (h.crc32 != c.crc32)
      return fail("crc32 differs between central-directory and file header");
    if (h.compressed_size != c.compressed_size)
      return fail("compressed size differs between central-directory and file header");
    if (h.uncompressed_size != c.uncompressed_size)
      return fail("uncompressed size differs between central-directory and file header");
    if (h.method != c.method)
      return fail("compression method differs between central-directory and file header");

    fread(static_cast<char *>(name2_buf), h.name_size, 1, f);
    if (name != jute::view { name2_buf, h.name_size })
      return fail("file name differs between central-directory and file header");

    // callback? add to a list?
    putan(name,
        c.rel_offset + sizeof(zipline::fh) + h.name_size + h.extra_size,
        h.compressed_size,
        h.uncompressed_size,
        h.crc32,
        static_cast<int>(h.method));

    offs += sizeof(zipline::cdfh) + c.name_size + c.extra_size + c.comment_size;
  }
}

int main() {
  if (!mtime::of("out/test.zip")) system("zip out/test.zip *.cpp");
  process("out/test.zip");
  return 0;
}
