#pragma leco tool

import jute;
import print;
import yoyo;
import zipline;

using namespace jute::literals;

static auto print(const zipline::cdir_entry &cdir) {
  putln(cdir.filename);
  return mno::req{};
}

static void list_zip(const char *name) {
  yoyo::file_reader::open(name)
      .fmap([](auto &r) { return zipline::list(r, print); })
      .trace(("processing "_s + jute::view::unsafe(name)).cstr())
      .log_error();
}

int main(int argc, char **argv) {
  for (auto i = 1; i < argc; i++) {
    list_zip(argv[i]);
  }
}
