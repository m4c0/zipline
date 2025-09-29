#pragma leco tool

import hai;
import jojo;
import jute;
import print;
import zipline;

using namespace traits::ints;

struct reader : zipline::reader {
  hai::cstr data;
  unsigned pos {};

  // TODO: check for errors
  void seek(unsigned ofs) override { pos = ofs; }
  void read(void * ptr, unsigned sz) override {
    for (auto i = 0; i < sz; i++) static_cast<char *>(ptr)[i] = data.data()[pos++];
  }
  uint32_t size() override { return data.size(); }

  void fail(jute::view err) override { die(err); }
};


int main(int argc, char **argv) {
  for (auto i = 1; i < argc; i++) {
    reader r {};
    r.data = jojo::read_cstr(jute::view::unsafe(argv[i]));

    for (auto e : zipline::list(&r)) putln(e.name);
  }
}
