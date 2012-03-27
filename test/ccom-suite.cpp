#include <array>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <cppunit/TestAssert.h>
#include "ccom-suite.h"
#include "ccom.h"

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

  template<size_t N, typename ReadType, typename PrintType>
  void debugout(std::istream& strm) {
    ReadType r;
    std::cout << "debugstrm: ";
    for(size_t i=0; i < N; ++i) {
      strm.read(reinterpret_cast<char*>(&r), sizeof(ReadType));
      std::cout << static_cast<PrintType>(r) << " ";
    }
    std::cout << "\n";
    strm.seekg(0, std::ios::beg);
  }

  template<size_t N, typename T>
  void writearray(const char* filename, const std::array<T,N> data) {
    std::ofstream ofs(filename, std::ios::trunc | std::ios::binary);
    for(auto i=data.begin(); i != data.end(); ++i) {
      ofs.write(reinterpret_cast<const char*>(&*i), sizeof(T));
    }
    ofs.close();
  }

  void wrnhdr(size_t x, size_t y, size_t z) {
    std::ofstream nhdr(".nhdr", std::ios::app);
    nhdr << "sizes: " << x << " " << y << " " << z << "\n";
    nhdr.close();
  }

  bool at_eof(std::istream& is) {
    uint8_t v;
    is.read(reinterpret_cast<char*>(&v), sizeof(uint8_t));
    return is.eof();
  }
}

CComSuite::CComSuite() { }
CComSuite::~CComSuite() { }

void CComSuite::setUp() {
  std::ofstream cfg(".config");
  cfg << "in: .nhdr\n"
      << "outraw: .outraw\n"
      << "outnhdr: .outnhdr\n"
      << "component: { range 1 20 }\n";
  cfg.close();

  std::ofstream nhdr(".nhdr");
  nhdr << "NRRD0002\n"
       << "dimension: 3\n"
       << "type: uint8\n"
       << "encoding: raw\n"
       << "data file: .rawfile\n";
  nhdr.close();
}
void CComSuite::tearDown() {
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
  CPPUNIT_ASSERT(outraw);
  uint8_t v;
  outraw.read(reinterpret_cast<char*>(&v), sizeof(uint8_t));
  CPPUNIT_ASSERT(outraw.eof());
}

// just a single value
void CComSuite::test_single() {
  writearray<6,uint8_t>(".rawfile", {{19, 19, 19, 19, 19, 19}});

  std::ofstream nhdr(".nhdr", std::ios::app);
  nhdr << "sizes: 6 1 1\n";
  nhdr.close();

  ccom(".config");

  std::ifstream outraw(".outraw", std::ios::binary);
  // make sure we get all the same value
  uint8_t v;
  for(size_t i=0; i < 6; ++i) {
    outraw.read(reinterpret_cast<char*>(&v), sizeof(uint8_t));
    CPPUNIT_ASSERT(v == 1);
  }
  // .. and also that the output is not too large.
  outraw.read(reinterpret_cast<char*>(&v), sizeof(uint8_t));
  CPPUNIT_ASSERT(outraw.eof());
}

// two components but the range provided means they should be merged.
void CComSuite::test_twovalues_merged() {
  writearray<6,uint8_t>(".rawfile", {{6,6, 19,19,19,19}});

  std::ofstream nhdr(".nhdr", std::ios::app);
  nhdr << "sizes: 6 1 1\n";
  nhdr.close();

  ccom(".config");

  std::ifstream outraw(".outraw", std::ios::binary);
  // make sure we get all the same value
  CPPUNIT_ASSERT(match<6>({{1,1,1,1,1,1}}, outraw));
  // .. and also that the output is not too large.
  uint8_t v;
  outraw.read(reinterpret_cast<char*>(&v), sizeof(uint8_t));
  CPPUNIT_ASSERT(outraw.eof());
}

// same value with a zero between it -- should become two separate components.
void CComSuite::test_twovalues_separate() {
  writearray<7,uint8_t>(".rawfile", {{6,6,6,250,6,6,6}});
  wrnhdr(7, 1, 1);
  ccom(".config");

  std::ifstream outraw(".outraw", std::ios::binary);
  // make sure we get all the same value
  //debugout<7, uint8_t, uint32_t>(outraw);

  CPPUNIT_ASSERT(match<7>({{1,1,1,0,2,2,2}}, outraw));
  // .. and also that the output is not too large.
  CPPUNIT_ASSERT(at_eof(outraw));
}

// 2 2D blocks.
// a a 0 b b
// a a 0 b b
void CComSuite::test_2d_separate() {
  writearray<10,uint8_t>(".rawfile", {{8,8,0,5,5,8,8,0,5,5}});
  wrnhdr(5, 2, 1);
  ccom(".config");
  std::ifstream outraw(".outraw", std::ios::binary);
  CPPUNIT_ASSERT(match<10>({{1,1,0,2,2,1,1,0,2,2}}, outraw));
  CPPUNIT_ASSERT(at_eof(outraw));
}
