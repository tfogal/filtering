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
#include <libgen.h>

#include "f-nrrd.h"
#include "sutil.h"

struct array_deleter {
  template<typename T> void operator()(T* t) const { delete[] t; }
};

/// mmap-backed memory
struct memory {
  memory(const char* fn, size_t sz);
  ~memory();

  explicit operator bool() const {
    return this->fd != -1 && this->map != MAP_FAILED;
  }

  void close();

  int fd;
  void* map;
  size_t length;
};

memory::memory(const char* fn, size_t sz) : fd(-1), map(MAP_FAILED) {
  const int access = O_RDWR | O_CREAT;
  this->fd = ::open(fn, access, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  if(this->fd == -1) { return; }
#if _POSIX_C_SOURCE >= 200112L
  {
    int err;
    if((err = posix_fallocate(this->fd, 0, sz)) != 0) {
      std::cerr << "fallocate failed, errno=" << err << "\n";
      ::close(this->fd);
    }
  }
#endif

  const int mm_protection = PROT_WRITE;
  const int mm_flags = MAP_SHARED;

  /* length must be a multiple of the page size.  Round it up. */
  const long page_size = sysconf(_SC_PAGESIZE);
  const uint64_t u_page_size = static_cast<uint64_t>(page_size);
  this->length = sz;
  if(page_size != -1 && (this->length % u_page_size) != 0) {
    /* i.e. sysconf was successful and length isn't a multiple of page size. */
    this->length += (u_page_size * ((this->length-1) / u_page_size)) +
                    (u_page_size - this->length);
    assert((this->length % u_page_size) == 0);
  }
  assert(this->length > 0);

  this->map = ::mmap(NULL, this->length, mm_protection, mm_flags, this->fd, 0);
  if(MAP_FAILED == this->map) {
    this->close();
  }
}

memory::~memory() { this->close(); }

void memory::close() {
  if(this->map != MAP_FAILED) {
    int mu = munmap(this->map, this->length);
    if(mu != 0) {
      throw std::invalid_argument("could not munmap file");
    }
  }
  this->map = MAP_FAILED;

  if(this->fd != -1) {
    int cl;
    do {
      cl = ::close(this->fd);
    } while(cl == -1 && errno == EINTR);
    if(cl == -1 && errno == EBADF) {
      throw std::invalid_argument("invalid file descriptor");
    } else if(cl == -1 && errno == EIO) {
      throw std::runtime_error("I/O error.");
    }
  }
  this->fd = -1;
}

struct stream_deleter {
  void operator()(std::ifstream* f) const {
    std::clog << "Closing stream " << f << "\n";
    f->close();
  }
};

class config {
  public:
    /// @param file is the configuration file
    /// @param delim is the delimiter which separates keys from values
    config(std::string file, const char delim = ':');
    virtual ~config();

    virtual std::string value(std::string key);

  private:
    std::unique_ptr<std::ifstream, stream_deleter> cfg;
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

int main(int argc, char* argv[])
{
  if(argc != 2) {
    std::cerr << "Usage: " << argv[0] << "configfile\n";
    return EXIT_FAILURE;
  }

  config cfg(argv[1]);

  nrrd innhdr(cfg.value("in").c_str());
  assert(innhdr.n_dimensions() == 3); // can't handle more, right now.
  std::array<uint64_t,3> dims = innhdr.dimensions();

  const uint64_t bytes = std::accumulate(dims.begin(), dims.end(),
                                         sizeof(uint8_t),
                                         std::multiplies<uint64_t>());

  return EXIT_SUCCESS;
}
