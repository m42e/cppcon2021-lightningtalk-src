#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <type_traits>

#include <cxxabi.h>

#define flag_a 0x0011001100110011ULL
#define flag_b 0xFF11001100110011ULL

std::map<uint64_t, std::uint16_t> const v{
    {flag_a, 123U},
    {flag_b, 555U},
};



template <typename T, typename Current, typename... Left>
auto Apply(std::optional<T> value) {
  if constexpr (sizeof...(Left) > 0) {
    return Apply<typename decltype(Current::Apply(value))::value_type, Left...>(
        Current::Apply(value));
  } else {
    return Current::Apply(value);
  }
};

struct ParameterOption {};
struct ParameterLoadOption : ParameterOption {};

template <uint64_t ID, typename OriginalType = std::uint64_t, typename LoadType = OriginalType>
struct Load : ParameterLoadOption{
  using BaseType = OriginalType;
  static std::optional<OriginalType> Apply(std::optional<OriginalType> value) {
    if (v.find(ID) != v.end()) {
      return std::optional<OriginalType>{static_cast<LoadType>(v.at(ID))};
    }
    return std::nullopt;
  }
};

template <auto MinValue, typename T = decltype(MinValue)>
struct Min : ParameterOption{
  template <typename OriginalType>
  static std::optional<OriginalType> Apply(std::optional<OriginalType> value) {
    static_assert(std::is_convertible_v<OriginalType, T> ||
                      std::is_constructible_v<OriginalType, T>,
                  "MinValue not compatible with value type");
    return value ? std::max(static_cast<OriginalType>(MinValue), value.value())
                 : value;
  }
};

template <auto MaxValue, typename T = decltype(MaxValue)>
struct Max : ParameterOption{
  template <typename OriginalType>
  static std::optional<OriginalType> Apply(std::optional<OriginalType> value) {
    static_assert(std::is_convertible_v<OriginalType, T> ||
                      std::is_constructible_v<OriginalType, T>,
                  "MaxValue not compatible with value type");
    return value ? std::min(static_cast<OriginalType>(MaxValue), value.value())
                 : value;
  }
};

template <auto DefaultValue, typename T = decltype(DefaultValue)>
struct Default : ParameterLoadOption{
  template <typename OriginalType>
  static std::optional<OriginalType> Apply(std::optional<OriginalType> value) {
    return (!value) ? static_cast<OriginalType>(DefaultValue) : value;
  }
};

template <auto OffsetValue, typename T = decltype(OffsetValue)>
struct Offset : ParameterOption{
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
struct Ratio : ParameterOption{
  template <typename OriginalType>
  static std::optional<OriginalType> Apply(std::optional<OriginalType> value) {
    return (value) ? static_cast<OriginalType>((value.value() * Nominator) /
                                               Denominator)
                   : value;
  }
};

template <bool ptype, typename option, typename... options>
class Parameter_impl;
template <typename option, typename... options>
class Parameter_impl<true, option, options...> {
  using OriginalType = typename option::BaseType;
  OriginalType value_;

 public:
  Parameter_impl() : value_{Process()} {}
  explicit operator OriginalType() { return value_; }
  auto Value() -> OriginalType { return value_; }
  auto Process() -> OriginalType {
    std::optional<OriginalType> value;
    value = Apply<typename decltype(option::Apply(value))::value_type, option,
                  options...>(value);
    return (!value.has_value()) ? OriginalType{} : OriginalType{value.value()};
  }
};


template <typename option, typename... options>
class Parameter_impl<false, option, options...> {
  using OriginalType = option;
  using FirstEntityType = std::tuple_element_t<0, std::tuple<options...>>;
  using FirstType = typename FirstEntityType::BaseType;
  OriginalType value_;

 public:
  Parameter_impl() : value_{Process()} {}
  explicit operator OriginalType() { return value_; }
  auto Value() -> OriginalType { return value_; }
  auto Process() -> OriginalType {
    std::optional<FirstType> value;
    value = Apply<FirstType, options...>(value);
    return (!value.has_value()) ? OriginalType{} : OriginalType{value.value()};
  }
};

template <typename T, typename ...Ts>
using Parameter = Parameter_impl<std::is_base_of_v<ParameterLoadOption, T>, T, Ts...>;


class MyClass{
    double value;
  public:
    MyClass()=default;
    MyClass(double v): value{v} {};

