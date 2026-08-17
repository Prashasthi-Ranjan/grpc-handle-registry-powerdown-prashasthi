#include "handle_registry.h"
#include "module_sim.h"
#include <iostream>
#include <chrono>

int main(){
  HandleRegistry r;
  // Test powerdown success
  {
    auto m = new MockModule("fr4");
    m->power_up();
    auto h = r.create_handle(m, "OpticsInsertion");
    auto res = r.erase_with_powerdown(h, std::chrono::milliseconds(500));
    std::cout << "erase_with_powerdown 500ms: erased=" << res.erased << " ok=" << res.powerdown_ok << " state=" << to_string(m->get_state()) << " destroyed=" << m->is_destroyed() << std::endl;
    if (!res.erased || !res.powerdown_ok) { std::cerr << "FAIL should succeed" << std::endl; return 1; }
    if (m->get_state()!=ModuleState::LOW_PWR) { std::cerr << "FAIL state not LOW_PWR" << std::endl; return 1; }
    delete m;
  }
  // Test powerdown timeout - required is 10ms, so 2ms should timeout
  {
    auto m = new MockModule("fr4");
    m->power_up();
    auto h = r.create_handle(m, "OpticsInsertion");
    auto res = r.erase_with_powerdown(h, std::chrono::milliseconds(2));
    std::cout << "erase_with_powerdown 2ms: erased=" << res.erased << " ok=" << res.powerdown_ok << " state=" << to_string(m->get_state()) << std::endl;
    if (!res.erased || res.powerdown_ok) { std::cerr << "FAIL should timeout but still erase" << std::endl; return 1; }
    // Even on timeout, should be destroyed and erased
    if (!m->is_destroyed()) { std::cerr << "FAIL destroyed flag not set on timeout" << std::endl; return 1; }
    if (r.size()!=0) { std::cerr << "FAIL size not 0 after timeout erase" << std::endl; return 1; }
    delete m;
  }
  // Test double erase
  {
    auto m = new MockModule("fr4");
    m->power_up();
    auto h = r.create_handle(m);
    bool first = r.erase(h);
    bool second = r.erase(h);
    std::cout << "double erase: first=" << first << " second=" << second << std::endl;
    if (!first || second) { std::cerr << "FAIL double erase" << std::endl; return 1; }
    delete m;
  }
  // Test lookup after erase
  {
    auto m = new MockModule("fr4");
    m->power_up();
    auto h = r.create_handle(m);
    r.erase(h);
    auto p = r.lookup(h);
    std::cout << "lookup after erase: " << (p?"found":"null") << std::endl;
    if (p!=nullptr) { std::cerr << "FAIL lookup after erase should be null" << std::endl; return 1; }
    delete m;
  }
  // Test shutdown idempotent
  {
    for(int i=0;i<5;i++){ auto m=new MockModule("fr4"); m->power_up(); r.create_handle(m); }
    size_t first = r.shutdown();
    size_t second = r.shutdown();
    std::cout << "shutdown idempotent: first=" << first << " second=" << second << std::endl;
    if (first!=5 || second!=0) { std::cerr << "FAIL shutdown idempotent" << std::endl; return 1; }
  }
  std::cout << "PASS powerdown" << std::endl;
  return 0;
}
