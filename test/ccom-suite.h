#ifndef TJF_CCOM_SUITE_H
#define TJF_CCOM_SUITE_H
#include <memory>
#include <cpptest.h>

class CComSuite : public Test::Suite {
  public:
    CComSuite();

  protected:
    virtual void setup();
    virtual void tear_down();

  private:
    void test_empty();
    void test_single();
    void test_twovalues();
    class nullbuf;
    std::unique_ptr<nullbuf> nb;
};
#endif /* TJF_CCOM_SUITE_H */
