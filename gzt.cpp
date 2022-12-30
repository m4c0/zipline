#include <cassert>
#include <iostream>
#include <optional>

import bitstream;
import deflate;
import yoyo;

int main() {
  yoyo::istr_reader in{std::cin};

  // magic header
  assert(*in.read_u8() == 0x1FU);
  assert(*in.read_u8() == 0x8BU);
  // deflate
  assert(*in.read_u8() == 0x08U);

  auto flg = *in.read_u8();
  auto mtime = *in.read_u32();
  auto xfl = *in.read_u8();
  auto os = *in.read_u8();

  // extra field
  if (flg & 0b100) {
    auto xlen = *in.read_u16();
    assert(in.seekg(xlen, yoyo::seek_mode::current));
  }
  // original file name
  if (flg & 0b1000) {
    while (*in.read_u8()) {
    }
  }
  // comments
  if (flg & 0b10000) {
    while (*in.read_u8()) {
    }
  }
  // header crc
  if (flg & 0b10) {
    assert(in.read_u16());
  }

  zipline::bitstream b{&in};
  zipline::deflater def{&b};
  unsigned isize = 0;
  while (true) {
    auto n = def.next();
    if (!n) {
      if (def.last_block())
        break;
      def.set_next_block(&b);
      continue;
    }

    // std::cout.put(*n);
    isize++;
  }

  // crc32
  assert(in.read_u32());
  // uncompressed size
  auto exp_isize = *in.read_u32();

  std::cerr << "Decompressed " << isize << " bytes \n";
  std::cerr << "Expected " << exp_isize << "\n";
  assert(isize == exp_isize);
}
