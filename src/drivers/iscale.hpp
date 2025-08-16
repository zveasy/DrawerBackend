#pragma once
struct IScale {
  virtual ~IScale() = default;
  virtual long read_average(int samples = 8) = 0;
};
