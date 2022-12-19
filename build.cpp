#include "../yoyo/build.hpp"
#include "ecow.hpp"

using namespace ecow;

int main(int argc, char **argv) {
  auto all = unit::create<seq>("zipline");
  all->add_wsdep("yoyo", yoyo());

  all->add_unit<mod>("containers");
  all->add_unit<mod>("bitstream");
  all->add_unit<mod>("huffman");

  auto d = all->add_unit<mod>("deflate");
  d->add_part("symbols");
  d->add_part("buffer");
  d->add_part("details");

  auto z = all->add_unit<mod>("zipline");
  z->add_part("common");
  z->add_part("eocd");
  z->add_part("cdir");
  z->add_part("iterator");
  z->add_part("local");

  return run_main(all, argc, argv);
}
