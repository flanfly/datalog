#include <cppunit/extensions/HelperMacros.h>

#include "dlog.hh"
#include "dsl.hh"

class DESTest : public CppUnit::TestFixture  
{
	CPPUNIT_TEST_SUITE(DESTest);
	CPPUNIT_TEST(testFamily);
	CPPUNIT_TEST_SUITE_END();

public:
  void setUp(void) {}
  void tearDown(void) {}
  
  void testFamily(void)
	{
		rel_ptr father_rel(new relation());
		insert(father_rel,"tom","amy");
		insert(father_rel,"tony","carol_II");
		insert(father_rel,"fred","carol_III");
		insert(father_rel,"jack","fred");
		
		rel_ptr mother_rel(new relation());
		insert(mother_rel,"grace","amy");
		insert(mother_rel,"amy","fred");
		insert(mother_rel,"carol_I","carol_II");
		insert(mother_rel,"carol_II","carol_III");

		parse parent("parent"),ancestor("ancestor"),father("father"),mother("mother"),answer("answer");

		parent("X","Y") >> father("X","Y");
		parent("X","Y") >> mother("X","Y");

		ancestor("X","Y") >> parent("X","Y");
		ancestor("X","Z") >> parent("X","Y"),ancestor("Y","Z");

		answer("X","Y") >> ancestor("X","Y");

		std::list<rule> all;
		std::map<std::string,rel_ptr> db;

		std::copy(parent.rules.begin(),parent.rules.end(),std::inserter(all,all.end()));
		std::copy(ancestor.rules.begin(),ancestor.rules.end(),std::inserter(all,all.end()));
		std::copy(answer.rules.begin(),answer.rules.end(),std::inserter(all,all.end()));
		db.insert(std::make_pair("mother",mother_rel));
		db.insert(std::make_pair("father",father_rel));

		std::cout << eval(answer("X","Y"),all,db) << std::endl;
	}
};
