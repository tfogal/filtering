#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/types.h>
#include <utility>

#include <libgen.h>

#include "ccom.h"
#include "f-nrrd.h"
#include "sutil.h"

struct array_deleter {
  template<typename T> void operator()(T* t) const { delete[] t; }
};

int main(int argc, char* argv[])
{
  if(argc != 2) {
    std::cerr << "Usage: " << argv[0] << "configfile\n";
    return EXIT_FAILURE;
  }

  ccom(argv[1]);

  return EXIT_SUCCESS;
}
