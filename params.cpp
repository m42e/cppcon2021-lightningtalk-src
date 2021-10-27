
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>

#define flag_a 0x0011001100110011ULL
#define flag_b 0xFF11001100110011ULL

std::map<uint64_t, std::uint16_t> const v{
    {flag_a, 123U},
    {flag_b, 555U},
};

template <uint64_t ID>
struct Load {
  template <typename OriginalType>
  static std::optional<OriginalType> Apply(std::optional<OriginalType> value) {
    if (v.find(ID) != v.end()) {
      value = static_cast<OriginalType>(v.at(ID));
    }
    return value;
  }
};

template <auto MinValue, typename T = decltype(MinValue)>
struct Min {
  template <typename OriginalType>
  static std::optional<OriginalType> Apply(std::optional<OriginalType> value) {
    return value ? std::max(static_cast<OriginalType>(MinValue), value.value())
                 : value;
  }
};

template <auto MaxValue, typename T = decltype(MaxValue)>
struct Max {
  template <typename OriginalType>
  static std::optional<OriginalType> Apply(std::optional<OriginalType> value) {
    return value ? std::min(static_cast<OriginalType>(MaxValue), value.value())
                 : value;
  }
};

template <auto DefaultValue, typename T = decltype(DefaultValue)>
struct Default {
  template <typename OriginalType>
  static std::optional<OriginalType> Apply(std::optional<OriginalType> value) {
    return (!value) ? static_cast<OriginalType>(DefaultValue) : value;
  }
};

template <auto OffsetValue, typename T = decltype(OffsetValue)>
struct Offset {
  template <typename OriginalType>
  static std::optional<OriginalType> Apply(std::optional<OriginalType> value) {
    return (value) ? static_cast<OriginalType>(value + OffsetValue) : value;
  }
};

template <
    auto Nominator, auto Denominator, typename T = decltype(Nominator),
    typename T2 = decltype(Denominator),
    std::enable_if_t<std::is_convertible_v<T, T2> or std::is_same_v<T, T2>,
                     bool> = true>
struct Ratio {
  template <typename OriginalType>
  static std::optional<OriginalType> Apply(std::optional<OriginalType> value) {
    return (value) ? static_cast<OriginalType>((value.value() * Nominator) /
                                               Denominator)
                   : value;
  }
};

template <typename T, typename Current, typename... Left>
auto Apply(std::optional<T> value) -> std::optional<T> {
  if constexpr (sizeof...(Left) > 0) {
    return Apply<T, Left...>(Current::Apply(value));
  } else {
    return Current::Apply(value);
  }
}

template <typename T, typename... options>
class Parameter {
  T value_;

 public:
  Parameter() : value_{Load()} {}
  explicit operator T() { return value_; }
  auto Value() -> T { return value_; }
  auto Load() -> T {
    std::optional<T> value;
    value = Apply<T, options...>(value);
    return (!value.has_value()) ? T{} : value.value();
  }
};

int main() {
  {
    Parameter<std::uint16_t, Load<flag_a>> t;
    std::cout << t.Value() << std::endl;
  }
  {
    Parameter<std::uint16_t, Load<flag_a>, Min<160U>> t;
    std::cout << t.Value() << std::endl;
  }
  {
    Parameter<std::uint16_t, Load<flag_a>, Max<88U>> t;
    std::cout << t.Value() << std::endl;
  }
  {
    Parameter<std::uint16_t, Load<flag_a>, Min<88U>, Max<160U>> t;
    std::cout << t.Value() << std::endl;
  }
  {
    Parameter<std::uint16_t, Load<1234>, Default<123345U>, Min<1234U>,
              Max<160U>>
        t;
    std::cout << t.Value() << std::endl;
  }
  {
    Parameter<std::uint16_t, Load<flag_b>, Default<1230U>, Ratio<1, 10>,
              Max<160U>>
        t;
    std::cout << t.Value() << std::endl;
  }

  return 0;
}
