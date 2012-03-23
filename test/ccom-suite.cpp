#include <array>
#include <cstdio>
#include <fstream>
#include <cpptest.h>
#include "ccom-suite.h"
#include "ccom.h"

class CComSuite::nullbuf : public std::streambuf {
  public:
    nullbuf() { }
    int_type underflow() { return 0; }
    void flush() { }
};

CComSuite::CComSuite() {
  //TEST_ADD(CComSuite::test_empty);
  //TEST_ADD(CComSuite::test_single);
  TEST_ADD(CComSuite::test_twovalues);
}

void CComSuite::setup() {
  std::clog.rdbuf(this->nb.get());

  std::ofstream cfg(".config");
  cfg << "in: .nhdr\n"
      << "outraw: .outraw\n"
      << "outnhdr: .outnhdr\n"
      << "component: { range 0 20 }\n";
  cfg.close();

  std::ofstream nhdr(".nhdr");
  nhdr << "NRRD0002\n"
       << "dimension: 3\n"
       << "type: uint8\n"
       << "encoding: raw\n"
       << "data file: .rawfile\n";
  nhdr.close();
}
void CComSuite::tear_down() {
#if 1
  remove(".rawfile");
  remove(".config");
  remove(".nhdr");
  remove(".outnhdr");
  remove(".outraw");
#endif
}

void CComSuite::test_empty() {
  std::ofstream empty(".rawfile", std::ios::binary);
  empty.close();

  std::ofstream nhdr(".nhdr", std::ios::app);
  nhdr << "sizes: 0 0 0\n";
  nhdr.close();

  ccom(".config");

  std::ifstream outraw(".outraw", std::ios::binary);
  TEST_ASSERT(outraw);
  uint8_t v;
  outraw.read(reinterpret_cast<char*>(&v), sizeof(uint8_t));
  TEST_ASSERT(outraw.eof());
}

// just a single value
void CComSuite::test_single() {
  uint8_t value = 42;
  std::ofstream empty(".rawfile", std::ios::binary);
  for(size_t i=0; i < 6; ++i) {
    empty.write(reinterpret_cast<const char*>(&value), sizeof(uint8_t));
  }
  empty.close();

  std::ofstream nhdr(".nhdr", std::ios::app);
  nhdr << "sizes: 6 1 1\n";
  nhdr.close();

  ccom(".config");

  std::ifstream outraw(".outraw", std::ios::binary);
  // make sure we get all the same value
  uint8_t v;
  for(size_t i=0; i < 6; ++i) {
    outraw.read(reinterpret_cast<char*>(&v), sizeof(uint8_t));
    TEST_ASSERT(v == 0);
  }
  // .. and also that the output is not too large.
  outraw.read(reinterpret_cast<char*>(&v), sizeof(uint8_t));
  TEST_ASSERT(outraw.eof());
}

namespace {
  template<size_t N>
  bool match(const std::array<uint8_t,N> data, std::istream& strm) {
    uint8_t v;
    for(size_t i=0; i < N; ++i) {
      strm.read(reinterpret_cast<char*>(&v), sizeof(uint8_t));
      if(data[i] != v) { return false; }
    }
    return true;
  }
}

// two components
void CComSuite::test_twovalues() {
  uint8_t value = 42;
  std::ofstream empty(".rawfile", std::ios::binary);
  for(size_t i=0; i < 6; ++i) {
    if(i >= 2) { value = 19; }
    empty.write(reinterpret_cast<const char*>(&value), sizeof(uint8_t));
  }
  empty.close();

  std::ofstream nhdr(".nhdr", std::ios::app);
  nhdr << "sizes: 6 1 1\n";
  nhdr.close();

  ccom(".config");

  std::ifstream outraw(".outraw", std::ios::binary);
  // make sure we get all the same value
  for(size_t i=0; i < 6; ++i) {
    uint8_t v;
    outraw.read(reinterpret_cast<char*>(&v), sizeof(uint8_t));
    std::cout << (unsigned int)v << " ";
  }
  outraw.seekg(0, std::ios::beg);
  std::cout << "\n";
  TEST_ASSERT(match<6>({{0,0,0,1,1,1}}, outraw));
  // .. and also that the output is not too large.
  uint8_t v;
  outraw.read(reinterpret_cast<char*>(&v), sizeof(uint8_t));
  TEST_ASSERT(outraw.eof());
}
