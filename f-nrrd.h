#ifndef TJF_FILTER_NRRD_H
#define TJF_FILTER_NRRD_H

#include <array>
#include <memory>
#include <string>

struct nrrd_impl;

class nrrd {
  public:
    enum dtype {
      UINT8, INT8, UINT16, INT16, UINT32, INT32, UINT64, INT64, FLOAT, DOUBLE
    };

  public:
    nrrd(const char* fn);
    virtual ~nrrd();

    // .. nrrd can technically be ND, but we don't care/force 3D here
    virtual std::array<uint64_t, 3> dimensions() const;
    virtual size_t n_dimensions(); // should always return 3, as per above

    virtual dtype datatype() const;

    // we don't provide data access from this class.  instead, we assume all
    // nrrds are 'detached' and just give the user the filename.
    virtual std::string filename() const;

  private:
    std::unique_ptr<nrrd_impl> m;
};

#endif /* TJF_FILTER_NRRD_H */
