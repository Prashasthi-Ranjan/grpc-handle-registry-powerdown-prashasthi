#include "handle_registry.h"
#include "module_sim.h"

#include <iostream>
#include <vector>

std::string HandleRegistry::create_handle(MockModule* ptr, const std::string& rpc) {
  uint64_t id = counter_.fetch_add(1);
  std::string handle = "mod-" + std::to_string(id);
  HandleState st;
  st.ptr = ptr;
  st.last_access = std::chrono::steady_clock::now();
  st.last_rpc = rpc;
  st.powering_down = false;
  {
    std::lock_guard<std::mutex> lock(mu_);
    map_[handle] = std::move(st);
  }
  return handle;
}

MockModule* HandleRegistry::lookup(const std::string& handle, const std::string& rpc) {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = map_.find(handle);
  if (it == map_.end()) return nullptr;
  if (it->second.powering_down) {
    // Per spec, handle in powerdown-in-progress should not be accessible (OpticsRemoval in progress)
    return nullptr;
  }
  it->second.last_access = std::chrono::steady_clock::now();
  if (!rpc.empty()) it->second.last_rpc = rpc;
  return it->second.ptr;
}

bool HandleRegistry::erase(const std::string& handle) {
  // Use erase_with_powerdown with default 500ms timeout for spec MaxDurationModulePwrDn
  auto res = erase_with_powerdown(handle, std::chrono::milliseconds(500));
  return res.erased;
}

size_t HandleRegistry::sweep_idle(std::chrono::seconds ttl) {
  // Phase 1: collect idle handles while holding lock
  struct ToSweep {
    std::string handle;
    MockModule* ptr;
    std::string last_rpc;
    std::chrono::seconds age;
    ModuleState state_before;
  };
  std::vector<ToSweep> to_sweep;
  auto now = std::chrono::steady_clock::now();

  {
    std::lock_guard<std::mutex> lock(mu_);
    for (auto& kv : map_) {
      if (kv.second.powering_down) continue; // already being powered down
      auto age = now - kv.second.last_access;
      if (age > ttl) {
        ToSweep item;
        item.handle = kv.first;
        item.ptr = kv.second.ptr;
        item.last_rpc = kv.second.last_rpc;
        item.age = std::chrono::duration_cast<std::chrono::seconds>(age);
        item.state_before = kv.second.ptr ? kv.second.ptr->get_state() : ModuleState::LOW_PWR;
        kv.second.powering_down = true; // mark to block concurrent lookup
        to_sweep.push_back(std::move(item));
      }
    }
  }

  // Phase 2: powerdown outside lock (avoid deadlock, per TODO_CONFORMA Q5 Q6)
  for (auto& item : to_sweep) {
    std::cerr << "[WARNING] GC sweeping idle handle " << item.handle
              << " age " << item.age.count()
              << "s last_rpc=" << item.last_rpc << std::endl;
    std::cout << "[INFO] Powerdown handle " << item.handle
              << " starting, state=" << to_string(item.state_before) << std::endl;

    bool ok = true;
    if (item.ptr) {
      ok = item.ptr->power_down(std::chrono::milliseconds(500));
      item.ptr->destroy();
      if (ok) {
        std::cout << "[INFO] Powerdown handle " << item.handle
                  << " complete, state=" << to_string(item.ptr->get_state()) << std::endl;
      } else {
        std::cerr << "[WARNING] Powerdown timeout handle " << item.handle
                  << " state=" << to_string(item.ptr->get_state()) << std::endl;
      }
    }
  }

  // Phase 3: erase from map
  {
    std::lock_guard<std::mutex> lock(mu_);
    for (auto& item : to_sweep) {
      map_.erase(item.handle);
    }
  }

  return to_sweep.size();
}

