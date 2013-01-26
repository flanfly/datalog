#include "dsl.hh"

parse::parse(std::string n)
: name(n)
{
	return;
}

parse_h::parse_h(parse_i &head, parse_i &tail)
: parent(head.parent), variables(head.variables)
{
	cur_rule = rule_ptr(new rule(predicate(head.parent.name,head.variables,false),{predicate(tail.parent.name,tail.variables,tail.negated)}));
	parent.rules.push_back(cur_rule);
}

parse_i::parse_i(parse &p, std::vector<variable> &v)
: parent(p), variables(v), negated(false)
{
	return;
}

parse_c::parse_c(constraint c)
: constraints({c})
{
	return;
}

void fill(std::vector<variable> &p)
{
	return;
}

parse_h operator,(parse_h h, parse_i i)
{
	//std::cout << "operator,(h,i)" << std::endl;
	h.cur_rule->body.push_back(predicate(i.parent.name,i.variables,i.negated));
	return h;
}

parse_h operator,(parse_h h, parse_c c)
{
	//std::cout << "operator,(h,c)" << std::endl;
	std::copy(c.constraints.begin(),c.constraints.end(),std::inserter(h.cur_rule->constraints,h.cur_rule->constraints.end()));
	return h;
}

parse_h operator<<(parse_i lhs, parse_i rhs)
{
	//std::cout << "operator<<(i,i)" << std::endl;
	return parse_h(lhs,rhs);
}

parse_i operator!(parse_i i)
{
	i.negated = !i.negated;
	return i;
}

parse_c operator<(variant a, variable b)
{
	return parse_c(constraint(constraint::Less,variable(true,a,""),b));
}

parse_c operator<(variable a, variable b)
{
	return parse_c(constraint(constraint::Less,a,b));
}

parse_c operator<(variable a, variant b)
{
	return parse_c(constraint(constraint::Less,a,variable(true,b,"")));
}

parse_c operator<=(variant a, variable b)
{
	return parse_c(constraint(constraint::LessOrEqual,variable(true,a,""),b));
}

parse_c operator<=(variable a, variable b)
{
	return parse_c(constraint(constraint::LessOrEqual,a,b));
}

parse_c operator<=(variable a, variant b)
{
	return parse_c(constraint(constraint::LessOrEqual,a,variable(true,b,"")));
}

parse_c operator>(variant a, variable b)
{
	return parse_c(constraint(constraint::Greater,variable(true,a,""),b));
}

parse_c operator>(variable a, variable b)
{
	return parse_c(constraint(constraint::Greater,a,b));
}

parse_c operator>(variable a, variant b)
{
	return parse_c(constraint(constraint::Greater,a,variable(true,b,"")));
}

parse_c operator>=(variant a, variable b)
{
	return parse_c(constraint(constraint::GreaterOrEqual,variable(true,a,""),b));
}

parse_c operator>=(variable a, variable b)
{
	return parse_c(constraint(constraint::GreaterOrEqual,a,b));
}

parse_c operator>=(variable a, variant b)
{
	return parse_c(constraint(constraint::GreaterOrEqual,a,variable(true,b,"")));
}
