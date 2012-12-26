#include "dsl.hh"

parse::parse(std::string n,std::initializer_list<std::string> il)
: name(n), columns(il)
{
	return;
}

parse_h::parse_h(parse_i &p, parse_i &o)
: parent(p.parent), variables(p.variables), first(o)
{
	return;
}

parse_i::parse_i(parse &p, std::vector<variable> &v)
: parent(p), variables(v)
{
	return;
}

parse_i::parse_i(parse &p, std::list<variable> &v)
: parent(p)
{ 
	copy(v.begin(),v.end(),std::inserter(variables,variables.begin())); 
}

void fill(std::list<variable> &p)
{
	return;
}

parse_i operator,(parse_i lhs, parse_i rhs)
{
	parse_i ret(lhs);

	ret.tail.push_back(parse_i(rhs.parent,rhs.variables));
	copy(rhs.tail.begin(),rhs.tail.end(),std::inserter(ret.tail,ret.tail.end()));
	return ret;
}

parse_h operator,(parse_h h, parse_i i)
{
	h.parent.rules.pop_back();
	add(h,&i);
	return h;
}

parse_h operator>>(parse_i lhs, parse_i rhs)
{
	parse_h h(lhs,rhs);
	add(h,0);
	return h;
}

void add(parse_h &h, parse_i *opt)
{
	std::cout << "rule for " << h.parent.name << std::endl;
	std::list<predicate> preds;

	std::function<predicate(const parse_i&)> f = [](const parse_i &p)
		{ return predicate(p.parent.name,p.variables); };

	preds.push_back(f(h.first));
	for(const parse_i &p: h.first.tail)
		preds.push_back(f(p));

	if(opt)
	{
		preds.push_back(f(*opt));
		for(const parse_i &p: opt->tail)
			preds.push_back(f(p));
	}

	h.parent.rules.push_back(rule(predicate(h.parent.name,h.variables),preds));
}

