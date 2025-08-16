#include "util/metrics.hpp"

namespace metrics {

void Counter::inc(long delta) { v_.fetch_add(delta); }
long Counter::value() const { return v_.load(); }

Ewma::Ewma(double alpha) : alpha_(alpha) {}
void Ewma::update(double x) {
  double prev = y_.load();
  double next;
  do {
    next = alpha_*x + (1.0 - alpha_)*prev;
  } while(!y_.compare_exchange_weak(prev, next));
}
double Ewma::value() const { return y_.load(); }

} // namespace metrics
