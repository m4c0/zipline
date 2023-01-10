#include "build.hpp"

using namespace ecow;

int main(int argc, char **argv) {
  auto all = unit::create<seq>("all");

  auto gzt = all->add_unit<exe>("gzt");
  gzt->add_wsdep("yoyo", yoyo());
  gzt->add_ref(zipline());
  gzt->add_unit<>("gzt");

  return ecow::run_main(gzt, argc, argv);
}
