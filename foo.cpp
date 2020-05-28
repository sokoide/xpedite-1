#include <iostream>
#include <stdexcept>
#include <xpedite/framework/Framework.H>
#include <xpedite/framework/Probes.H>

void eat()   { std::cout << "eat..."   << std::endl; }
void sleep() { std::cout << "sleep..." << std::endl; }
void code()  { std::cout << "code..."  << std::endl; }

void life(int timeToLive_) {
  for(unsigned i=0; i<timeToLive_; ++i) {
    XPEDITE_TXN_SCOPE(Life);
    eat();

    XPEDITE_PROBE(SleepBegin);
    sleep();

    XPEDITE_PROBE(CodeBegin);
    code();
  }
}


int main() {
    if(!xpedite::framework::initialize("/tmp/xpedite-appinfo.txt", {xpedite::framework::AWAIT_PROFILE_BEGIN})) {
        throw std::runtime_error {"failed to init xpedite"};
    }
    life(100);
    return 0;
}

