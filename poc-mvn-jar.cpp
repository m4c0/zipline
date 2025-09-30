#pragma leco tool
#define MCT_SYSCALL_IMPLEMENTATION
#include "../mct/mct-syscall.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

import jojo;
import jute;
import print;
import zipline;

using namespace jute::literals;

static const auto home = mct_syscall_dupenv("HOME");

class view_reader : public zipline::reader {
  jute::view data;
  unsigned pos {};

public:
  explicit constexpr view_reader(jute::view v) : data { v } {}

  constexpr void seek(unsigned ofs) override { pos = ofs; }
  constexpr void read(void * ptr, unsigned sz) override {
    for (auto i = 0; i < sz; i++) static_cast<char *>(ptr)[i] = data.data()[pos++];
  }
  constexpr uint32_t size() override { return data.size(); }

  void fail(jute::view err) override { die(err); }
};

static auto list_zip(jute::view name) {
  auto data = jojo::read_cstr(name);
  view_reader v { data };
  return zipline::list(&v);
}

static void list_classes(jute::view line) {
  if (line == "") return;

  if (*(line.end() - 1) == '\n') line = line.rsplit('\n').before;

  auto [lscp, scope] = line.trim().rsplit(':');
  if (lscp == "") return; // Maven junk
  if (scope != "compile") return;

  auto [grp_id, rgrp]    = lscp.split(':');
  auto [art_id, rart]    = rgrp.split(':');
  auto [type,   version] = rart.split(':');

  // Not entirely sure how deps in this criteria works
  if (version.index_of(':') >= 0) return;

  auto grp = grp_id.cstr();
  for (auto & c : grp) c = (c == '.') ? '/' : c;

  auto jar = jute::view::unsafe(home) + "/.m2/repository/" + grp + "/" + art_id + "/" + version + "/" + art_id + "-" + version + "." + type;
  for (auto & e : list_zip(jar.cstr())) {
    auto [name, ext] = (*e.name).rsplit('.');
    if (ext == "class") putln(name);
  }
}

int main(int argc, char **argv) try {
  // Takes the result of `mvn dependency:collect` and creates a list of all
  // classes in that classpath (based on some sensible defaults about maven)
  // Example:
  // mvn dependency:collect -DoutputFile=target/deps
  // poc-mvn-jar my-java-project/target/deps
  for (auto i = 1; i < argc; i++) {
    jojo::readlines(jute::view::unsafe(argv[i]), list_classes);
  }
  return 0;
} catch (...) {
  return 1;
}
