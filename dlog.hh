#ifndef DLOG_HH
#define DLOG_HH

#include <set>
#include <map>
#include <algorithm>
#include <functional>
#include <cassert>
#include <string>
#include <iomanip>
#include <sstream>
#include <list>
#include <typeinfo>
#include <typeindex>
#include <boost/variant.hpp>
#include <cstring>

class relation
{
public:
	typedef boost::variant<bool,std::string> variant;
	typedef std::vector<variant> row;

	relation(std::initializer_list<std::string> ns = std::initializer_list<std::string>());
	relation(const std::vector<std::string> &ns);

	void mutate(std::function<void(std::vector<std::string> &,std::vector<row> &)> f);
	
	const std::vector<std::string> &names(void) const;
	const std::vector<std::type_index> &types(void) const;
	const std::vector<row> &rows(void) const;

	size_t column(std::string n) const;

private:
	std::vector<std::string> m_names;
	std::vector<std::type_index> m_types;
	std::vector<row> m_rows;
};

struct variable
{
	variable(bool b, relation::variant v, std::string n);

	bool bound;
	relation::variant instantiation;
	std::string name;
};

bool operator==(const variable &a, const variable &b);
std::ostream &operator<<(std::ostream &os, const variable &v);

template<typename T>
variable bound(T t)
{
	return variable(true,relation::variant(t),"");
}

variable symbolic(std::string n);

struct predicate
{
	predicate(std::string n, relation r);
	predicate(std::string n, std::initializer_list<variable> &lst);
	predicate(std::string n, const std::vector<variable> &lst);

	std::string name;
	bool intensional;

	std::vector<variable> variables;	// intensional == true
	relation table;										// intensional == false
};

bool operator==(const predicate &a, const predicate &b);
std::ostream &operator<<(std::ostream &os, const predicate &p);

struct rule
{
	rule(predicate h);
	rule(predicate h, std::initializer_list<predicate> &lst);
	rule(predicate h, const std::list<predicate> &lst);
	
	predicate head;
	std::list<predicate> body;
};

std::ostream &operator<<(std::ostream &os, const rule &r);

struct rg_node
{
	rg_node(rule *r);
	rg_node(predicate *p);

	bool goal;
	union { rule *rule_node; predicate *goal_node; } d;

	std::set<rg_node *> children;
};

struct database
{
	database(std::initializer_list<rule> &lst);
	database(const std::list<rule> &lst);

	void build_graph(void);
	
	std::list<rule> rules;
	std::set<rg_node *> extensional;
	std::set<rg_node *> rg_graph;
};

std::ostream &operator<<(std::ostream &os, const database &db);

template<typename... Args>
relation make_relation(Args&&... args)
{
	return relation({args...});
}

template<typename... Args>
void insert_row(relation &rel, Args&&... args)
{
	assert(rel.names().size() == sizeof...(args));
	
	std::vector<relation::variant> nr({relation::variant(args)...});

	rel.mutate([&](std::vector<std::string> &cols, std::vector<std::vector<relation::variant>> &rows)
	{
		rows.push_back(nr);
	});
}

template<typename T>
void insert_column(relation &rel, std::string n, const T &t)
{
	rel.mutate([&](std::vector<std::string> &cols, std::vector<std::vector<relation::variant>> &rows)
	{
		cols.push_back(n);
		for(std::vector<relation::variant> &r: rows)
			r.push_back(relation::variant(t));
	});
}

bool union_compatible(const relation &a, const relation &b);
relation set_union(const relation &a, const relation &b);
relation set_difference(const relation &a, const relation &b);
relation set_intersection(const relation &a, const relation &b);
relation projection(const std::set<size_t> &cols, const relation &a);
relation projection(std::initializer_list<size_t> cols, const relation &a);
relation selection(std::function<bool(const std::vector<relation::variant>&)> p, const relation &a);
relation rename(const relation &a, std::string from, std::string to);
relation join(const relation &a, const relation &b, std::string attr);

std::ostream &operator<<(std::ostream &os, const relation &a);

// p,q predicates
// p `derives' q
// p -> q
// p occurs in the body of a rule whose head is q
std::set<predicate *> derives(const predicate &q, const database &db);

relation step(const predicate &rule_a, const relation &rel_a, const predicate &rule_b, const relation &rel_b);
relation step(const rule &r, std::map<std::string,relation> &rels);
//void eval(database &db, class parse_i query);
relation eval(class parse_i query,std::map<std::string,relation> &context);

#endif
