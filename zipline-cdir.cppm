export module zipline:cdir;
import :common;
import hai;
import jute;
import yoyo;

namespace zipline {
static constexpr const auto cdir_magic = 0x02014b50; // PK\1\2

[[nodiscard]] static constexpr auto check_cdir(yoyo::reader &r) {
  return r.read_u32()
      .assert([](auto v) { return v == cdir_magic; },
              "invalid central dir magic number")
      .map([](auto) {});
}
[[nodiscard]] static constexpr auto check_version(yoyo::reader &r) {
  return r.read_u16()
      .assert([](auto v) { return v <= maximum_supported_version; },
              "zip version is greater than supported")
      .map([](auto) {});
}
[[nodiscard]] static constexpr auto check_disk(yoyo::reader &r) {
  return r.read_u16()
      .assert([](auto v) { return v == 0; }, "multidisk is not supported")
      .map([](auto) {});
}

[[nodiscard]] static constexpr auto read_str(uint16_t &len,
                                             hai::array<uint8_t> &str) {
  return [&](yoyo::reader &r) {
    str.set_capacity(len);
    return r.read(str.begin(), str.size());
  };
}

[[nodiscard]] static constexpr auto skip_u16(yoyo::reader &r) {
  return r.read_u16().map([](auto) {});
}
[[nodiscard]] static constexpr auto skip_u32(yoyo::reader &r) {
  return r.read_u32().map([](auto) {});
}

[[nodiscard]] constexpr auto read_cd(yoyo::subreader &r) {
  cdir_entry result{};

  uint16_t filename_len{};
  uint16_t extra_len{};
  uint16_t comment_len{};

  return mno::req{r}
      .fpeek(check_cdir)
      .fpeek(skip_u16) // version made by
      .fpeek(check_version)
      .fpeek(yoyo::read_u16(result.flags))
      .fpeek(yoyo::read_u16(result.method))
      .fpeek(skip_u16) // modification time
      .fpeek(skip_u16) // modification date
      .fpeek(yoyo::read_u32(result.crc))
      .fpeek(yoyo::read_u32(result.compressed_size))
      .fpeek(yoyo::read_u32(result.uncompressed_size))
      .fpeek(yoyo::read_u16(filename_len))
      .fpeek(yoyo::read_u16(extra_len))
      .fpeek(yoyo::read_u16(comment_len))
      .fpeek(check_disk)
      .fpeek(skip_u16) // internal file attr
      .fpeek(skip_u32) // external file attr
      .fpeek(yoyo::read_u32(result.offset))
      .fpeek(read_str(filename_len, result.filename))
      .fpeek(read_str(extra_len, result.extra))
      .fpeek(yoyo::seekg(comment_len, yoyo::seek_mode::current))
      .map([&](auto &r) { return traits::move(result); });
}
} // namespace zipline

static constexpr auto cd_data = yoyo::ce_reader{
    "\x50\x4b\x01\x02"                 // magic
    "\x1e\x03\x14\x00"                 // version (by + needed)
    "\x00\x00\x08\x00"                 // flags + method
    "\xb7\x98\x2d\x54"                 // modification time + date
    "\xba\x06\x4c\xba"                 // crc32
    "\x80\x01\x00\x00\x0e\x03\x00\x00" // compressed + uncompressed sizes
    "\x0c\x00\x18\x00\x00\x00"         // sizes (file, extra, comment)
    "\x00\x00\x01\x00\x00\x00\xa4\x81" // disk + attrs
    "\x42\x00\x00\x00"                 // offset

    "\x7a\x69\x70\x2e\x65\x6f\x63\x64\x2e\x63\x70\x70" // filename

    // extra
    "\x55\x54\x05\x00\x03\x0a\x78\xe0\x61\x75\x78\x0b\x00\x01\x04\xf6\x01\x00"
    "\x00\x04\x14\x00\x00\x00"};

using namespace zipline;

static_assert([] {
  constexpr const auto comp_size = 0x180;
  constexpr const auto uncomp_size = 0x030e;
  constexpr const auto crc = 0xba4c06baU;
  constexpr const auto offset = 0x42;
  constexpr const auto extra_size = 0x18;
  constexpr const jute::view filename = "zip.eocd.cpp";

  constexpr const auto assert = [](auto res, auto exp) {
    if (res != exp)
      throw 0;
  };

  auto r = cd_data;

  return r.size()
      .fmap(
          [&](auto sz) { return yoyo::subreader::seek_and_create(&r, 0, sz); })
      .fmap([](auto sr) { return read_cd(sr); })
      .map([&](auto &cd) {
        assert(cd.compressed_size, comp_size);
        assert(cd.uncompressed_size, uncomp_size);
        assert(cd.crc, crc);
        assert(cd.offset, offset);
        assert(cd.extra.size(), extra_size);
        if (!filename_matches(cd, filename))
          throw 0;
      })
      .fmap([&] { return r.eof(); })
      .unwrap(false);
}());
