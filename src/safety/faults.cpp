#include "safety/faults.hpp"
#include <chrono>
#include <filesystem>

using namespace std::chrono;

namespace safety {

static uint64_t now_ns() {
  return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

std::string fault_code_str(FaultCode c) {
  switch(c){
    case FaultCode::ESTOP: return "ESTOP";
    case FaultCode::LID_TAMPER: return "LID_TAMPER";
    case FaultCode::OVERCURRENT: return "OVERCURRENT";
    case FaultCode::JAM_SHUTTER: return "JAM_SHUTTER";
    case FaultCode::JAM_HOPPER: return "JAM_HOPPER";
    case FaultCode::LIMIT_TIMEOUT: return "LIMIT_TIMEOUT";
    case FaultCode::SENSOR_FAIL: return "SENSOR_FAIL";
    case FaultCode::CONFIG_ERROR: return "CONFIG_ERROR";
    default: return "NONE";
  }
}

FaultManager::FaultManager(const cfg::Safety& s, eventlog::Logger* elog)
  : cfg_(s), elog_(elog) {
  chip_ = hal::make_chip();
  estop_ = (s.estop_pin>=0)? chip_->request_line(s.estop_pin, hal::Direction::In) : nullptr;
  lid_ = (s.lid_tamper_pin>=0)? chip_->request_line(s.lid_tamper_pin, hal::Direction::In) : nullptr;
  overcurrent_ = (s.overcurrent_pin>=0)? chip_->request_line(s.overcurrent_pin, hal::Direction::In) : nullptr;
}

FaultManager::~FaultManager(){ stop(); }

bool FaultManager::read_line(hal::Line* ln) const {
  if(!ln) return false;
  bool v = ln->read();
  return cfg_.active_high ? v : !v;
}

void FaultManager::start(){
  running_=true;
  th_=std::thread(&FaultManager::poll_thread,this);
}

void FaultManager::stop(){
  running_=false;
  if(th_.joinable()) th_.join();
}

void FaultManager::poll_thread(){
  bool est_prev=false,lid_prev=false,oc_prev=false;
  while(running_){
    bool e=read_line(estop_), l=read_line(lid_), o=read_line(overcurrent_);
    if(e && !est_prev) raise(FaultCode::ESTOP, "estop");
    if(!e && est_prev) clear(FaultCode::ESTOP);
    if(l && !lid_prev) raise(FaultCode::LID_TAMPER, "lid");
    if(!l && lid_prev) clear(FaultCode::LID_TAMPER);
    if(o && !oc_prev) raise(FaultCode::OVERCURRENT, "overcurrent");
    if(!o && oc_prev) clear(FaultCode::OVERCURRENT);
    est_prev=e; lid_prev=l; oc_prev=o;
    std::this_thread::sleep_for(std::chrono::milliseconds(cfg_.debounce_ms));
  }
}

void FaultManager::raise(FaultCode code, const std::string& reason, bool latch){
  std::lock_guard<std::mutex> lk(mu_);
  Fault f; f.code=code; f.reason=reason; f.ts_ns=now_ns(); f.latched=latch;
  faults_[code]=f;
  if(elog_){ elog_->append({{"ts", std::to_string(f.ts_ns)}, {"code", fault_code_str(code)}, {"reason", reason}}); }
}

void FaultManager::clear(FaultCode code){
  std::lock_guard<std::mutex> lk(mu_);
  faults_.erase(code);
}

bool FaultManager::any_active() const{
  std::lock_guard<std::mutex> lk(mu_);
  return !faults_.empty();
}

std::vector<Fault> FaultManager::active() const{
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<Fault> v; v.reserve(faults_.size());
  for(auto& kv: faults_) v.push_back(kv.second);
  return v;
}

bool FaultManager::safety_blocking() const{
  std::lock_guard<std::mutex> lk(mu_);
  return faults_.count(FaultCode::ESTOP) || faults_.count(FaultCode::LID_TAMPER) || faults_.count(FaultCode::OVERCURRENT);
}

bool FaultManager::check_and_block_motion() const{
  return !any_active();
}

} // namespace safety

