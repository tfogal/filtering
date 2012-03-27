#ifndef TJF_CCOM_SUITE_H
#define TJF_CCOM_SUITE_H
#include <memory>
#include <cppunit/TestFixture.h>

class CComSuite : public CppUnit::TestFixture {
  public:
    CComSuite();
    virtual ~CComSuite();

    virtual void setUp();
    virtual void tearDown();

    void test_empty();
    void test_single();
    void test_twovalues_merged();
    void test_twovalues_separate();
    void test_2d_separate();
};
#endif /* TJF_CCOM_SUITE_H */
