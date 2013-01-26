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
public:
	parse_i(parse &p, std::vector<variable> &v);

	parse &parent;
	std::vector<variable> variables;
	std::list<parse_i> tail;
	bool negated;
};

struct parse
{
	parse(std::string n);

	template<typename... Tail>
	parse_i operator()(Tail&&... tail)
	{
		std::vector<variable> vars;

		fill(vars,tail...);
		return parse_i(*this,vars);
	}

	std::string name;
	std::vector<rule_ptr> rules;
};

class parse_h
{
public:
	parse_h(parse_i &p,parse_i &o);

	parse &parent;
	std::vector<variable> variables;
	rule_ptr cur_rule;
};

class parse_c
{
public:
	parse_c(constraint c);

	std::list<constraint> constraints;
};

void fill(std::vector<variable> &p);

template<typename Head>
void fill(std::vector<variable> &p, Head head)
{
	p.push_back(bound<Head>(head));
}

template<>
inline void fill(std::vector<variable> &p, variable head)
{
	p.push_back(head);
}

template<typename Head, typename... Tail>
void fill(std::vector<variable> &p, Head head, Tail&&... tail)
{
	p.push_back(bound<Head>(head));
	fill(p,tail...);
}
	
template<typename... Tail>
void fill(std::vector<variable> &p, variable head, Tail&&... tail)
{
	p.push_back(head);
	fill(p,tail...);
}

parse_c operator<(variant a, variable b);
parse_c operator<(variable a, variable b);
parse_c operator<(variable a, variant b);
parse_c operator<=(variant a, variable b);
parse_c operator<=(variable a, variable b);
parse_c operator<=(variable a, variant b);
parse_c operator>(variant a, variable b);
parse_c operator>(variable a, variable b);
parse_c operator>(variable a, variant b);
parse_c operator>=(variant a, variable b);
parse_c operator>=(variable a, variable b);
parse_c operator>=(variable a, variant b);

parse_i operator!(parse_i i);
parse_h operator,(parse_h h, parse_i i);
parse_h operator,(parse_h h, parse_c c);
parse_h operator<<(parse_i lhs, parse_i rhs);

#endif
