#ifndef TJF_CONFIG_H
#define TJF_CONFIG_H

#include <fstream>
#include <memory>
#include "nonstd.h"

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
#endif /* TJF_CONFIG_H */
