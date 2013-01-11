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
#include <unordered_set>
#include <unordered_map>
#include <typeinfo>
#include <typeindex>
#include <boost/variant.hpp>
#include <cstring>
#include <memory>

struct variable;
class relation;
struct predicate;
struct rg_node;
struct rule;

typedef boost::variant<unsigned int,std::string> variant;

namespace std 
{
	template<>
	class hash<::variant>
	{
	public:
    size_t operator()(const ::variant &v) const 
    {
			if(v.type() == typeid(bool))
				return hash<bool>()(::boost::get<bool>(v));
			else if(v.type() == typeid(unsigned int))
				return hash<unsigned int>()(::boost::get<unsigned int>(v));
			else if(v.type() == typeid(string))
				return hash<string>()(::boost::get<string>(v));
			else
				assert(false);
    }
	};	
	
	template<>
	class hash<vector<::variant>>
	{
	public:
    size_t operator()(const vector<::variant> &s) const 
    {
			return std::accumulate(s.cbegin(),s.cend(),0,[](size_t acc, const ::variant &v)
				{ return acc ^ hash<::variant>()(v); });
    }
	};	
}

class relation
{
public:
	typedef std::vector<variant> row;
	
	const std::vector<row> &rows(void) const;
	std::set<unsigned int> *find(const std::vector<variable> &b) const;

	void insert(const row &r);
	void reject(std::function<bool(const row &)> f);

private:
	std::vector<row> m_rows;
	mutable std::vector<std::unordered_multimap<variant,unsigned int>> m_indices;

	void index(void) const;
};
typedef std::shared_ptr<relation> rel_ptr;

struct variable
{
	variable(bool b, variant v, std::string n);

	bool bound;
	variant instantiation;
	std::string name;
};

bool operator==(const variable &a, const variable &b);
std::ostream &operator<<(std::ostream &os, const variable &v);

template<typename T>
variable bound(T t)
{
	return variable(true,variant(t),"");
}

variable symbolic(std::string n);

struct predicate
{
	predicate(std::string n, rel_ptr r);
	predicate(std::string n, std::initializer_list<variable> &lst);
	predicate(std::string n, const std::vector<variable> &lst);

	std::string name;
	std::vector<variable> variables;
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

template<typename... Args>
void insert(rel_ptr rel, Args&&... args)
{
	assert(rel->rows().empty() || rel->rows().begin()->size() == sizeof...(args));
	
	std::vector<variant> nr({variant(args)...});
	rel->insert(nr);
}

variable find_helper(const ub &);
variable find_helper(variant v);

template<typename... Args>
rel_ptr find(rel_ptr rel, Args&&... args)
{
	assert(rel->rows().empty() || rel->rows().begin()->size() == sizeof...(args));
	
	std::vector<variable> nr({variable(args)...});
	std::set<unsigned int> *idx = rel->find(nr);
	rel_ptr ret(new relation());

	if(idx)
	{
		auto i = idx->begin();
		while(i != idx->end())
			ret->insert(rel->rows()[*i++]);
		delete idx;
	}

	return ret;
}

std::set<rg_node *> build_graph(std::list<rule> &rules);
bool union_compatible(const rel_ptr &a, const rel_ptr &b);

std::ostream &operator<<(std::ostream &os, const rel_ptr &a);

// p,q predicates
// p `derives' q
// p -> q
// p occurs in the body of a rule whose head is q
//std::set<predicate *> derives(const predicate &q, const database &db);

bool step(const predicate &lhs, const std::vector<const predicate*> &rhs, const std::vector<const relation::row*> &rows, std::unordered_set<relation::row> &out);
rel_ptr single(const rule &r, std::map<std::string,rel_ptr> &rels);
rel_ptr eval(class parse_i query, std::list<rule> &in, std::map<std::string,rel_ptr> &extensional);

#endif
