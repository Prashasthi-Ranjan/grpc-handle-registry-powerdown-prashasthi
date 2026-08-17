#include "handle_registry.h"
#include "module_sim.h"
#include <thread>
#include <vector>
#include <set>
#include <mutex>
#include <iostream>

int main(){
 HandleRegistry r;
 std::set<std::string> all;
 std::mutex mu;
 std::vector<std::thread> th;
 std::vector<MockModule*> mods;
 std::mutex mods_mu;

 for(int t=0;t<10;t++) th.emplace_back([&](){
   for(int i=0;i<100;i++){
     auto m = new MockModule("fr4");
     m->power_up();
     auto h=r.create_handle(m, "test");
     {
       std::lock_guard<std::mutex> lk(mu);
       all.insert(h);
     }
     {
       std::lock_guard<std::mutex> lk(mods_mu);
       mods.push_back(m);
     }
   }
 });
 for(auto &t:th) t.join();
 printf("unique=%zu size=%zu\n", all.size(), r.size());
 if (all.size()!=1000 || r.size()!=1000) {
   std::cerr << "FAIL concurrency unique" << std::endl;
   return 1;
 }
 // Cleanup via shutdown
 size_t swept = r.shutdown();
 printf("shutdown swept=%zu size after=%zu\n", swept, r.size());
 if (swept!=1000 || r.size()!=0) {
   std::cerr << "FAIL shutdown" << std::endl;
   return 1;
 }
 for (auto m: mods) delete m;
 std::cout << "PASS concurrency" << std::endl;
 return 0;
}
