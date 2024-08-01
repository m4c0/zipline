#pragma leco tool
#include <stdlib.h>

import silog;
import yoyo;
import zipline;

static auto create_zip() {
  static constexpr zipline::cdfh cd{
      .name = "test.txt",
      .crc32 = 0,
      .offset = 0,
      .compressed_size = 0,
      .uncompressed_size = 0,
  };
  unsigned cd_ofs{};
  return yoyo::file_writer::open("out/test.zip")
      .fpeek([](auto &w) { return zipline::write_lfh(w, cd); })
      .fpeek(
          [&](auto &w) { return w.tellp().map([&](auto p) { cd_ofs = p; }); })
      .fpeek([](auto &w) { return zipline::write_cd(w, cd); })
      .fmap([&](auto &w) {
        return w.tellp().fmap([&](auto p) {
          auto sz = p - cd_ofs;
          return zipline::write_eocd(w, 1, cd_ofs, sz);
        });
      });
}

int main() {
  return create_zip()
      .map([] { return system("unzip -t out/test.zip"); })
      .log_error();
}
