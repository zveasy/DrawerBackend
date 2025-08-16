#pragma once
#include <string>
struct IShutter {
  virtual ~IShutter() = default;
  virtual bool home(int max_mm, std::string* reason=nullptr) = 0;
  virtual bool open_mm(int mm, std::string* reason=nullptr) = 0;
  virtual bool close_mm(int mm, std::string* reason=nullptr) = 0;
};
