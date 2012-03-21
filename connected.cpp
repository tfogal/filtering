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
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utility>

#include <boost/pending/disjoint_sets.hpp>
#include <libgen.h>
#include <omp.h>

#include "f-nrrd.h"
#include "mmap-memory.h"
#include "nonstd.h"
#include "sutil.h"

struct array_deleter {
  template<typename T> void operator()(T* t) const { delete[] t; }
};

class config {
  public:
    /// @param file is the configuration file
    /// @param delim is the delimiter which separates keys from values
    config(std::string file, const char delim = ':');
    virtual ~config();

    virtual std::string value(std::string key);

  private:
    std::unique_ptr<std::ifstream, nonstd::stream_deleter> cfg;
    const char delimiter;
};

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

// identifies the set of equivalences which should be considered equal
// expects to parse something of the form:
//    { range a b }
// or:
//    { a b c }
// The former means the values 'a' through 'b' are the same.
// The latter means that a, b, and c should all be considered the same value
std::set<int64_t> equivalences(std::istream& is)
{
  std::string junk;
  std::string value;

  std::set<int64_t> equiv;
  is >> junk; // leading "{"
  is >> value;
  if(value == "range") { //  parse range values
    int64_t lower, upper;
    is >> lower >> upper;
    for(int64_t i=lower; i < upper; ++i) {
      equiv.insert(i);
    }
  } else { // parse out set
    int64_t v;
    // 'value' is an actual value.. convert it to an integer and add it.
    std::istringstream c(value);
    c >> v;
    equiv.insert(v);
    // now convert all the rest of the values
    while(is) {
      is >> v;
      if(is) {
        equiv.insert(v);
      }
    }
  }
  is >> junk; // trailing "}"
  return equiv;
}

