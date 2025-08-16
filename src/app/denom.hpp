#pragma once
#include <string>
#include <vector>
#include <cstdint>

enum class Denom : int { PENNY=1, NICKEL=5, DIME=10, QUARTER=25 };

inline int cents(Denom d){ return static_cast<int>(d); }

struct DenomSpec {
  Denom  denom{Denom::QUARTER};
  double mass_g{5.670};   // US quarter default
  int    pulses_per_coin{1};
  int    low_stock_threshold{20}; // trigger fallback when <= threshold
  std::string name{"quarter"};
};

inline const DenomSpec& us_quarter(){
  static const DenomSpec s{Denom::QUARTER, 5.670, 1, 20, "quarter"};
  return s;
}
inline const DenomSpec& us_dime(){
  static const DenomSpec s{Denom::DIME, 2.268, 1, 20, "dime"};
  return s;
}
inline const DenomSpec& us_nickel(){
  static const DenomSpec s{Denom::NICKEL, 5.000, 1, 20, "nickel"};
  return s;
}
inline const DenomSpec& us_penny(){
  static const DenomSpec s{Denom::PENNY, 2.500, 1, 20, "penny"};
  return s;
}

inline std::vector<DenomSpec> default_us_specs(bool include_pennies=true){
  std::vector<DenomSpec> v{us_quarter(), us_dime(), us_nickel()};
  if(include_pennies) v.push_back(us_penny());
  return v;
}

