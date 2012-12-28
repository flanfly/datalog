#ifndef DSL_HH
#define DSL_HH

#include <string>
#include <list>
#include <vector>

struct parse;
class parse_i;
class parse_h;

#include "dlog.hh"

class parse_i
{
private:
	parse_i(parse &p, std::vector<variable> &v);
	parse_i(parse &p, std::list<variable> &v);

public:
	parse &parent;
	std::vector<variable> variables;
	std::list<parse_i> tail;

	friend struct parse;
	friend parse_i operator,(parse_i lhs, parse_i rhs);
};

struct parse
{
	parse(std::string n);

	template<typename... Tail>
	parse_i operator()(Tail&&... tail)
	{
		std::list<variable> vars;

		fill(vars,tail...);
		return parse_i(*this,vars);
	}

	std::string name;
	std::list<rule> rules;
};

class parse_h
{
private:
	parse_h(parse_i &p,parse_i &o);

public:
	parse &parent;
	std::vector<variable> variables;
	parse_i first;
	
	friend parse_h operator,(parse_h h, parse_i i);
	friend parse_h operator>>(parse_i lhs, parse_i rhs);
};

void fill(std::list<variable> &p);

template<typename Head>
void fill(std::list<variable> &p, Head head)
{
	p.push_back(bound<Head>(head));
}

template<>
inline void fill(std::list<variable> &p, std::string head)
{
	assert(head.size());
	p.push_back(isupper(head[0]) ? symbolic(head) : bound<std::string>(head));
}

template<>
inline void fill(std::list<variable> &p, const char *head)
{
	assert(strlen(head));
	p.push_back(isupper(head[0]) ? symbolic(head) : bound<std::string>(head));
}

template<typename Head, typename... Tail>
void fill(std::list<variable> &p, Head head, Tail&&... tail)
{
	p.push_back(bound<Head>(head));
	fill(p,tail...);
}
	
template<typename... Tail>
void fill(std::list<variable> &p, std::string head, Tail&&... tail)
{
	assert(head.size());
	p.push_back(isupper(head[0]) ? symbolic(head) : bound<std::string>(head));
	fill(p,tail...);
}

template<typename... Tail>
void fill(std::list<variable> &p, const char *head, Tail&&... tail)
{
	assert(strlen(head));
	p.push_back(isupper(head[0]) ? symbolic(head) : bound<std::string>(head));
	fill(p,tail...);
}

parse_i operator,(parse_i lhs, parse_i rhs);
parse_h operator,(parse_h h, parse_i i);
parse_h operator>>(parse_i lhs, parse_i rhs);
void add(parse_h &h, parse_i *opt);

#endif
