#include "app/service_mode.hpp"
#include <map>
#include <chrono>

using namespace std::chrono;

ServiceMode::ServiceMode(const cfg::Service& cfg, safety::FaultManager& fm, eventlog::Logger& elog)
  : cfg_(cfg), fm_(fm), elog_(elog) {}

bool ServiceMode::enter(const std::string& pin, const std::string& user){
  if(!cfg_.enable) return false;
  if(pin!=cfg_.pin_code) return false;
  active_=true; user_=user;
  log({{"ts", std::to_string(duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count())},
       {"user", user_}, {"action", "enter"}, {"ok", "true"}});
  return true;
}

void ServiceMode::exit(){
  if(active_){
    log({{"ts", std::to_string(duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count())},
         {"user", user_}, {"action", "exit"}, {"ok", "true"}});
  }
  active_=false; user_.clear();
}

bool ServiceMode::guard(){
  if(!active_) return false;
  if(fm_.safety_blocking()) return false;
  return true;
}

void ServiceMode::log(const std::map<std::string,std::string>& kv){ elog_.append(kv); }

bool ServiceMode::cmd_open_shutter(IShutter& sh, int mm){
  if(!guard()) return false;
  bool ok=sh.open_mm(mm,nullptr);
  log({{"ts", std::to_string(duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count())},
       {"user", user_}, {"action", "open"}, {"mm", std::to_string(mm)}, {"ok", ok?"true":"false"}});
  return ok;
}

bool ServiceMode::cmd_close_shutter(IShutter& sh, int mm){
  if(!guard()) return false;
  bool ok=sh.close_mm(mm,nullptr);
  log({{"ts", std::to_string(duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count())},
       {"user", user_}, {"action", "close"}, {"mm", std::to_string(mm)}, {"ok", ok?"true":"false"}});
  return ok;
}

bool ServiceMode::cmd_dispense(IDispenser& d, int coins){
  if(!guard()) return false;
  auto st=d.dispenseCoins(coins);
  log({{"ts", std::to_string(duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count())},
       {"user", user_}, {"action", "dispense"}, {"coins", std::to_string(coins)}, {"ok", st.ok?"true":"false"}});
  return st.ok;
}

bool ServiceMode::run_jam_clear_shutter(Stepper& stp, const cfg::Service& s){
  if(!guard()) return false;
  bool ok=jam_clear::shutter(stp,s,fm_);
  log({{"ts", std::to_string(duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count())},
       {"user", user_}, {"action", "jam_clear_shutter"}, {"ok", ok?"true":"false"}});
  return ok;
}

bool ServiceMode::run_jam_clear_hopper(IDispenser& d, const cfg::Service& s){
  if(!guard()) return false;
  bool ok=jam_clear::hopper(d,s,fm_);
  log({{"ts", std::to_string(duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count())},
       {"user", user_}, {"action", "jam_clear_hopper"}, {"ok", ok?"true":"false"}});
  return ok;
}

