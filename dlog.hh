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

//bool operator<(const variant &a, const variant &b);
bool operator<=(const variant &a, const variant &b);
bool operator>(const variant &a, const variant &b);
bool operator>=(const variant &a, const variant &b);

class relation
{
public:
	typedef std::vector<variant> row;
	
	const std::vector<row> &rows(void) const;
	std::set<unsigned int> *find(const std::vector<variable> &b) const;
	bool includes(const relation::row &r) const;

	bool insert(const row &r);
	bool insert(std::shared_ptr<relation> r);
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
inline variable operator"" _dl(const char *s, size_t sz)
{
	return variable(false,"",std::string(s));
}

template<typename T>
variable bound(T t)
{
	return variable(true,variant(t),"");
}

variable symbolic(std::string n);

struct predicate
{
	predicate(std::string n, std::initializer_list<variable> &lst, bool m);
	predicate(std::string n, const std::vector<variable> &lst, bool m);

	std::string name;
	std::vector<variable> variables;
	bool negated;
};

bool operator==(const predicate &a, const predicate &b);
std::ostream &operator<<(std::ostream &os, const predicate &p);

struct constraint
{
	enum Type
	{
		Less, LessOrEqual, Greater, GreaterOrEqual,
	};

	constraint(Type t, variable a, variable b);
	bool operator()(const std::unordered_map<std::string,unsigned int> &binding, const relation::row &r) const;

	Type type;
	variable operand1, operand2;
};

std::ostream &operator<<(std::ostream &os, const constraint &c);

struct rule
{
	rule(predicate h);
	rule(predicate h, std::initializer_list<predicate> &lst);
	rule(predicate h, const std::list<predicate> &plst);
	
	predicate head;
	std::list<predicate> body;
	std::list<constraint> constraints;
};
typedef std::shared_ptr<rule> rule_ptr;

std::ostream &operator<<(std::ostream &os, const rule &r);

template<typename... Args>
void insert(rel_ptr rel, Args&&... args)
{
	assert(rel->rows().empty() || rel->rows().begin()->size() == sizeof...(args));
	
	std::vector<variant> nr({variant(args)...});
	rel->insert(nr);
}

/*
variable find_helper(const ub &);
variable find_helper(variant v);

template<typename... Args>
rel_ptr find(rel_ptr rel, Args&&... args)
{
	assert(rel->rows().empty() || rel->rows().begin()->size() == sizeof...(args));
	
	std::vector<variable> nr({find_helper(args)...});
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
}*/

std::ostream &operator<<(std::ostream &os, const relation &a);
rel_ptr eval(std::string query, std::multimap<std::string,rule_ptr> &in, std::map<std::string,rel_ptr> &extensional);

#endif
