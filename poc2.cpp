#pragma leco test
#define MCT_SYSCALL_IMPLEMENTATION
#include "../mct/mct-syscall.h"
#include <stdio.h>
#include <stdlib.h>

import hai;
import hay;
import jojo;
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
  system("zip -q out/read-test.zip *.cpp");

  reader r {};
  for (auto & f : zipline::list(&r)) {
    auto expected = jojo::read(f.name);
    auto got = zipline::read(&r, f);
    if (expected.size() != got.size()) putln(f.name, ": differs in size: ", expected.size(), " v ", got.size());
    else for (auto i = 0; i < got.size(); i++) {
      if (expected[i] == got[i]) continue;
      putln(f.name, ": differs at byte ", i);
      break;
    }
  }
  return 0;
}