    friend std::ostream& operator<<(std::ostream& os, MyClass const& self) {
       os << "MyClassA [value=" << self.value << "]";
       return os;
    }
};


int main() {
  std::cout << "Stored Values:\n=============\n";
  std::cout << "flab_a = " << v.at(flag_a) << "\n";
  std::cout << "flab_b = " << v.at(flag_b) << "\n\n";


  {
    Parameter<Load<flag_b, std::uint16_t, std::uint8_t>> t;
    std::cout << "Parameter<Load<flag_b, std::uint16_t, std::uint8_t>> t\n    ";
    auto vv = t.Value();
    std::cout << t.Value() << " is ";

    int status = -4;

    std::unique_ptr<char, void (*)(void*)> res{
        abi::__cxa_demangle(typeid(vv).name(), NULL, NULL, &status), std::free};
    std::cout << res << "\n";
  }
  {
    Parameter<Load<flag_b, std::uint16_t>> t;
    std::cout << "Parameter<Load<flag_b, std::uint16_t>> t\n    ";
    auto vv = t.Value();
    std::cout << t.Value() << " is ";

    int status = -4;

    std::unique_ptr<char, void (*)(void*)> res{
        abi::__cxa_demangle(typeid(vv).name(), NULL, NULL, &status), std::free};
    std::cout << res << "\n";
  }
  {
    Parameter<Load<flag_a, std::uint16_t>> t;
    std::cout << "Parameter<Load<flag_a, std::uint16_t>> t\n    ";
    std::cout << t.Value() << std::endl;
  }
  {
    Parameter<Load<flag_a, std::uint16_t>, Min<160U>> t;
    std::cout << "Parameter<Load<flag_a, std::uint16_t>, Min<160U>> t\n    ";
    std::cout << t.Value() << std::endl;
  }
  {
    Parameter<Load<flag_a, std::uint16_t>, Default<160U>> t;
    std::cout << "Parameter<Load<flag_a, std::uint16_t>, Default<160U>> t\n    ";
    std::cout << t.Value() << std::endl;
  }
  {
    Parameter<Load<flag_a, std::uint16_t>, Max<88U>> t;
    std::cout << "Parameter<Load<flag_a, std::uint16_t>, Max<88U>> t\n    ";
    std::cout << t.Value() << std::endl;
  }
  {
    Parameter<Load<flag_a, std::uint16_t>, Min<88U>, Max<160U>> t;
    std::cout << "Parameter<Load<flag_a, std::uint16_t>, Min<88U>, Max<160U>> t\n    ";
    std::cout << t.Value() << std::endl;
  }
  {
    Parameter<Load<1234, std::uint16_t>, Default<123345U>, Min<1234U>, Max<160U>> t;
    std::cout << "Parameter<Load<1234, std::uint16_t>, Default<123345U>, Min<1234U>, Max<160U>> t\n    ";
    std::cout << t.Value() << std::endl;
  }
  {
    Parameter<Load<flag_b, std::uint16_t>, Default<1230U>, Ratio<1, 10>, Max<160U>> t;
    std::cout << "Parameter<Load<flag_b, std::uint16_t>, Default<1230U>, Ratio<1, 10>, Max<160U>> t\n    ";
    std::cout << t.Value() << std::endl;
  }
  {
    Parameter<std::chrono::microseconds, Load<flag_b+1, uint16_t>, Default<1230U>, Ratio<1, 10>, Max<160U>> t;
    std::cout << "Parameter<std::chrono::microseconds, Load<flag_b+1, uint16_t>, Default<1230U>, Ratio<1, 10>, Max<160U>> t\n    ";
    std::cout << t.Value().count() << std::endl;
  }

  {
    Parameter<std::chrono::microseconds, Load<flag_b, uint16_t>, Default<1230U>, Ratio<1, 10>, Max<160U>> t;
    std::cout << "Parameter<std::chrono::microseconds, Load<flag_b, uint16_t>, Default<1230U>, Ratio<1, 10>, Max<160U>> t\n    ";
    std::cout << t.Value().count() << std::endl;
  }
  {
    Parameter<double, Load<flag_b, double, uint16_t>, Default<1230U>, Ratio<1, 100>, Max<160U>> t;
    std::cout << "Parameter<double, Load<flag_b, double, uint16_t>, Default<1230U>, Ratio<1, 100>, Max<160U>> t\n    ";
    std::cout << t.Value() << std::endl;
  }
  {
    Parameter<double, Load<flag_b, double, uint16_t>, Default<1230U>, Ratio<1, 100000>, Max<160U>> t;
    std::cout << "Parameter<double, Load<flag_b, double, uint16_t>, Default<1230U>, Ratio<1, 100000>, Max<160U>> t\n    ";
    std::cout << t.Value() << std::endl;
  }
  {
    Parameter<MyClass, Load<flag_b, double, uint16_t>, Default<1230U>, Ratio<1, 100000>, Max<160U>> t;
    std::cout << "Parameter<MyClass, Load<flag_b, double, uint16_t>, Default<1230U>, Ratio<1, 100000>, Max<160U>> t\n    ";
    std::cout << t.Value() << std::endl;;
  }

  return 0;
}
