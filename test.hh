#include <cppunit/extensions/HelperMacros.h>

#include "dlog.hh"
#include "dsl.hh"

class DESTest : public CppUnit::TestFixture  
{
	CPPUNIT_TEST_SUITE(DESTest);
	CPPUNIT_TEST(testFamily);
	CPPUNIT_TEST(testMutualRec);
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

		std::map<std::string,rel_ptr> edb;
		std::multimap<std::string,rule_ptr> idb;

		std::for_each(parent.rules.begin(),parent.rules.end(),[&](rule_ptr r) { idb.insert(std::make_pair(r->head.name,r)); });
		std::for_each(ancestor.rules.begin(),ancestor.rules.end(),[&](rule_ptr r) { idb.insert(std::make_pair(r->head.name,r)); });
		std::for_each(answer.rules.begin(),answer.rules.end(),[&](rule_ptr r) { idb.insert(std::make_pair(r->head.name,r)); });
		edb.insert(std::make_pair("mother",mother_rel));
		edb.insert(std::make_pair("father",father_rel));

		std::cout << eval(answer("X","Y"),idb,edb) << std::endl;
	}

	void testMutualRec(void)
	{	
		parse a("a"),b("b"),c("c"),answer("answer");

		a("X") >> a("X"),b("X");
//		b("X") >> a("X");
		b("X") >> c("X");
		c("X") >> a("X");

		answer("X") >> a("X");
		std::map<std::string,rel_ptr> edb;
		std::multimap<std::string,rule_ptr> idb;

		std::for_each(a.rules.begin(),a.rules.end(),[&](rule_ptr r) { idb.insert(std::make_pair(r->head.name,r)); });
		std::for_each(b.rules.begin(),b.rules.end(),[&](rule_ptr r) { idb.insert(std::make_pair(r->head.name,r)); });
		std::for_each(c.rules.begin(),c.rules.end(),[&](rule_ptr r) { idb.insert(std::make_pair(r->head.name,r)); });
		std::for_each(answer.rules.begin(),answer.rules.end(),[&](rule_ptr r) { idb.insert(std::make_pair(r->head.name,r)); });
		edb.insert(std::make_pair("c",rel_ptr(new relation())));

		std::cout << eval(answer("X"),idb,edb) << std::endl;
	}
};