PowerdownResult HandleRegistry::erase_with_powerdown(const std::string& handle,
                                                     std::chrono::milliseconds timeout) {
  MockModule* ptr = nullptr;
  std::string last_rpc;
  ModuleState state_before = ModuleState::LOW_PWR;

  {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = map_.find(handle);
    if (it == map_.end()) {
      return {false, false};
    }
    if (it->second.powering_down) {
      // Already in powerdown, treat as not found for idempotency (second erase false)
      return {false, false};
    }
    ptr = it->second.ptr;
    last_rpc = it->second.last_rpc;
    state_before = ptr ? ptr->get_state() : ModuleState::LOW_PWR;
    it->second.powering_down = true; // block lookup during powerdown
  }

  // Powerdown outside lock
  std::cout << "[INFO] Powerdown handle " << handle
            << " starting, state=" << to_string(state_before)
            << " timeout=" << timeout.count() << "ms last_rpc=" << last_rpc << std::endl;

  bool ok = true;
  if (ptr) {
    ok = ptr->power_down(timeout);
    ptr->destroy();
    if (ok) {
      std::cout << "[INFO] Powerdown handle " << handle
                << " complete, state=" << to_string(ptr->get_state()) << std::endl;
    } else {
      std::cerr << "[WARNING] Powerdown timeout handle " << handle
                << " state=" << to_string(ptr->get_state())
                << " timeout=" << timeout.count() << "ms" << std::endl;
    }
  }

  // Erase after powerdown
  {
    std::lock_guard<std::mutex> lock(mu_);
    map_.erase(handle);
  }

  return {true, ok};
}

size_t HandleRegistry::shutdown() {
  struct ToSweep {
    std::string handle;
    MockModule* ptr;
    ModuleState state_before;
  };
  std::vector<ToSweep> to_sweep;

  {
    std::lock_guard<std::mutex> lock(mu_);
    if (map_.empty()) {
      return 0;
    }
    for (auto& kv : map_) {
      if (kv.second.powering_down) continue;
      ToSweep item;
      item.handle = kv.first;
      item.ptr = kv.second.ptr;
      item.state_before = kv.second.ptr ? kv.second.ptr->get_state() : ModuleState::LOW_PWR;
      kv.second.powering_down = true;
      to_sweep.push_back(std::move(item));
    }
  }

  std::cout << "[INFO] Shutdown powerdown " << to_sweep.size() << " handles" << std::endl;

  for (auto& item : to_sweep) {
    std::cout << "[INFO] Powerdown handle " << item.handle
              << " starting, state=" << to_string(item.state_before) << " (shutdown)" << std::endl;
    bool ok = true;
    if (item.ptr) {
      ok = item.ptr->power_down(std::chrono::milliseconds(500));
      item.ptr->destroy();
      if (ok) {
        std::cout << "[INFO] Powerdown handle " << item.handle
                  << " complete, state=" << to_string(item.ptr->get_state()) << std::endl;
      } else {
        std::cerr << "[WARNING] Powerdown timeout handle " << item.handle
                  << " during shutdown" << std::endl;
      }
    }
  }

  {
    std::lock_guard<std::mutex> lock(mu_);
    // Erase all collected (they were marked powering_down)
    for (auto& item : to_sweep) {
      map_.erase(item.handle);
    }
    // In case new handles were added during shutdown powerdown (race), they remain - that's ok per idempotent spec
    // But spec says shutdown should return count destroyed, which is to_sweep.size()
    // For strictness, clear any remaining that were not marked? No, leave them for next shutdown call
    // To meet size()==0 after shutdown when no concurrent creates, we already erased to_sweep
    // If there were no concurrent creates, map should now be empty because we erased all we collected
    // Handle any leftover that were marked powering_down already? We skipped those in collection, but they are mid-powerdown - keep them
  }

  return to_sweep.size();
}

size_t HandleRegistry::size() const {
  std::lock_guard<std::mutex> lock(mu_);
  // Exclude those currently powering down? For test expectations, size should reflect not-yet-erased handles
  // Original implementation counted all entries, including those marked powering_down until erased
  // So we count entries that are not yet erased (including powering_down)
  // But to avoid size counting handles that are logically gone, we could count all
  // Keep original behavior: return map size (includes powering_down until final erase)
  // For sweep_idle and shutdown we erase after powerdown, so size reflects final state
  // For lookup during powerdown returning nullptr, size still includes it until erase - that's intended to avoid double sweep
  // So return map size, but if we want size to be 0 after marking powering_down? No, after final erase it becomes 0
  // Count only non-powering_down? Let's count all for backwards compat, but tests expect after sweep size 0, after marking powering_down but before erase size still >0 temporarily
  // Our sweep and shutdown erase after powerdown, so size will be 0 after completion
  // For intermediate, size includes powering_down entries
  return map_.size();
}
