#include "module_sim.h"
#include <thread>
#include <chrono>

std::atomic<int> MockModule::global_id_{1};

MockModule::MockModule(std::string t) : type(std::move(t)), id(global_id_.fetch_add(1)) {
  state_ = ModuleState::LOW_PWR;
}

MockModule::~MockModule() {
  // Do not auto-destroy flag here, destroy() must be called explicitly to track leaks
}

ModuleState MockModule::get_state() const {
  return state_.load();
}

bool MockModule::power_up() {
  std::lock_guard<std::mutex> lk(mu_);
  if (state_ != ModuleState::LOW_PWR) {
    return false;
  }
  state_ = ModuleState::PWR_UP;
  // Simulate timed transition ModulePwrUp -> ModuleReady per spec timeout_ms 5000 (here 5ms for test speed)
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  state_ = ModuleState::READY;
  return true;
}

bool MockModule::power_down(std::chrono::milliseconds timeout) {
  const auto kRequired = std::chrono::milliseconds(10); // spec 5000ms, test ускорен to 10ms for 1000-handle perf
  {
    std::lock_guard<std::mutex> lk(mu_);
    if (state_ != ModuleState::READY) {
      // Already not ready, treat as success (already low power or fault)
      return true;
    }
    state_ = ModuleState::PWR_DN;
  }
  // Simulate hardware powerdown duration MaxDurationModulePwrDn = 10ms for tests (spec 5000ms)
  // If timeout < required, we timeout and leave in PWR_DN
  if (timeout < kRequired) {
    std::this_thread::sleep_for(timeout);
    // Timeout path: still in PWR_DN, return false
    return false;
  }
  std::this_thread::sleep_for(kRequired);
  {
    std::lock_guard<std::mutex> lk(mu_);
    if (state_ == ModuleState::PWR_DN) {
      state_ = ModuleState::LOW_PWR;
    }
  }
  return true;
}

void MockModule::set_fault() {
  state_ = ModuleState::FAULT;
}

bool MockModule::is_destroyed() const {
  return destroyed_.load();
}

void MockModule::destroy() {
  destroyed_ = true;
}

bool MockModule::is_powerdown_complete() const {
  auto s = state_.load();
  return s == ModuleState::LOW_PWR;
}
