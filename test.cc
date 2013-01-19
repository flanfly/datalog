#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include "test.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(DESTest);	

int main( int argc, char **argv)
{
  CppUnit::TextUi::TestRunner runner;
  CppUnit::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry();

	runner.addTest( registry.makeTest() );
  runner.run();
  return 0;
}
