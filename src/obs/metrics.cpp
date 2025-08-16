#include "obs/metrics.hpp"
#include <sstream>

namespace obs {

Histogram::Histogram(const std::vector<double>& buckets) : buckets_(buckets), counts_(buckets.size()+1) {}

void Histogram::observe(double v) {
  double s = sum_.load(); while(!sum_.compare_exchange_weak(s, s+v)){}
  count_.fetch_add(1);
  size_t i=0;
  for (; i<buckets_.size(); ++i) {
    if (v <= buckets_[i]) break;
  }
  for (size_t j=i; j<counts_.size(); ++j) counts_[j].fetch_add(1);
}

std::vector<long> Histogram::bucket_counts() const {
  std::vector<long> out;
  out.reserve(counts_.size());
  for (const auto& c : counts_) out.push_back(c.load());
  return out;
}

Counter& Metrics::counter(const std::string& name, const std::string& help, const Labels& labels) {
  std::lock_guard<std::mutex> lk(mu_);
  auto& fam = fams_[name];
  fam.type = Type::Counter; fam.help = help;
  auto it = fam.metrics.find(labels);
  if (it == fam.metrics.end()) {
    auto ptr = std::make_shared<Counter>();
    fam.metrics.emplace(labels, ptr);
    return *ptr;
  }
  return *static_cast<Counter*>(it->second.get());
}

Gauge& Metrics::gauge(const std::string& name, const std::string& help, const Labels& labels) {
  std::lock_guard<std::mutex> lk(mu_);
  auto& fam = fams_[name];
  fam.type = Type::Gauge; fam.help = help;
  auto it = fam.metrics.find(labels);
  if (it == fam.metrics.end()) {
    auto ptr = std::make_shared<Gauge>();
    fam.metrics.emplace(labels, ptr);
    return *ptr;
  }
  return *static_cast<Gauge*>(it->second.get());
}

Histogram& Metrics::histogram(const std::string& name, const std::string& help, const std::vector<double>& buckets, const Labels& labels) {
  std::lock_guard<std::mutex> lk(mu_);
  auto& fam = fams_[name];
  fam.type = Type::Histogram; fam.help = help;
  if (fam.buckets.empty()) fam.buckets = buckets;
  auto it = fam.metrics.find(labels);
  if (it == fam.metrics.end()) {
    auto ptr = std::make_shared<Histogram>(fam.buckets);
    fam.metrics.emplace(labels, ptr);
    return *ptr;
  }
  return *static_cast<Histogram*>(it->second.get());
}

static std::string fmt_labels(const Labels& labels) {
  if (labels.empty()) return "";
  std::ostringstream oss; oss << '{'; bool first=true;
  for (const auto& p:labels) {
    if(!first) oss << ','; first=false;
    oss << p.first << "=\"" << p.second << "\"";
  }
  oss << '}';
  return oss.str();
}

void Metrics::to_prometheus(std::ostream& os) const {
  std::lock_guard<std::mutex> lk(mu_);
  for (const auto& fpair : fams_) {
    const auto& name = fpair.first; const auto& fam = fpair.second;
    os << "# HELP " << name << ' ' << fam.help << "\n";
    std::string t;
    switch(fam.type){case Type::Counter:t="counter";break;case Type::Gauge:t="gauge";break;case Type::Histogram:t="histogram";break;}
    os << "# TYPE " << name << ' ' << t << "\n";
    for (const auto& mpair : fam.metrics) {
      const auto& labels = mpair.first;
      if (fam.type == Type::Counter) {
        auto* c = static_cast<Counter*>(mpair.second.get());
        os << name << fmt_labels(labels) << ' ' << c->value() << "\n";
      } else if (fam.type == Type::Gauge) {
        auto* g = static_cast<Gauge*>(mpair.second.get());
        os << name << fmt_labels(labels) << ' ' << g->value() << "\n";
      } else if (fam.type == Type::Histogram) {
        auto* h = static_cast<Histogram*>(mpair.second.get());
        auto counts = h->bucket_counts();
        for (size_t i=0;i<fam.buckets.size();++i) {
          Labels l = labels; l["le"] = std::to_string(fam.buckets[i]);
          os << name << "_bucket" << fmt_labels(l) << ' ' << counts[i] << "\n";
        }
        Labels linf = labels; linf["le"] = "+Inf";
        os << name << "_bucket" << fmt_labels(linf) << ' ' << counts.back() << "\n";
        os << name << "_sum" << fmt_labels(labels) << ' ' << h->sum() << "\n";
        os << name << "_count" << fmt_labels(labels) << ' ' << h->count() << "\n";
      }
    }
  }
}

void Metrics::to_json(std::ostream& os) const {
  std::lock_guard<std::mutex> lk(mu_);
  os << "{\"metrics\":[";
  bool first_metric=true;
  for (const auto& fpair : fams_) {
    const auto& name = fpair.first; const auto& fam = fpair.second;
    for (const auto& mpair : fam.metrics) {
      const auto& labels = mpair.first;
      if (fam.type == Type::Counter || fam.type == Type::Gauge) {
        double v = fam.type==Type::Counter ? static_cast<Counter*>(mpair.second.get())->value() : static_cast<Gauge*>(mpair.second.get())->value();
        if(!first_metric) os << ','; first_metric=false;
        os << "{\"name\":\""<<name<<"\",\"labels\":{";
        bool first=true; for(const auto& p:labels){ if(!first) os<<','; first=false; os<<"\""<<p.first<<"\":\""<<p.second<<"\""; }
        os << "},\"value\":"<<v<<"}";
      } else if (fam.type == Type::Histogram) {
        auto* h = static_cast<Histogram*>(mpair.second.get());
        auto counts = h->bucket_counts();
        for(size_t i=0;i<fam.buckets.size();++i){
          if(!first_metric) os<<','; first_metric=false;
          os << "{\"name\":\""<<name<<"_bucket\",\"labels\":{";
          bool first=true; for(const auto& p:labels){ if(!first) os<<','; first=false; os<<"\""<<p.first<<"\":\""<<p.second<<"\""; }
          if(!labels.empty()) os<<',';
          os<<"\"le\":\""<<fam.buckets[i]<<"\"},\"value\":"<<counts[i]<<"}";
        }
        if(!first_metric) os<<','; first_metric=false;
        os << "{\"name\":\""<<name<<"_bucket\",\"labels\":{";
        bool first=true; for(const auto& p:labels){ if(!first) os<<','; first=false; os<<"\""<<p.first<<"\":\""<<p.second<<"\""; }
        if(!labels.empty()) os<<',';
        os<<"\"le\":\"+Inf\"},\"value\":"<<counts.back()<<"}";
        if(!first_metric) os<<','; first_metric=false;
        os<<"{\"name\":\""<<name<<"_sum\",\"labels\":{";
        bool first2=true; for(const auto& p:labels){ if(!first2) os<<','; first2=false; os<<"\""<<p.first<<"\":\""<<p.second<<"\""; }
        os << "},\"value\":"<<h->sum()<<"}";
        if(!first_metric) os<<','; first_metric=false;
        os<<"{\"name\":\""<<name<<"_count\",\"labels\":{";
        bool first3=true; for(const auto& p:labels){ if(!first3) os<<','; first3=false; os<<"\""<<p.first<<"\":\""<<p.second<<"\""; }
        os << "},\"value\":"<<h->count()<<"}";
      }
    }
  }
  os << "]}";
}

Metrics& M() {
  static Metrics* m = new Metrics();
  return *m;
}

} // namespace obs

