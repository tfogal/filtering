#include <memory>
#include <cppunit/TestCaller.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestSuite.h>
#include <cppunit/ui/text/TestRunner.h>
#include "ccom-suite.h"

int main(int, char *[]) {
  CppUnit::TextUi::TestRunner runner;
  CppUnit::TestSuite *suite = new CppUnit::TestSuite("blah");
  CppUnit::TestResult result;
#if 1
  suite->addTest(new CppUnit::TestCaller<CComSuite>("test_empty",
                 &CComSuite::test_empty));
  suite->addTest(new CppUnit::TestCaller<CComSuite>("test_single",
                 &CComSuite::test_single));
  suite->addTest(new CppUnit::TestCaller<CComSuite>("test_twovalues_merged",
                 &CComSuite::test_twovalues_merged));
#endif
  suite->addTest(new CppUnit::TestCaller<CComSuite>("test_twovalues_separate",
                 &CComSuite::test_twovalues_separate));
  suite->addTest(new CppUnit::TestCaller<CComSuite>("test_2d_separate",
                 &CComSuite::test_2d_separate));
  runner.addTest(suite);
  runner.run();
}
