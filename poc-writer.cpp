#pragma leco tool
#include <stdio.h>
#include <stdlib.h>

import hay;
import jute;
import zipline;

struct hay_writer : zipline::writer {
  hay<FILE *, fopen, fclose> f { "out/test.zip", "wb" };

  // TODO: treat errors
  void write(const void * ptr, unsigned sz) override { fwrite(ptr, sz, 1, f); }
  uint32_t tell() override { return ftell(f); }
};

static void create_zip() {
  hay_writer h {};
  zipline::zipwriter zip { &h };

  zip.write_file("this-is-a.txt", "hello wonderful world\n");
  zip.write_file("i-am-b.txt", "this is another file\n");
  zip.write_file("le-c-filet.txt", "je ne parle par\n");
  zip.write_cd();
}

int main() {
  create_zip();
  return system("unzip -c out/test.zip");
}
