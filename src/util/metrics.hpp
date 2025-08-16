#pragma once
#include <atomic>

namespace metrics {

class Counter {
public:
  void inc(long delta=1);
  long value() const;
private:
  std::atomic<long> v_{0};
};

class Ewma {
public:
  explicit Ewma(double alpha=0.2);
  void update(double x);
  double value() const;
private:
  std::atomic<double> y_{0.0};
  double alpha_;
};

} // namespace metrics
