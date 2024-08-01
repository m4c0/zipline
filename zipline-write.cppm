export module zipline:write;
import yoyo;

namespace zipline {
export auto write_cd(yoyo::writer &w) { return mno::req{}; }
export auto write_eocd(yoyo::writer &w, unsigned offset, unsigned size) {
  return w
      .write_u32(0x06054b50)                     // PK56
      .fmap([&] { return w.write_u16(0); })      // Disk
      .fmap([&] { return w.write_u16(0); })      // Disk where CD exist
      .fmap([&] { return w.write_u16(0); })      // Number of CD entries on disk
      .fmap([&] { return w.write_u16(0); })      // Total number of CD entries
      .fmap([&] { return w.write_u32(size); })   // Size of CD
      .fmap([&] { return w.write_u32(offset); }) // Offset of CD
      .fmap([&] { return w.write_u16(0); })      // Size of comment
      ;
}
} // namespace zipline
