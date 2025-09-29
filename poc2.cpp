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

struct file_entry {
  jute::heap name;
  uint32_t offset;
  uint32_t compressed_size;
  uint32_t uncompressed_size;
  uint32_t crc32;
  zipline::comp_method method;
};

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
};

static hai::array<file_entry> list() {
  const auto fail = [](jute::view err) { die(err); return hai::array<file_entry> {}; };

  reader r {};

  auto fsize = r.size();
  
  zipline::eocd e {};
  for (int64_t p = fsize - sizeof(zipline::eocd); p >= 0; p--) {
    r.seek(p);
    e = r.obj_read<zipline::eocd>();
    if (e.magic == zipline::eocd_magic) break;
  }
  if (e.magic != zipline::eocd_magic)
    return fail("end-of-central-directory not found");
  if (e.disk != 0 || e.cd_disk != 0)
    return fail("multi-disk is not supported");

  hai::array<file_entry> res { e.cd_entries };
  file_entry * entry = res.begin();

  hay<char[]> name_buf { 0xFFFF };
  hay<char[]> name2_buf { 0xFFFF };
  unsigned offs = 0;
  while (offs < e.cd_size) {
    r.seek(e.cd_offset + offs);
    auto c = r.obj_read<zipline::cdfh>();
    if (c.magic != zipline::cdfh_magic)
      return fail("invalid entry in central-directory");
    if (c.min_version > 20)
      return fail("file contains unsupported version");
    if (c.disk != 0)
      return fail("multi-disk is not supported");

    r.read(static_cast<char *>(name_buf), c.name_size);
    jute::view name { name_buf, c.name_size };

    r.seek(c.rel_offset);
    auto h = r.obj_read<zipline::fh>();
    if (h.magic != zipline::fh_magic)
      return fail("invalid entry in file header");
    if (h.min_version > 20)
      return fail("file contains unsupported version");
    if (h.method != c.method)
      return fail("compression method differs between central-directory and file header");

    r.read(static_cast<char *>(name2_buf), h.name_size);
    if (name != jute::view { name2_buf, h.name_size })
      return fail("file name differs between central-directory and file header");

    uint32_t file_pos = c.rel_offset + sizeof(zipline::fh) + h.name_size + h.extra_size;
    if ((h.flags & 0x8) == 0x8) {
      if (h.crc32) fail("file with extra data descriptor should not have crc32 in its header");
      if (h.compressed_size) fail("file with extra data descriptor should not have compressed size in its header");
      if (h.uncompressed_size) fail("file with extra data descriptor should not have uncompressed size in its header");
      if (!c.compressed_size) fail("file with extra data descriptor and no compressed size in its central-directory");

      r.seek(file_pos + c.compressed_size);

      r.obj_read<uint32_t>();
      r.read(&h.crc32, 12);
    }

    if (h.crc32 != c.crc32)
      return fail("crc32 differs between central-directory and file header");
    if (h.compressed_size != c.compressed_size)
      return fail("compressed size differs between central-directory and file header");
    if (h.uncompressed_size != c.uncompressed_size)
      return fail("uncompressed size differs between central-directory and file header");

    *entry++ = {
      .name              = jute::heap { name },
      .offset            = file_pos,
      .compressed_size   = h.compressed_size,
      .uncompressed_size = h.uncompressed_size,
      .method            = h.method,
    };

    offs += sizeof(zipline::cdfh) + c.name_size + c.extra_size + c.comment_size;
  }

  return res;
}

void read(const file_entry & entry) {
  reader r{};

  hay<char[]> buffer { entry.compressed_size };
  r.seek(entry.offset);
  r.read(static_cast<char *>(buffer), entry.compressed_size);

  switch (entry.method) {
    case zipline::comp_method::stored:
      putln(entry.name);
      putfn("%.*s", entry.compressed_size, static_cast<char *>(buffer));
      break;
    case zipline::comp_method::deflated:
      putln(entry.name, " deflated");
      break;
  }
}

int main() {
  if (!mtime::of("out/read-test.zip")) system("zip out/read-test.zip *.cpp");
  for (auto & f : list()) read(f);
  return 0;
}
