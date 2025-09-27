#pragma leco test
#include <stdio.h>
#include <stdlib.h>

import hay;
import mtime;
import zipline;

int main() {
  if (!mtime::of("out/test.zip")) system("zip out/test.zip *.cpp");

  hay<FILE *, fopen, fclose> f { "out/test.zip", "rb" };
  
  fseek(f, 0, SEEK_END);
  auto fsize = ftell(f);

  zipline::eocd e {};
  for (int64_t p = fsize - sizeof(zipline::eocd); p >= 0; p--) {
    fseek(f, p, SEEK_SET);
    fread(&e, sizeof(zipline::eocd), 1, f);
    if (e.magic == zipline::eocd_magic) return 0;
  }
  if (e.magic != zipline::eocd_magic) return 1;

  return 0;
}
