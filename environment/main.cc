#include "handle_registry.h"
#include "module_sim.h"
#include <iostream>
#include <thread>
#include <vector>
#include <set>

int main() {
  HandleRegistry r;
  std::cout << "=== Manual smoke test for handle_registry powerdown ===" << std::endl;

  // Test create
  auto m1 = new MockModule("fr4");
  m1->power_up();
  std::cout << "m1 state after power_up: " << to_string(m1->get_state()) << std::endl;
  auto h1 = r.create_handle(m1, "OpticsInsertion");
  std::cout << "Created " << h1 << " size=" << r.size() << std::endl;

  // Test lookup
  auto p = r.lookup(h1, "ModuleReadInfo");
  std::cout << "Lookup " << h1 << " -> " << (p ? p->type : "null") << std::endl;

  // Test erase with powerdown
  std::cout << "Erasing " << h1 << std::endl;
  bool ok = r.erase(h1);
  std::cout << "Erase ok=" << ok << " size=" << r.size() << std::endl;
  std::cout << "m1 destroyed? " << m1->is_destroyed() << " state=" << to_string(m1->get_state()) << std::endl;
  delete m1;

  // Test sweep_idle
  std::cout << "\n--- sweep_idle test ---" << std::endl;
  std::vector<MockModule*> mods;
  std::vector<std::string> handles;
  for (int i=0;i<5;i++) {
    auto m = new MockModule("fr4");
    m->power_up();
    mods.push_back(m);
    handles.push_back(r.create_handle(m, "test"));
  }
  std::cout << "Created 5, size=" << r.size() << std::endl;
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  size_t swept = r.sweep_idle(std::chrono::seconds(0));
  std::cout << "Swept " << swept << " size after=" << r.size() << std::endl;
  for (auto m: mods) {
    std::cout << "mod destroyed=" << m->is_destroyed() << " state=" << to_string(m->get_state()) << std::endl;
    delete m;
  }

  // Test shutdown
  std::cout << "\n--- shutdown test ---" << std::endl;
  for (int i=0;i<3;i++) {
    auto m = new MockModule("fr4");
    m->power_up();
    r.create_handle(m);
  }
  std::cout << "Before shutdown size=" << r.size() << std::endl;
  size_t shut = r.shutdown();
  std::cout << "Shutdown returned " << shut << " size=" << r.size() << std::endl;

  std::cout << "\nManual test done. Now run proper pytest." << std::endl;
  return 0;
}
