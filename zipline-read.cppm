export module zipline:read;
import :objects;
import hai;
import hay;
import jute;
import traits;

using namespace traits::ints;

namespace zipline {
  export struct file_entry {
    jute::heap name;
    uint32_t offset;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint32_t crc32;
    comp_method method;
  };

  export struct reader {
    virtual constexpr void seek(uint32_t) = 0;
    virtual constexpr void read(void *, unsigned) = 0;
    virtual constexpr uint32_t size() = 0;
    virtual constexpr void fail(jute::view err) = 0;

    template<typename T> constexpr T obj_read() {
      T t {};
      read(&t, sizeof(T));
      return t;
    }
  };

  export hai::array<file_entry> list(reader * r) {
    const auto fail = [&](jute::view msg) {
      r->fail(msg);
      return hai::array<file_entry> {};
    };

    auto fsize = r->size();

    eocd e {};
    for (int64_t p = fsize - sizeof(eocd); p >= 0; p--) {
      r->seek(p);
      e = r->obj_read<eocd>();
      if (e.magic == eocd_magic) break;
    }
    if (e.magic != eocd_magic)
      return fail("end-of-central-directory not found");
    if (e.disk != 0 || e.cd_disk != 0)
      return fail("multi-disk is not supported");

    hai::array<file_entry> res { e.cd_entries };
    file_entry * entry = res.begin();

    hay<char[]> name_buf { 0xFFFF };
    hay<char[]> name2_buf { 0xFFFF };
    unsigned offs = 0;
    while (offs < e.cd_size) {
      r->seek(e.cd_offset + offs);
      auto c = r->obj_read<cdfh>();
      if (c.magic != cdfh_magic)
        return fail("invalid entry in central-directory");
      if (c.min_version > 20)
        return fail("file contains unsupported version");
      if (c.disk != 0)
        return fail("multi-disk is not supported");

      r->read(static_cast<char *>(name_buf), c.name_size);
      jute::view name { name_buf, c.name_size };

      r->seek(c.rel_offset);
      auto h = r->obj_read<fh>();
      if (h.magic != fh_magic)
        return fail("invalid entry in file header");
      if (h.min_version > 20)
        return fail("file contains unsupported version");
      if (h.method != c.method)
        return fail("compression method differs between central-directory and file header");

      r->read(static_cast<char *>(name2_buf), h.name_size);
      if (name != jute::view { name2_buf, h.name_size })
        return fail("file name differs between central-directory and file header");

      uint32_t file_pos = c.rel_offset + sizeof(fh) + h.name_size + h.extra_size;
      if ((h.flags & 0x8) == 0x8) {
        if (h.crc32) fail("file with extra data descriptor should not have crc32 in its header");
        if (h.compressed_size) fail("file with extra data descriptor should not have compressed size in its header");
        if (h.uncompressed_size) fail("file with extra data descriptor should not have uncompressed size in its header");
        if (!c.compressed_size) fail("file with extra data descriptor and no compressed size in its central-directory");

        r->seek(file_pos + c.compressed_size);

        r->obj_read<uint32_t>();
        r->read(&h.crc32, 12);
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

      offs += sizeof(cdfh) + c.name_size + c.extra_size + c.comment_size;
    }

    return res;
  }

  export hai::array<char> read(reader * r, const file_entry & entry) {
    hai::array<char> buffer { entry.compressed_size };
    r->seek(entry.offset);
    r->read(buffer.begin(), buffer.size());

    switch (entry.method) {
      case zipline::comp_method::stored:
        return buffer;
      case zipline::comp_method::deflated:
        // TODO: deflate
        return {};
    }
  }
}
