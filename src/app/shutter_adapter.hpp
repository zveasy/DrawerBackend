#pragma once
#include "ishutter.hpp"
#include "shutter_fsm.hpp"

class ShutterAdapter : public IShutter {
public:
  explicit ShutterAdapter(ShutterFSM& fsm);
  bool home(int max_mm, std::string* reason=nullptr) override;
  bool open_mm(int mm, std::string* reason=nullptr) override;
  bool close_mm(int mm, std::string* reason=nullptr) override;
private:
  ShutterFSM& fsm_;
  bool waitFor(ShutterState target, std::string* reason);
};
