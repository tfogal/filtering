#ifndef TJF_MMAP_MEMORY_H
#define TJF_MMAP_MEMORY_H

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

#endif /* TJF_MMAP_MEMORY_H */
