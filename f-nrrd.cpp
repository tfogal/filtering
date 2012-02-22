#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include "f-nrrd.h"

struct stream_deleter {
  void operator()(std::ifstream* f) const {
    std::clog << "Closing stream " << f << "\n";
    f->close();
  }
};

struct nrrd_impl {
  nrrd_impl(const char* fn);
  std::array<uint64_t, 3> dimensions();
  size_t n_dimensions();
  nrrd::dtype datatype();
  std::string filename();

  // case-insensitive match of the key; returns the rest of the line.
  std::string value(std::string key);

  std::string fn;
  std::unique_ptr<std::ifstream, stream_deleter> nhdr;
};

nrrd_impl::nrrd_impl(const char* f) : fn(f), nhdr(new std::ifstream(f)) {
  if(!nhdr) {
    throw std::invalid_argument("Cannot open nhdr.");
  }
}

std::array<uint64_t, 3> nrrd_impl::dimensions() {
  std::istringstream sizes(this->value("sizes"));
  std::array<uint64_t, 3> dims;

  sizes >> dims[0] >> dims[1] >> dims[2];
  return dims;
}

size_t nrrd_impl::n_dimensions() { /* HACK */ return 3; }

nrrd::dtype nrrd_impl::datatype() {
  const std::string typ = this->value("type");

  if(typ == "unsigned char" || typ == "uchar" || typ == "uint8" ||
     typ == "uint8_t") {
    return nrrd::dtype::UINT8;
  } else if(typ == "signed char" || typ == "int8" || typ == "int8_t") {
    return nrrd::dtype::INT8;
  } else if(typ == "ushort" || typ == "unsigned short" || typ == "uint16" ||
            typ == "unsigned short int" || typ == "uint16_t") {
    return nrrd::dtype::UINT16;
  } else if(typ == "short" || typ == "short int" || typ == "signed short" ||
            typ == "signed short int" || typ == "int16" || typ == "int16_t") {
    return nrrd::dtype::INT16;
  } else if(typ == "unsigned int" || typ == "uint" || typ == "uint32" ||
            typ == "uint32_t") {
    return nrrd::dtype::UINT32;
  } else if(typ == "int" || typ == "signed int" || typ == "int32" ||
            typ == "int32") {
    return nrrd::dtype::INT32;
  } else if(typ == "unsigned long long" || typ == "unsigned long long int" ||
            typ == "ulonglong" || typ == "uint64" || typ == "uint64_t") {
    return nrrd::dtype::UINT64;
  } else if(typ == "long long" || typ == "long long int" ||
            typ == "longlong" || typ == "int64" || typ == "int64_t") {
    return nrrd::dtype::INT64;
  } else if(typ == "float") {
    return nrrd::dtype::FLOAT;
  } else if(typ == "double") {
    return nrrd::dtype::DOUBLE;
  }

  std::clog << "unknown nrrd type '" << typ << "'!\n";
  throw std::domain_error("unknown nrrd type.");
}

std::string nrrd_impl::filename() { return this->fn; }

std::string trim(const std::string s) {
  std::string::const_iterator begin = s.begin();
  std::string::const_iterator end = s.end();

  for(begin=s.begin(); isspace(*begin); ++begin) { }
  for(end = s.end(); isspace(*end); --end)  { }

  return s.substr(
    std::distance(s.begin(), begin),
    s.length() - std::distance(end, s.end())
  );
}

// case-insensitive match of the key; returns the rest of the line.
std::string nrrd_impl::value(std::string key) {
  std::istream& hdr(*nhdr);

  hdr.seekg(0);
  std::string k;
  hdr >> k; // skip "NRRD000#" version number.
  do {
    char buffer[512];
    hdr.getline(buffer, 512, ':');
    k = trim(std::string(buffer));
    std::clog << "read key: '" << k << "'\n";
    hdr.getline(buffer, 512);
    if(k == key) {
      return trim(buffer);
    }
  } while(hdr && k != key);
  return std::string();
}

nrrd::nrrd(const char* fn) : m(new nrrd_impl(fn)) { }
nrrd::~nrrd() { }

std::array<uint64_t, 3> nrrd::dimensions() const { return m->dimensions(); }
size_t nrrd::n_dimensions() { return m->n_dimensions(); }

nrrd::dtype nrrd::datatype() const { return m->datatype(); }

std::string nrrd::filename() const { return m->filename(); }
