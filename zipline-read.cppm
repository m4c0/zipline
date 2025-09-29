export module zipline:read;
import traits;

using namespace traits::ints;

namespace zipline {
  export struct reader {
    virtual constexpr void seek(uint32_t) = 0;
    virtual constexpr void read(void *, unsigned) = 0;
    virtual constexpr uint32_t size() = 0;

    template<typename T> constexpr T obj_read() {
      T t {};
      read(&t, sizeof(T));
      return t;
    }
  };
}
