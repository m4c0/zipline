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

  fseek(f, e.cd_offset, SEEK_SET);
}

int main() {
  if (!mtime::of("out/test.zip")) system("zip out/test.zip *.cpp");
  process("out/test.zip");
  return 0;
}
