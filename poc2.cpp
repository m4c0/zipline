#pragma leco test
#define MCT_SYSCALL_IMPLEMENTATION
#include "../mct/mct-syscall.h"
#include <stdio.h>
#include <stdlib.h>

import hai;
import hay;
import jute;
import mtime;
import print;
import traits;
import zipline;

#ifdef LECO_TARGET_WINDOWS
// Clashes on non-Windows
using namespace traits::ints;
#endif

class reader : public zipline::reader {
  hay<FILE *, mct_syscall_fopen, fclose> f { "out/read-test.zip", "rb" };

public:
  // TODO: check for errors
  void seek(unsigned ofs) override { fseek(f, ofs, SEEK_SET); }
  void read(void * ptr, unsigned sz) override { fread(ptr, sz, 1, f); }
  uint32_t size() override {
    fseek(f, 0, SEEK_END);
    return ftell(f);
  }

  void fail(jute::view err) override { die(err); }
};


int main() {
  if (!mtime::of("out/read-test.zip")) system("zip out/read-test.zip *.cpp");

  reader r {};
  for (auto & f : zipline::list(&r)) {
    putln(f.name);
    putln(zipline::read(&r, f));
  }
  return 0;
}
