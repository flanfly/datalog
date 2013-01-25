#include <cppunit/extensions/HelperMacros.h>

#include "dlog.hh"
#include "dsl.hh"

class DESTest : public CppUnit::TestFixture  
{
	CPPUNIT_TEST_SUITE(DESTest);
	CPPUNIT_TEST(testFamily);
	CPPUNIT_TEST(testGame);
	CPPUNIT_TEST(testMrTc);
	CPPUNIT_TEST_SUITE_END();

public:
  void setUp(void) {}
  void tearDown(void) {}
  
	// shamelessly ripped-off DES (des.sourceforge.net)
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
		
		rel_ptr expected_rel(new relation());
		insert(expected_rel,"amy","carol_III");
		insert(expected_rel,"amy","fred");
		insert(expected_rel,"carol_I","carol_II");
		insert(expected_rel,"carol_I","carol_III");
		insert(expected_rel,"carol_II","carol_III");
		insert(expected_rel,"fred","carol_III");
		insert(expected_rel,"grace","amy");
		insert(expected_rel,"grace","carol_III");
		insert(expected_rel,"grace","fred");
		insert(expected_rel,"jack","carol_III");
		insert(expected_rel,"jack","fred");
		insert(expected_rel,"tom","amy");
		insert(expected_rel,"tom","carol_III");
		insert(expected_rel,"tom","fred");
		insert(expected_rel,"tony","carol_II");
		insert(expected_rel,"tony","carol_III");

		parse parent("parent"),ancestor("ancestor"),father("father"),mother("mother"),answer("answer");

		parent("X"_dl,"Y"_dl) << father("X"_dl,"Y"_dl);
		parent("X"_dl,"Y"_dl) << mother("X"_dl,"Y"_dl);

		ancestor("X"_dl,"Y"_dl) << parent("X"_dl,"Y"_dl);
		ancestor("X"_dl,"Z"_dl) << parent("X"_dl,"Y"_dl),ancestor("Y"_dl,"Z"_dl);

		answer("X"_dl,"Y"_dl) << ancestor("X"_dl,"Y"_dl);

		std::map<std::string,rel_ptr> edb;
		std::multimap<std::string,rule_ptr> idb;

		std::for_each(parent.rules.begin(),parent.rules.end(),[&](rule_ptr r) { idb.insert(std::make_pair(r->head.name,r)); });
		std::for_each(ancestor.rules.begin(),ancestor.rules.end(),[&](rule_ptr r) { idb.insert(std::make_pair(r->head.name,r)); });
		std::for_each(answer.rules.begin(),answer.rules.end(),[&](rule_ptr r) { idb.insert(std::make_pair(r->head.name,r)); });
		edb.insert(std::make_pair("mother",mother_rel));
		edb.insert(std::make_pair("father",father_rel));

		rel_ptr res = eval("answer",idb,edb);

