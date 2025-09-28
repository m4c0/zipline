#pragma leco tool
#define MCT_SYSCALL_IMPLEMENTATION
#include "../mct/mct-syscall.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

import jute;
import silog;
import yoyo;
import zipline;

using namespace jute::literals;

static const auto home = mct_syscall_dupenv("HOME");

static auto print(const zipline::cdir_entry &cdir) {
  auto name = jute::view{reinterpret_cast<const char *>(cdir.filename.begin()),
                         cdir.filename.size()};
  if (name.subview(name.size() - 6).after != ".class")
    return mno::req{};

  printf("%.*s\n", cdir.filename.size() - 6, cdir.filename.begin());
  return mno::req{};
}

static auto list_zip(const char *name) {
  return yoyo::file_reader::open(name)
      .fmap([](auto &r) { return zipline::list(r, print); })
      .trace(("processing "_s + jute::view::unsafe(name)).cstr());
}

static mno::req<void> list_classes(char *line) {
  if (!*line)
    return {};

  line[strlen(line) - 1] = 0;

  auto tp = strrchr(line, ':');
  if (!tp) // Maven junk
    return {};

  if (0 != strcmp(tp + 1, "compile"))
    return {};

  *tp = 0;

  while (*line == ' ')
    line++;

  auto *grp_id = line;

  auto *art_id = strchr(line, ':');
  *art_id++ = 0;

  auto *type = strchr(art_id, ':');
  *type++ = 0;

  auto *version = strchr(type, ':');
  *version++ = 0;

  // Not entirely sure how deps in this criteria works
  if (strchr(version, ':'))
    return {};

  while ((line = strchr(line, '.'))) {
    *line = '/';
  }

  char jar[10240];
  snprintf(jar, sizeof(jar),
           "%5$s/.m2/repository/%1$s/%2$s/%4$s/%2$s-%4$s.%3$s", grp_id, art_id,
           type, version, home);
  return list_zip(jar);
}

int main(int argc, char **argv) try {
  // Takes the result of `mvn dependency:collect` and creates a list of all
  // classes in that classpath (based on some sensible defaults about maven)
  for (auto i = 1; i < argc; i++) {
    yoyo::file_reader::open(argv[i])
        .fpeek(yoyo::until_eof([](auto &r) {
          char buf[10240];
          return r.readline(buf, sizeof(buf)).fmap([&] {
            return list_classes(buf);
          });
        }))
        .map([](auto &) {})
        .log_error([] { throw 1; });
  }

  return 0;
} catch (...) {
  return 1;
}