int main(int argc, char* argv[])
{
  if(argc != 2) {
    std::cerr << "Usage: " << argv[0] << "configfile\n";
    return EXIT_FAILURE;
  }

  config cfg(argv[1]);

  nrrd innhdr(cfg.value("in").c_str());
  assert(innhdr.n_dimensions() == 3); // can't handle more, right now.
  const std::array<uint64_t,3> dims = innhdr.dimensions();

  // for now, we can only deal with 8bit data!
  // this is because it's a PITA otherwise: what type does this function
  // return?  it should return the type of the nrrd we are reading from.
  // but that means we need to templatize it.  templatized lambdas don't
  // exist.
  assert(innhdr.datatype() == nrrd::UINT8);

  const uint64_t bytes = std::accumulate(dims.begin(), dims.end(),
                                         sizeof(uint8_t),
                                         std::multiplies<uint64_t>());
  std::ifstream in(innhdr.filename().c_str(), std::ios::binary | std::ios::in);
  std::istringstream iss(cfg.value("component"));
  const std::set<int64_t> equivs = equivalences(iss);

  std::clog << "Creating '" << cfg.value("outraw") << "' output file.\n";
  memory out(cfg.value("outraw").c_str(), bytes);
  uint8_t* labels = static_cast<uint8_t*>(out.map);
  std::clog << "Zeroing out map...\n";
  std::fill(labels, labels+bytes, 0);

  auto valid_coordinate = [=](std::array<int64_t,3> d) {
    std::array<int64_t,3> ds;
    std::copy(dims.begin(), dims.end(), ds.begin());
    assert(std::accumulate(ds.begin(), ds.end(), 0, std::plus<int64_t>()) > 0);
    return 0 <= d[0] && d[0] < ds[0] &&
           0 <= d[1] && d[1] < ds[1] &&
           0 <= d[2] && d[2] < ds[2];
  };
  auto idx = [&](std::array<int64_t,3> d) {
    assert(valid_coordinate(d));
    return d[2]*(dims[0]*dims[1]) + d[1]*(dims[0]) + d[0];
  };
  auto value = [&](std::array<int64_t,3> d) {
    in.seekg(idx(d));
    uint8_t v;
    in.read(reinterpret_cast<char*>(&v), sizeof(uint8_t));
    /// @todo once we support multi-byte data, we probably want to do
    /// endianness conversion here.
    return v;
  };
  // true if the values at these two locations are the same, via the definition
  // of equality given in the config file.
  auto equal = [&](std::array<int64_t,3> a, std::array<int64_t,3> b) {
    return equivs.count(value(a)) > 0 && equivs.count(value(b)) > 0;
  };
  // true iff equal AND the coordinates are valid.
  // "c"equal -- "coordinate" equal
  auto cequal = [&](std::array<int64_t,3> a, std::array<int64_t,3> b) {
    assert(valid_coordinate(a) || valid_coordinate(b));
    if(!valid_coordinate(a)) { return false; }
    if(!valid_coordinate(b)) { return false; }
    return equal(a,b);
  };

  std::vector<int> rank(255);
  std::vector<int> parent(255);
  boost::disjoint_sets<int*,int*> ds(&rank[0], &parent[0]);

  uint8_t identifier = 0;
  std::clog << "Pass 1...\n";
  #pragma omp parallel for
  for(uint64_t z=0; z < dims[2]; ++z) {
    for(uint64_t y=0; y < dims[1]; ++y) {
      for(uint64_t x=0; x < dims[0]; ++x) {
#if 1
        if(idx({{x,y,z}}) % 1000 == 0 && omp_get_thread_num() == 0) {
          std::clog << "\r" << idx({{x,y,z}}) << " / "
                    << idx({{dims[0]-1, dims[1]-1, dims[2]-1}}) << " ("
                    << static_cast<double>(idx({{x,y,z}})) /
                       idx({{dims[0]-1, dims[1]-1, dims[2]-1}}) * 100.0f
                    << "%)...";
        }
#endif
        // is this voxel the one to merge all 3 neighbors?
        if(cequal({{x-1,y,z}}, {{x,y,z}}) && cequal({{x,y-1,z}}, {{x,y,z}}) &&
           cequal({{x,y,z-1}}, {{x,y,z}})) {
          // just copy left label; they'll all be unioned anyway, so it won't
          // matter, we'll clean it up in the second pass.
          labels[idx({{x,y,z}})] = labels[idx({{x-1,y,z}})];
          // (left equiv above) and (above equiv behind)
          ds.union_set(labels[idx({{x-1,y,z}})], labels[idx({{x,y-1,z}})]);
          ds.union_set(labels[idx({{x,y-1,z}})], labels[idx({{x,y,z-1}})]);
          continue;
        }

        // merge any two neighbors?  z-1&y-1, z-1&x-1, y-1&x-1
        // z-1&y-1
        if(cequal({{x,y,z-1}}, {{x,y,z}}) && cequal({{x,y-1,z}}, {{x,y,z}})) {
          labels[idx({{x,y,z}})] = labels[idx({{x,y,z-1}})];
          ds.union_set(labels[idx({{x,y,z-1}})], labels[idx({{x,y-1,z}})]);
          continue;
        }
        // z-1&x-1
        if(cequal({{x,y,z-1}}, {{x,y,z}}) && cequal({{x-1,y,z}}, {{x,y,z}})) {
          labels[idx({{x,y,z}})] = labels[idx({{x,y,z-1}})];
          ds.union_set(labels[idx({{x,y,z-1}})], labels[idx({{x-1,y,z}})]);
          continue;
        }
        // y-1&x-1
        if(cequal({{x,y-1,z}}, {{x,y,z}}) && cequal({{x-1,y,z}}, {{x,y,z}})) {
          labels[idx({{x,y,z}})] = labels[idx({{x,y-1,z}})];
          ds.union_set(labels[idx({{x,y-1,z}})], labels[idx({{x-1,y,z}})]);
          continue;
        }

        // merge any one neighbor?
        if(cequal({{x-1,y,z}}, {{x,y,z}})) { // x, left neighbor
          labels[idx({{x,y,z}})] = labels[idx({{x-1,y,z}})]; continue;
        }
        if(cequal({{x,y-1,z}}, {{x,y,z}})) { // y, neighbor underneath
          labels[idx({{x,y,z}})] = labels[idx({{x,y-1,z}})]; continue;
        }
        if(cequal({{x,y,z-1}}, {{x,y,z}})) { // z, neighbor behind
          labels[idx({{x,y,z}})] = labels[idx({{x,y,z-1}})]; continue;
        }

        // merges nobody, then!  assign a new label.
        #pragma omp critical
        {
          labels[idx({{x,y,z}})] = identifier++;
        }
      }
    }
  }
  std::clog << "\r" << "                                                     ";
  std::clog << "\r" << idx({{dims[0]-1,dims[1]-1,dims[2]-1}}) << " / "
            << idx({{dims[0]-1, dims[1]-1, dims[2]-1}}) << " ("
            << static_cast<double>(idx({{dims[0]-1,dims[1]-1,dims[2]-1}})) /
               idx({{dims[0]-1, dims[1]-1, dims[2]-1}}) * 100.0f
            << "%)...\n";
  std::clog << "Pass 2...\n";
  for(uint64_t z=0; z < dims[2]; ++z) {
    for(uint64_t y=0; y < dims[1]; ++y) {
      for(uint64_t x=0; x < dims[0]; ++x) {
        labels[idx({{x,y,z}})] = ds.find_set(labels[idx({{x,y,z}})]);
      }
    }
  }
  out.close();

  std::ofstream onhdr(cfg.value("outnhdr").c_str(), std::ios::out);
  onhdr << "NRRD0002\n"
        << "dimension: 3\n"
        << "sizes: " << dims[0] << " " << dims[1] << " " << dims[2] << "\n"
        << "type: " << "uint8" << "\n"
        << "encoding: raw\n"
        << "data file: " << cfg.value("outraw") << "\n";
  onhdr.close();

  return EXIT_SUCCESS;
}
