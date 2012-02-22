#include <cstdlib>
#include <fstream>
#include <iostream>

#include "f-nrrd.h"

int main(int argc, char* argv[])
{
  if(argc != 4) {
    std::cerr << "Usage: " << argv[0]
              << " in-nhdr out-raw out-nhdr\n";
    return EXIT_FAILURE;
  }

  nrrd n(argv[1]);

  std::array<uint64_t,3> dims = n.dimensions();

  return EXIT_SUCCESS;
}
