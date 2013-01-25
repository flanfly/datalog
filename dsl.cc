#include "dsl.hh"

parse::parse(std::string n)
: name(n)
{
	return;
}

parse_h::parse_h(parse_i &head, parse_i &tail)
: parent(head.parent), variables(head.variables)
{
	parent.rules.push_back(rule_ptr(new rule(predicate(head.parent.name,head.variables,false),{predicate(tail.parent.name,tail.variables,tail.negated)})));
	index = parent.rules.size() - 1;
}

parse_i::parse_i(parse &p, std::vector<variable> &v)
: parent(p), variables(v), negated(false)
{
	return;
}

parse_i::parse_i(parse &p, std::list<variable> &v)
: parent(p), negated(false)
{ 
	copy(v.begin(),v.end(),std::inserter(variables,variables.begin())); 
}

void fill(std::list<variable> &p)
{
	return;
}

parse_h operator,(parse_h h, parse_i i)
{
	//std::cout << "operator,(h,i)" << std::endl;
	h.parent.rules[h.index]->body.push_back(predicate(i.parent.name,i.variables,i.negated));
	return h;
}

parse_h operator>>(parse_i lhs, parse_i rhs)
{
	//std::cout << "operator>>(i,i)" << std::endl;
	return parse_h(lhs,rhs);
}

parse_i operator!(parse_i i)
{
	i.negated = !i.negated;
	return i;
}
