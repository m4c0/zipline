export module zipline:eocd;
import :common;
import traits;
import yoyo;

using namespace traits::ints;

namespace zipline {
static constexpr const uint32_t eocd_magic_number = 0x06054b50; // PK\5\6
static constexpr const auto eocd_len = 22;

static constexpr mno::req<void> find_eocd_start(yoyo::reader *r) {
  return r->seekg(-eocd_len, yoyo::seek_mode::end)
      .fmap([&] {
        while (true) {
          auto u32 = r->read_u32();
          if (!u32.is_valid())
            return u32.map([](auto) {});

          if (u32.unwrap(0) == eocd_magic_number)
            return mno::req<void>{};

          auto res = r->seekg(-sizeof(uint32_t) - 1, yoyo::seek_mode::current);
          if (!res.is_valid())
            return res;
        }
      })
      .trace("searching for end-of-central-directory");
}

export [[nodiscard]] constexpr auto read_eocd(yoyo::reader *r) {
  unwrap<missing_eocd_error>(find_eocd_start(r));

  constexpr const uint16_t zip64_magic = 0xFFFF;
  auto disk_no = unwrap<truncated_eocd_error>(r->read_u16());
  if (disk_no == zip64_magic)
    throw zip64_is_unsupported{};
  if (disk_no != 0)
    throw multidisk_is_unsupported{};

  auto cdir_disk = unwrap<truncated_eocd_error>(r->read_u16());
  if (cdir_disk != 0)
    throw multidisk_is_unsupported{};

  auto cdir_count_disk = unwrap<truncated_eocd_error>(r->read_u16());
  auto cdir_total_count = unwrap<truncated_eocd_error>(r->read_u16());
  if (cdir_count_disk != cdir_total_count)
    throw multidisk_is_unsupported{};

  auto cdir_size = unwrap<truncated_eocd_error>(r->read_u32());
  auto cdir_offset = unwrap<truncated_eocd_error>(r->read_u32());

  return mno::req{cdir_meta{
      .count = cdir_total_count,
      .size = cdir_size,
      .offset = cdir_offset,
  }};
}
} // namespace zipline

// End-of-central-directory has an extra comment to test the reverse seek and
// extra padding at front
static constexpr auto eocd_data = yoyo::ce_reader{
    "\x50\x4b\x01\x02\x00" // pad
    "\x50\x4b\x05\x06\x00\x00\x00\x00"
    "\x01\x00\x01\x00\x4e\x00\x00\x00"
    "\x42\x00\x00\x00\x00\x03"
    "---" // pad
};

using namespace zipline;

static_assert([] {
  constexpr const auto after_pk56_pos = 5 + 4; // PAD + u32

  auto r = eocd_data;
  unwrap<int>(find_eocd_start(&r));
  return r.tellg() == after_pk56_pos;
}());
static_assert([] {
  constexpr const auto cdm_size = 0x4e;
  constexpr const auto cdm_offset = 0x42;

  auto r = eocd_data;
  return read_eocd(&r)
      .map([](auto &cdm) {
        return cdm.count == 1 && cdm.size == cdm_size &&
               cdm.offset == cdm_offset;
      })
      .unwrap(false);
}());