		CPPUNIT_ASSERT(res);
		CPPUNIT_ASSERT_EQUAL(res->rows().size(),(unsigned long)16);
		for(const relation::row &r: expected_rel->rows())
			CPPUNIT_ASSERT(res->includes(r));
	}

	void testGame(void)
	{
		rel_ptr move_rel(new relation());
		insert(move_rel,1,2);
		insert(move_rel,2,3);
		insert(move_rel,3,4);
		insert(move_rel,1,3);
		insert(move_rel,1,5);
	
		rel_ptr expected_rel(new relation());
		insert(expected_rel,1);
		insert(expected_rel,3);

		parse move("move"), canMove("canMove"), possible_winning("possible_winning"), winning("winning"), odd_move("odd_move");

		canMove("X") << move("X","Y");
		
		possible_winning("X"_dl) << odd_move("X"_dl,"Y"_dl),!canMove("Y"_dl);
		
		winning("X"_dl) << move("X"_dl,"Y"_dl),!possible_winning("Y"_dl);

		odd_move("X"_dl,"Y"_dl) << move("X"_dl,"Y"_dl);
		odd_move("X"_dl,"Y"_dl) << move("X"_dl,"Z1"_dl),move("Z1"_dl,"Z2"_dl),odd_move("Z2"_dl,"Y"_dl);

			std::map<std::string,rel_ptr> edb;
		std::multimap<std::string,rule_ptr> idb;

		std::for_each(canMove.rules.begin(),canMove.rules.end(),[&](rule_ptr r) { idb.insert(std::make_pair(r->head.name,r)); });
		std::for_each(possible_winning.rules.begin(),possible_winning.rules.end(),[&](rule_ptr r) { idb.insert(std::make_pair(r->head.name,r)); });
		std::for_each(winning.rules.begin(),winning.rules.end(),[&](rule_ptr r) { idb.insert(std::make_pair(r->head.name,r)); });
		std::for_each(odd_move.rules.begin(),odd_move.rules.end(),[&](rule_ptr r) { idb.insert(std::make_pair(r->head.name,r)); });
		edb.insert(std::make_pair("move",move_rel));

		rel_ptr res = eval("winning",idb,edb);

		CPPUNIT_ASSERT(res);
		CPPUNIT_ASSERT_EQUAL(res->rows().size(),(unsigned long)2);
		for(const relation::row &r: expected_rel->rows())
			CPPUNIT_ASSERT(res->includes(r));
	}

	void testMrTc(void)
	{
		rel_ptr p_rel(new relation());
		insert(p_rel,"a","b");
		insert(p_rel,"c","d");
		
		rel_ptr q_rel(new relation());
		insert(q_rel,"b","c");
		insert(q_rel,"d","e");
		
		rel_ptr p1_rel(new relation());
		insert(p1_rel,"a1","b1");
		insert(p1_rel,"c1","d1");
		
		rel_ptr q1_rel(new relation());
		insert(q1_rel,"b1","c1");
		insert(q1_rel,"d1","e1");
	
		rel_ptr expected_rel(new relation());
		insert(expected_rel,"a","b");
		insert(expected_rel,"a","c");
		insert(expected_rel,"a","d");
		insert(expected_rel,"a","e");
		insert(expected_rel,"a1","b1");
		insert(expected_rel,"a1","c1");
		insert(expected_rel,"a1","d1");
		insert(expected_rel,"a1","e1");
		insert(expected_rel,"b","c");
		insert(expected_rel,"b","d");
		insert(expected_rel,"b","e");
		insert(expected_rel,"b1","c1");
		insert(expected_rel,"b1","d1");
		insert(expected_rel,"b1","e1");
		insert(expected_rel,"<c","d");
		insert(expected_rel,"c","e");
		insert(expected_rel,"c1","d1");
		insert(expected_rel,"c1","e1");
		insert(expected_rel,"d","e");
		insert(expected_rel,"d1","e1");

		parse p("p"), q("q"), pqs("pqs"), p1("p1"), q1("q1"), pqs1("pqs1");
		
		pqs("X"_dl,"Y"_dl) << p("X"_dl,"Y"_dl);
		pqs("X"_dl,"Y"_dl) << q("X"_dl,"Y"_dl);
		pqs("X"_dl,"Y"_dl) << pqs("X"_dl,"Z"_dl),p("Z"_dl,"Y"_dl);
		pqs("X"_dl,"Y"_dl) << pqs("X"_dl,"Z"_dl),q("Z"_dl,"Y"_dl);
		pqs("X"_dl,"Y"_dl) << pqs1("X"_dl,"Y"_dl);

		pqs1("X"_dl,"Y"_dl) << p1("X"_dl,"Y"_dl);
		pqs1("X"_dl,"Y"_dl) << q1("X"_dl,"Y"_dl);
		pqs1("X"_dl,"Y"_dl) << pqs1("X"_dl,"Z"_dl),p1("Z"_dl,"Y"_dl);
		pqs1("X"_dl,"Y"_dl) << pqs1("X"_dl,"Z"_dl),q1("Z"_dl,"Y"_dl);

		std::map<std::string,rel_ptr> edb;
		std::multimap<std::string,rule_ptr> idb;

		std::for_each(pqs.rules.begin(),pqs.rules.end(),[&](rule_ptr r) { idb.insert(std::make_pair(r->head.name,r)); });
		std::for_each(pqs1.rules.begin(),pqs1.rules.end(),[&](rule_ptr r) { idb.insert(std::make_pair(r->head.name,r)); });
		edb.insert(std::make_pair("p",p_rel));
		edb.insert(std::make_pair("q",q_rel));
		edb.insert(std::make_pair("p1",p1_rel));
		edb.insert(std::make_pair("q1",q1_rel));

		rel_ptr res = eval("pqs",idb,edb);

		CPPUNIT_ASSERT(res);
		CPPUNIT_ASSERT_EQUAL(res->rows().size(),(unsigned long)20);
		for(const relation::row &r: expected_rel->rows())
			CPPUNIT_ASSERT(res->includes(r));
	}
};
