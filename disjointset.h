#ifndef TJF_DISJOINT_SET_H
#define TJF_DISJOINT_SET_H

#include <memory>

struct dset_impl;

/** hacky/quick disjoint set. */
class DisjointSet {
  public:
    DisjointSet();
    ~DisjointSet();
    // unions the two elements 'a' and 'b'
    void unio(int a, int b);
    // of the set which 'a' is a part of, returns the minimum value.
    int find(int a);

    void print(); // debugging

  private:
    std::unique_ptr<dset_impl> impl;
};
#endif /* TJF_DISJOINT_SET_H */
