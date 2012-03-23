#include <algorithm>
#include <iterator>
#include <iostream>
#include <list>
#include <set>
#include <stdexcept>
#include "disjointset.h"

struct dset_impl {
  dset_impl() { }

  void unio(int a, int b) {
    // search for the set with 'a' in it.
    auto it = findset(a);
    // did we find such a set?  if not, let's search for 'b'.
    if(it == sets.end()) {
      it = findset(b);
    }
    // did we *still* not find anything?!
    if(it == sets.end()) {
      // then we need to add a completely new set.
      std::set<int> newset;
      newset.insert(a);
      newset.insert(b);
      this->sets.push_front(newset);
    } else {
      // or we need to add in our elements to the set we found.
      // we don't know which set this is, though -- i.e. did we
      // grab the set with 'a' in it or the set with 'b' in it?
      // fortunately sets are smart and duplicate entries are just
      // discarded, so we'll just throw in both and that'll cover
      // it.
      it->insert(a);
      it->insert(b);
    }
  }
  int find(int a) {
    // find which set this value is in.
    auto it = findset(a);
    // ... or maybe we found nothing at all?
    if(it == sets.end()) { throw std::runtime_error("set not found!"); }
    // what's the 'canonical value' for a set of values?  it is
    // arbitrary, and really doesn't matter as long as we are
    // consistent.  we choose the minimum element in the set for
    // now.
    auto m = std::min_element(it->begin(), it->end());
    return *m;
  }

  // identifies the set which has 'a' in it.  will be sets::end()
  // if such a set was not found.
  std::list<std::set<int>>::iterator findset(int a) {
    return std::find_if(sets.begin(), sets.end(),
                        [=](const std::set<int>& s) -> bool {
                          return s.find(a) != s.end();
                        });
  }

  // for debugging
  void print() {
    for(const std::set<int>& s : sets) {
      std::copy(s.begin(), s.end(),
                std::ostream_iterator<int>(std::clog, " "));
      std::clog << "\n";
    }
  }

  std::list<std::set<int>> sets;
};

DisjointSet::DisjointSet() : impl(new dset_impl()) { }
DisjointSet::~DisjointSet() = default;
// unions the two elements 'a' and 'b'
void DisjointSet::unio(int a, int b) { this->impl->unio(a,b); }
// of the set which 'a' is a part of, returns the minimum value.
int DisjointSet::find(int a) { return this->impl->find(a); }
void DisjointSet::print() { this->impl->print(); }
