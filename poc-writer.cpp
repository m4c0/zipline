#pragma leco tool
#include <stdlib.h>

import yoyo;
import zipline;

static auto create_zip() {
  return yoyo::file_writer::open("out/test.zip")
      .fpeek([](auto &w) { return zipline::write_cd(w); })
      .fmap([](auto &w) { return zipline::write_eocd(w, 0, 0); });
}

int main() {
  return create_zip()
      .map([] { return system("unzip -t out/test.zip"); })
      .log_error();
}
