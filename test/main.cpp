#include <memory>
#include <cpptest.h>
#include "ccom-suite.h"

int main(int, char *[]) {
  Test::Suite ts;
  ts.add(std::auto_ptr<Test::Suite>(new CComSuite));

  Test::TextOutput o(Test::TextOutput::Verbose);
  return ts.run(o);
}
