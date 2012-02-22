#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <libgen.h>

#include "f-nrrd.h"

std::string dirname_fixed(std::string);

struct array_deleter {
  template<typename T> void operator()(T* t) const { delete[] t; }
};

template<typename T> void threshold(std::istream& is, std::ostream& os,
                                    std::string bounds)
{
  std::pair<T,T> bds;
  {
    std::istringstream b(bounds);
    b >> bds.first >> bds.second;
  }

  T elem;
  const T zero = static_cast<T>(0);
  do {
    is.read(reinterpret_cast<char*>(&elem), sizeof(T));
    if(is.eof()) { break; }

    if(!is) {
      std::clog << "errno(" << errno << "): " << strerror(errno) << "\n";
      throw std::runtime_error("read failed.");
    }

    if(bds.first <= elem && elem <= bds.second) {
      os.write(reinterpret_cast<const char*>(&elem), sizeof(T));
    } else {
      os.write(reinterpret_cast<const char*>(&zero), sizeof(T));
    }
  } while(is);
}

int main(int argc, char* argv[])
{
  if(argc != 6) {
    std::cerr << "Usage: " << argv[0]
              << " in-nhdr out-raw out-nhdr lower-bound upper-bound\n";
    return EXIT_FAILURE;
  }

  nrrd n(argv[1]);

  std::array<uint64_t,3> dims = n.dimensions();
  std::string rawfn = n.filename();
  std::clog << dims[0] << "x" << dims[1] << "x" << dims[2] << " nrrd in file "
            << rawfn << "\n";
  std::ifstream raw(rawfn.c_str(), std::ios::in | std::ios::binary);
  if(!raw) {
    rawfn = dirname_fixed(argv[1]) + "/" + n.filename();
    raw.open(rawfn.c_str(), std::ios::in | std::ios::binary);
  }
  if(!raw) {
    std::cerr << "Cannot open " << rawfn << "\n";
    return EXIT_FAILURE;
  }

  { // write out the header for the file we created
    std::ofstream outhdr(argv[3], std::ios::out);
    if(!outhdr) {
      std::clog << "Could not open '" << argv[3] << "' to create output hdr.\n";
      return EXIT_FAILURE;
    }
    outhdr << "NRRD0002\n"
           << "dimension: 3\n"
           << "sizes: " << dims[0] << " " << dims[1] << " " << dims[2] << "\n"
           << "type: " << nrrd::type(n.datatype()) << "\n"
           << "encoding: raw\n"
           << "data file: " << argv[2] << "\n";
    outhdr.close();
  }

  std::ofstream out(argv[2], std::ios::out | std::ios::binary);
  if(!out) {
    std::cerr << "Could not open '" << out << "'\n";
    remove(argv[3]); // try to delete the nhdr we created.
    return EXIT_FAILURE;
  }

  std::string bounds(argv[4]);
  bounds += std::string(" ") + argv[5];
  switch(n.datatype()) {
    case nrrd:: UINT8: threshold< uint8_t>(raw, out, bounds); break;
    case nrrd::UINT16: threshold<uint16_t>(raw, out, bounds); break;
    case nrrd::UINT32: threshold<uint32_t>(raw, out, bounds); break;
    case nrrd::UINT64: threshold<uint64_t>(raw, out, bounds); break;
    case nrrd:: INT8: threshold< int8_t>(raw, out, bounds); break;
    case nrrd::INT16: threshold<int16_t>(raw, out, bounds); break;
    case nrrd::INT32: threshold<int32_t>(raw, out, bounds); break;
    case nrrd::INT64: threshold<int64_t>(raw, out, bounds); break;
    case nrrd::FLOAT: threshold<float>(raw, out, bounds); break;
    case nrrd::DOUBLE: threshold<double>(raw, out, bounds); break;
  }

  return EXIT_SUCCESS;
}

// the default 'dirname' call modifies it's argument.  ridiculous.
// this is the same thing with useful semantics.
std::string dirname_fixed(std::string s)
{
  std::shared_ptr<char> dir(new char[s.length()+1], array_deleter());
  s.copy(dir.get(), s.length());
  dir.get()[s.length()] = '\0';

  return std::string(dirname(dir.get()));
}
