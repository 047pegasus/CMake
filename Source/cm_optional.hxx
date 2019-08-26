/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cm_optional_hxx
#define cm_optional_hxx

#include "cmConfigure.h" // IWYU pragma: keep

#if __cplusplus >= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#  define CMake_HAVE_CXX_OPTIONAL
#endif

#if defined(CMake_HAVE_CXX_OPTIONAL)
#  include <optional>
#else
#  include "cm_utility.hxx"
#  include <memory>
#  include <utility>
#endif

namespace cm {

#if defined(CMake_HAVE_CXX_OPTIONAL)

using std::nullopt_t;
using std::nullopt;
using std::optional;
using std::bad_optional_access;
using std::make_optional;

#else

class bad_optional_access : public std::exception
{
  using std::exception::exception;
};

struct nullopt_t
{
  explicit constexpr nullopt_t(int) {}
};

constexpr nullopt_t nullopt{ 0 };

template <typename T>
class optional
{
public:
  using value_type = T;

  optional() noexcept = default;
  optional(nullopt_t) noexcept;
  optional(const optional& other);
  optional(optional&& other) noexcept;

  template <typename... Args>
  explicit optional(cm::in_place_t, Args&&... args);

  template <
    typename U = T,
    typename = typename std::enable_if<
      std::is_constructible<T, U&&>::value &&
      !std::is_same<typename std::decay<U>::type, cm::in_place_t>::value &&
      !std::is_same<typename std::decay<U>::type,
                    cm::optional<T>>::value>::type>
  optional(U&& v);

  ~optional();

  optional& operator=(nullopt_t) noexcept;
  optional& operator=(const optional& other);
  optional& operator=(optional&& other) noexcept;

  template <
    typename U = T,
    typename = typename std::enable_if<
      !std::is_same<typename std::decay<U>::type, cm::optional<T>>::value &&
      std::is_constructible<T, U>::value && std::is_assignable<T&, U>::value &&
      (!std::is_scalar<T>::value ||
       !std::is_same<typename std::decay<U>::type, T>::value)>::type>
  optional& operator=(U&& v);

  const T* operator->() const;
  T* operator->();
  const T& operator*() const&;
  T& operator*() &;
  const T&& operator*() const&&;
  T&& operator*() &&;

  explicit operator bool() const noexcept;
  bool has_value() const noexcept;

  T& value() &;
  const T& value() const&;

  T&& value() &&;
  const T&& value() const&&;

  template <typename U>
  T value_or(U&& default_value) const&;

  template <typename U>
  T value_or(U&& default_value) &&;

  void swap(optional& other) noexcept;
  void reset() noexcept;

  template <typename... Args>
  T& emplace(Args&&... args);

private:
  bool _has_value = false;
  std::allocator<T> _allocator;
  union _mem_union
  {
    T value;

    // Explicit constructor and destructor is required to make this work
    _mem_union() noexcept {}
    ~_mem_union() noexcept {}
  } _mem;
};

template <typename T>
optional<typename std::decay<T>::type> make_optional(T&& value)
{
  return optional<typename std::decay<T>::type>(std::forward<T>(value));
}

template <typename T, class... Args>
optional<T> make_optional(Args&&... args)
{
  return optional<T>(in_place, std::forward<Args>(args)...);
}

template <typename T>
optional<T>::optional(nullopt_t) noexcept
{
}

template <typename T>
optional<T>::optional(const optional& other)
{
  *this = other;
}

template <typename T>
optional<T>::optional(optional&& other) noexcept
{
  *this = std::move(other);
}

template <typename T>
template <typename... Args>
optional<T>::optional(cm::in_place_t, Args&&... args)
{
  this->emplace(std::forward<Args>(args)...);
}

template <typename T>
template <typename U, typename>
optional<T>::optional(U&& v)
{
  this->emplace(std::forward<U>(v));
}

template <typename T>
optional<T>::~optional()
{
  this->reset();
}

template <typename T>
optional<T>& optional<T>::operator=(nullopt_t) noexcept
{
  this->reset();
  return *this;
}

template <typename T>
optional<T>& optional<T>::operator=(const optional& other)
{
  if (other.has_value()) {
    if (this->has_value()) {
      this->value() = *other;
    } else {
      this->emplace(*other);
    }
  } else {
    this->reset();
  }
  return *this;
}

template <typename T>
optional<T>& optional<T>::operator=(optional&& other) noexcept
{
  if (other.has_value()) {
    if (this->has_value()) {
      this->value() = std::move(*other);
    } else {
      this->emplace(std::move(*other));
    }
  } else {
    this->reset();
  }
  return *this;
}

template <typename T>
template <typename U, typename>
optional<T>& optional<T>::operator=(U&& v)
{
  if (this->has_value()) {
    this->value() = v;
  } else {
    this->emplace(std::forward<U>(v));
  }
  return *this;
}

template <typename T>
const T* optional<T>::operator->() const
{
  return &**this;
}

template <typename T>
T* optional<T>::operator->()
{
  return &**this;
}

template <typename T>
const T& optional<T>::operator*() const&
{
  return this->_mem.value;
}

template <typename T>
T& optional<T>::operator*() &
{
  return this->_mem.value;
}

template <typename T>
const T&& optional<T>::operator*() const&&
{
  return std::move(**this);
}

template <typename T>
T&& optional<T>::operator*() &&
{
  return std::move(**this);
}

template <typename T>
bool optional<T>::has_value() const noexcept
{
  return this->_has_value;
}

template <typename T>
optional<T>::operator bool() const noexcept
{
  return this->has_value();
}

template <typename T>
T& optional<T>::value() &
{
  if (!this->has_value()) {
    throw cm::bad_optional_access{};
  }
  return **this;
}

template <typename T>
const T& optional<T>::value() const&
{
  if (!this->has_value()) {
    throw cm::bad_optional_access{};
  }
  return **this;
}

template <typename T>
template <typename U>
T optional<T>::value_or(U&& default_value) const&
{
  return bool(*this) ? **this : static_cast<T>(std::forward<U>(default_value));
}

template <typename T>
template <typename U>
T optional<T>::value_or(U&& default_value) &&
{
  return bool(*this) ? std::move(**this)
                     : static_cast<T>(std::forward<U>(default_value));
}

template <typename T>
void optional<T>::swap(optional& other) noexcept
{
  if (this->has_value()) {
    if (other.has_value()) {
      using std::swap;
      swap(**this, *other);
    } else {
      other.emplace(std::move(**this));
      this->reset();
    }
  } else if (other.has_value()) {
    this->emplace(std::move(*other));
    other.reset();
  }
}

template <typename T>
void optional<T>::reset() noexcept
{
  if (this->has_value()) {
    this->_has_value = false;
    std::allocator_traits<std::allocator<T>>::destroy(this->_allocator,
                                                      &**this);
  }
}

template <typename T>
template <typename... Args>
T& optional<T>::emplace(Args&&... args)
{
  this->reset();
  std::allocator_traits<std::allocator<T>>::construct(
    this->_allocator, &**this, std::forward<Args>(args)...);
  this->_has_value = true;
  return this->value();
}

#endif
}

#endif
