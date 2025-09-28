#pragma leco test
#include <stdio.h>
#include <stdlib.h>

import hay;
import jute;
import mtime;
import print;
import zipline;

static void fail(jute::view err) { die(err); }

static void process(jute::view file) {
  hay<FILE *, fopen, fclose> f { "out/test.zip", "rb" };
  
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
  unsigned offs = 0;
  while (offs < e.cd_size) {
    fseek(f, e.cd_offset + offs, SEEK_SET);

    zipline::cdfh c {};
    fread(&c, sizeof(zipline::cdfh), 1, f);
    if (c.magic != zipline::cdfh_magic)
      return fail("invalid entry in central-directory");
    if (c.min_version > 20)
      return fail("file contains unsupported version");
    if (c.disk != 0)
      return fail("multi-disk is not supported");

    fread(static_cast<char *>(name_buf), c.name_size, 1, f);
    offs += sizeof(zipline::cdfh) + c.name_size + c.extra_size + c.comment_size;

    putln(jute::view { name_buf, c.name_size });
  }
}

int main() {
  if (!mtime::of("out/test.zip")) system("zip out/test.zip *.cpp");
  process("out/test.zip");
  return 0;
}
