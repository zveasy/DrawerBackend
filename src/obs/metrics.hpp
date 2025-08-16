#pragma once
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <ostream>

namespace obs {

using Labels = std::map<std::string,std::string>;

class Counter {
  std::atomic<double> v_{0};
public:
  void inc(double d=1.0) { double cur=v_.load(); while(!v_.compare_exchange_weak(cur, cur+d)){} }
  double value() const { return v_.load(); }
};

class Gauge {
  std::atomic<double> v_{0};
public:
  void set(double d) { v_.store(d); }
  void inc(double d=1.0) { double cur=v_.load(); while(!v_.compare_exchange_weak(cur, cur+d)){} }
  double value() const { return v_.load(); }
};

class Histogram {
  std::vector<double> buckets_;
  std::vector<std::atomic<long>> counts_; // cumulative counts per bucket (+Inf last)
  std::atomic<double> sum_{0};
  std::atomic<long> count_{0};
public:
  explicit Histogram(const std::vector<double>& buckets);
  void observe(double v);
  double sum() const { return sum_.load(); }
  long count() const { return count_.load(); }
  std::vector<long> bucket_counts() const; // returns cumulative counts including +Inf
  const std::vector<double>& buckets() const { return buckets_; }
};

enum class Type { Counter, Gauge, Histogram };

struct MetricFamily {
  Type type{Type::Counter};
  std::string help;
  std::vector<double> buckets; // for histograms
  std::map<Labels, std::shared_ptr<void>> metrics;
};

class Metrics {
  mutable std::mutex mu_;
  std::map<std::string, MetricFamily> fams_;
public:
  Counter& counter(const std::string& name, const std::string& help="", const Labels& labels={});
  Gauge& gauge(const std::string& name, const std::string& help="", const Labels& labels={});
  Histogram& histogram(const std::string& name, const std::string& help="", const std::vector<double>& buckets={}, const Labels& labels={});
  void to_prometheus(std::ostream& os) const;
  void to_json(std::ostream& os) const;
};

Metrics& M();

} // namespace obs

