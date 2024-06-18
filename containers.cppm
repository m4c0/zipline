module;
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <utility>

export module containers;

// Copied from my "STL" repo. Can be replaced by C++23's constexpr
// unique_ptr<T[]>
export namespace containers {
struct out_of_bounds : std::runtime_error {
  out_of_bounds() : runtime_error("Attempt at read past of bounded array") {}
};

// constexpr variant of STL's
template <typename Tp> class unique_array {
  Tp *m_ptr{};
  size_t m_len{};

public:
  constexpr unique_array() noexcept = default;
  constexpr explicit unique_array(size_t n) noexcept
      : m_ptr{new Tp[n]}, m_len{n} {}
  constexpr ~unique_array() noexcept { delete[] m_ptr; }

  constexpr unique_array(const unique_array &) = delete;
  constexpr unique_array &operator=(const unique_array &) = delete;

  constexpr unique_array(unique_array &&o) noexcept
      : m_ptr{o.m_ptr}, m_len{o.m_len} {
    o.m_ptr = nullptr;
    o.m_len = 0;
  }
  constexpr unique_array &operator=(unique_array &&o) noexcept {
    delete[] m_ptr;
    m_ptr = o.m_ptr;
    m_len = o.m_len;
    o.m_ptr = nullptr;
    o.m_len = 0;
    return *this;
  }

  [[nodiscard]] constexpr Tp &at(size_t idx) {
    if (idx >= m_len)
      throw out_of_bounds{};
    return m_ptr[idx]; // NOLINT
  }
  [[nodiscard]] constexpr const Tp &at(size_t idx) const {
    if (idx >= m_len)
      throw out_of_bounds{};
    return m_ptr[idx]; // NOLINT
  }

  [[nodiscard]] constexpr explicit operator bool() const noexcept {
    return m_ptr != nullptr;
  }

  [[nodiscard]] constexpr auto size() const noexcept { return m_len; }

  [[nodiscard]] constexpr Tp &operator[](size_t idx) noexcept {
    assert(idx < m_len);
    return m_ptr[idx]; // NOLINT
  }
  [[nodiscard]] constexpr const Tp &operator[](size_t idx) const noexcept {
    assert(idx < m_len);
    return m_ptr[idx]; // NOLINT
  }

  [[nodiscard]] constexpr auto begin() noexcept { return m_ptr; }
  [[nodiscard]] constexpr auto end() noexcept { return m_ptr + m_len; }
  [[nodiscard]] constexpr auto begin() const noexcept { return m_ptr; }
  [[nodiscard]] constexpr auto end() const noexcept { return m_ptr + m_len; }
};
} // namespace containers

using containers::unique_array;

using uaint = unique_array<int>;

static_assert(!uaint());
static_assert(uaint().size() == 0);
static_assert(uaint(3));
static_assert(uaint(3).size() == 3);

static constexpr auto algo_gen() noexcept {
  uaint res{3};
  std::generate(std::begin(res), std::end(res),
                [i = 0]() mutable noexcept { return ++i; });
  return res;
}
static constexpr auto idx_gen() noexcept {
  uaint res{3};
  res[0] = 1;
  res[1] = 2;
  res[2] = 3;
  return res;
}
static constexpr auto equals(const uaint &a, const uaint &b) noexcept {
  return std::equal(a.begin(), a.end(), b.begin(), b.end());
}
static_assert(equals(algo_gen(), idx_gen()));

static_assert([] {
  uaint a = algo_gen();
  uaint b = std::move(a);
  return equals(b, idx_gen());
}());
