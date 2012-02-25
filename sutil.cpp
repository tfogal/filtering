#include "sutil.h"

std::string trim(const std::string s) {
  std::string::const_iterator begin = s.begin();
  std::string::const_iterator end = s.end();

  for(begin=s.begin(); isspace(*begin); ++begin) { }
  for(end = s.end(); isspace(*end); --end)  { }

  return s.substr(
    std::distance(s.begin(), begin),
    s.length() - std::distance(end, s.end())
  );
}
