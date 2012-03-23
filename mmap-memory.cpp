#include <cassert>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "mmap-memory.h"

memory::memory(const char* fn, size_t sz) : fd(-1), map(MAP_FAILED) {
  const int access = O_RDWR | O_CREAT;
  this->fd = ::open(fn, access, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  if(this->fd == -1) { return; }
#if _POSIX_C_SOURCE >= 200112L
  {
    int err;
    if((err = posix_fallocate(this->fd, 0, sz)) != 0) {
      std::cerr << "fallocate failed, err=" << err << "\n";
      this->close();
      return;
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
