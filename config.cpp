#include <stdexcept>
#include "config.h"
#include "sutil.h"

config::~config() { }

config::config(std::string file, const char delim) :
  cfg(new std::ifstream(file.c_str())), delimiter(delim) {
  if(!*cfg) {
    throw std::invalid_argument("Cannot open config file");
  }
}

std::string config::value(std::string key) {
  std::istream& hdr(*this->cfg);

  hdr.seekg(0);
  std::string k;
  do {
    char buffer[512];
    hdr.getline(buffer, 512, this->delimiter);
    k = trim(std::string(buffer));
    hdr.getline(buffer, 512);
    if(k == key) {
      return trim(buffer);
    }
  } while(hdr && k != key);

  throw std::runtime_error("key not found");
}

