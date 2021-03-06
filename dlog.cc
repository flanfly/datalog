#include <iostream>
#include <algorithm>

#include "dlog.hh"
#include "dsl.hh"
/*
bool operator<(const variant &a, const variant &b)
{
	if(a.type() == b.type())
	{
		if(a.type() == typeid(bool))
			return boost::get<bool>(a) < boost::get<bool>(b);
		else if(a.type() == typeid(unsigned int))
			return boost::get<unsigned int>(a) < boost::get<unsigned int>(b);
		else if(a.type() == typeid(std::string))
			return boost::get<std::string>(a) < boost::get<std::string>(b);
		else
			assert(false);
	}
	else
		return std::type_index(a.type()) < std::type_index(b.type());
}*/

bool operator<=(const variant &a, const variant &b)
{
	return a == b || a < b;
}

bool operator>(const variant &a, const variant &b)
{
	return !(a <= b);
}

bool operator>=(const variant &a, const variant &b)
{
	return a == b || a > b;
}

const std::vector<relation::row> &relation::rows(void) const
{
	return m_rows;
}

std::set<unsigned int> *relation::find(const std::vector<variable> &b) const
{
	if(m_rows.empty()) return 0;
	assert(b.size() == m_rows[0].size());

	if(m_indices.empty()) index();
	assert(b.size() == m_indices.size());
	
	unsigned int col = 0;
	std::set<unsigned int> *ret = 0;
	std::multimap<std::string,unsigned int> unbound;
	bool all = true;
	bool last_pass = false;

	while(col < b.size())
	{
		const variable &var = b[col];
		const std::unordered_multimap<variant,unsigned int> &idx = m_indices[col];

		if(var.bound)
		{
			auto n = idx.equal_range(var.instantiation);

			if(!ret)
			{
				ret = new std::set<unsigned int>();
				while(n.first != n.second)
					ret->insert((n.first++)->second);
			}
			else
			{
				std::set<unsigned int> eqr;
				std::set<unsigned int> *nret = new std::set<unsigned int>();

				while(n.first != n.second)
					eqr.insert((n.first++)->second);

				std::set_intersection(eqr.begin(),eqr.end(),ret->begin(),ret->end(),std::inserter(*nret,nret->begin()));
				delete ret;
				ret = nret;
			}

			all = false;
		}
		else
		{
			last_pass |= unbound.count(var.name) > 0;
			unbound.insert(std::make_pair(var.name,col));
		}

		++col;
	}

	if(all)
	{
		unsigned int i = 0;

		ret = new std::set<unsigned int>();
		while(i < m_rows.size())
			ret->insert(i++);
	}

	assert(ret);

	if(last_pass && unbound.size() > 1)
	{
		auto i = ret->begin();
		while(i != ret->end())
		{
			const row &r = m_rows[*i];
			auto j = unbound.begin();

			while(std::next(j) != unbound.end())
			{
				auto n = std::next(j);

				if(n->first == j->first)
				{
					if(!(r[n->second] == r[j->second]))
					{
						i = ret->erase(i);
						break;
					}
				}
				++j;
			}

			++i;
		}
	}
						
	return ret;
}
bool relation::includes(const relation::row &r) const
{
	std::vector<variable> b;
	std::set<unsigned int> *coll;
	bool ret = false;
	
	if(m_rows.empty() || r.size() != m_rows[0].size())
		return ret;

	for(const variant &v: r)
		b.push_back(variable(true,v,""));
	
	coll = find(b);

	if(coll)
	{
		ret = !coll->empty();
		delete coll;
	}

	return ret;
}

bool relation::insert(const relation::row &r)
{
	assert(m_rows.empty() || r.size() == m_rows[0].size());

	if(!includes(r))
	{
		m_rows.push_back(r);
		unsigned int j = 0;

		while(j < r.size())
		{
			while(m_indices.size() <= j)
				m_indices.push_back(std::unordered_multimap<variant,unsigned int>());
			m_indices[j].insert(std::make_pair(r[j],m_rows.size() - 1));

			++j;
		}
		return true;
	}
	else
		return false;
}

bool relation::insert(rel_ptr r)
{
	assert(r);
	bool ret = false;

	for(const relation::row &s: r->rows())
		ret |= insert(s);
	
	return ret;
}

void relation::reject(std::function<bool(const relation::row &)> f)
{
	auto i = m_rows.begin();
	std::vector<row> n;
	
	n.reserve(m_rows.size());
	while(i != m_rows.end())
	{
		if(f(*i))
			m_indices.clear();
		else
			n.push_back(*i);
		++i;
	}

	m_rows = n;
}

void relation::index(void) const
{
	m_indices.clear();

	auto i = m_rows.cbegin();
	while(i != m_rows.cend())
	{
		const row &r = *i;
		unsigned int j = 0;

		while(j < r.size())
		{
			while(m_indices.size() <= j)
				m_indices.push_back(std::unordered_multimap<variant,unsigned int>());
			m_indices[j].insert(std::make_pair(r[j],std::distance(m_rows.begin(),i)));

			++j;
		}

		++i;
	}
}

variable::variable(bool b, variant v, std::string n)
: bound(b), instantiation(v), name(n)
{
	return;
}

bool operator==(const variable &a, const variable &b)
{
	return a.bound == b.bound && 
				((a.bound && a.instantiation == b.instantiation) ||
				(!a.bound && a.name == b.name));
}

std::ostream &operator<<(std::ostream &os, const variable &v)
{	
	if(v.bound)
		os << v.instantiation;
	else
		os << v.name;
	return os;
}

variable symbolic(std::string n)
{
	return variable(false,false,n);
}

predicate::predicate(std::string n, std::initializer_list<variable> &lst, bool m)
: name(n), variables(lst), negated(m)
{
	return;
} 

predicate::predicate(std::string n, const std::vector<variable> &lst, bool m)
: name(n), variables(lst), negated(m)
{
	return;
}

bool operator==(const predicate &a, const predicate &b)
{
	return a.name == b.name && a.variables == b.variables;
}

std::ostream &operator<<(std::ostream &os, const predicate &p)
{
	os << (p.negated ? "!" : "") << p.name << "(";
	
	for(const variable &v: p.variables)
		if(v == *std::prev(p.variables.end(),1))
			os << v;
		else
			os << v << ",";
	
	os << ")";
	return os;
}


constraint::constraint(Type t, variable a, variable b)
: type(t), operand1(a), operand2(b)
{
	return;
}

bool constraint::operator()(const std::unordered_map<std::string,unsigned int> &binding, const relation::row &r) const 
{
	variant a = !operand1.bound ? r.at(binding.at(operand1.name)) : operand1.instantiation,
					b = !operand2.bound ? r.at(binding.at(operand2.name)) : operand2.instantiation;
	
	switch(type)
	{
		case Less: return a < b;
		case LessOrEqual: return a <= b;
		case Greater: return a > b;
		case GreaterOrEqual: return a >= b;
		default: assert(false);
	}
}

std::ostream &operator<<(std::ostream &os, const constraint &c)
{
	os << c.operand1;
	switch(c.type)
	{
		case constraint::Less: os << " > "; break;
		case constraint::LessOrEqual: os << " >= "; break;
		case constraint::Greater: os << " < "; break;
		case constraint::GreaterOrEqual: os << " <= "; break;
		default: assert(false);
	}
	os << c.operand2;

	return os;
}

rule::rule(predicate h)
: head(h)
{
	return;
}

rule::rule(predicate h, std::initializer_list<predicate> &lst)
: head(h), body(lst) 
{
	return;
} 

rule::rule(predicate h, const std::list<predicate> &lst)
: head(h) 
{
	std::copy(lst.begin(),lst.end(),std::inserter(body,body.begin())); 
}

std::ostream &operator<<(std::ostream &os, const rule &r)
{
	os << r.head;
	
	if(r.body.size() || r.constraints.size())
	{
		os << " :- ";
		for(const predicate &p: r.body)
			if(p == *std::prev(r.body.end(),1))
				os << p;
			else
				os << p << ",";
				
		for(const constraint &c: r.constraints)
			os << "," <<  c;
	}

	return os;
}

std::ostream &operator<<(std::ostream &os, const relation &a)
{
	if(a.rows().empty())
	{
		os << "empty" << std::endl;
		return os;
	}

	const size_t cols = a.rows().begin()->size();
	size_t *widths = new size_t[cols];
	std::list<std::list<std::string>> text;
	size_t col = 0;

	while(col < cols)
	{
		widths[col] = 0;
	 	++col;
	}

	auto row = a.rows().begin();
	while(row != a.rows().end())
	{
		col = 0;
		
		text.push_back(std::list<std::string>());

		while(col < cols)
		{
			std::stringstream ss;

			ss << std::boolalpha << row->at(col);
			text.back().push_back(ss.str());
			widths[col] = std::max(ss.str().size(),widths[col]);
	 		++col;
		}

		++row;
	}

	std::function<void(void)> do_line = [&](void)
	{
		os << std::setfill('-');
		col = 0;
		while(col < cols)
			os << "+-" << std::setw(widths[col++]) << "-" << "-";
		os << "+" << std::endl << std::setfill(' ');
	};

	do_line();

	// rows
	for(const std::list<std::string> &r: text)
	{
		col = 0;
		while(col < cols)
		{
			os << "| " << std::setw(widths[col]); 
			os << *std::next(r.begin(),col++) << " ";
		}
		os << "| " << std::endl;
	}
	
	do_line();
	delete[] widths;

	return os;
}

/*
variable find_helper(const ub &)
{
	return variable(false,"","X");
}

variable find_helper(variant v)
{
	return variable(true,v,"");
}*/

rel_ptr join(const std::vector<variable> &a_bind,const rel_ptr a_rel,const std::vector<variable> &b_bind,const rel_ptr b_rel)
{
	assert(a_rel && b_rel);
	std::set<unsigned int> *a_idx = a_rel->find(a_bind);
	std::multimap<unsigned int,unsigned int> cross_vars; // a -> b
	rel_ptr ret(new relation());

	if(!a_idx)
		return ret;

	auto i = a_bind.begin();
	while(i != a_bind.end())
	{
		if(!i->bound)
		{
			auto j = b_bind.begin();
			while(j != b_bind.end())
			{
				if(!j->bound && i->name ==j->name)
			 		cross_vars.insert(std::make_pair(std::distance(a_bind.begin(),i),std::distance(b_bind.begin(),j)));
				++j;
			}
		}
		++i;
	}

	for(unsigned int a_ri: *a_idx)
	{	
		const relation::row &r = a_rel->rows()[a_ri];
		std::vector<variable> binding(b_bind);

		for(const std::pair<unsigned int,unsigned int> &xv: cross_vars)
		{
			binding[xv.second].instantiation = variant(r[xv.first]);
			binding[xv.second].bound = true;
		}

		std::set<unsigned int> *b_idx = b_rel->find(binding);
		if(b_idx)
		{
			for(unsigned int b_ri: *b_idx)
			{
				relation::row nr(r);
				const relation::row &s = b_rel->rows()[b_ri];

				std::copy(s.begin(),s.end(),std::inserter(nr,nr.end()));
				ret->insert(nr);
			}
			delete b_idx;
		}
	}

	delete a_idx;
	return ret;
}

rel_ptr eval_rule(const rule_ptr r, const std::vector<rel_ptr> &relations)
{
	assert(r);

	rel_ptr temp(new relation());
	std::vector<variable> binding;

	if(r->body.empty())
		return temp;

	// non-negated predicates
	if(std::count_if(r->body.begin(),r->body.end(),[](const predicate &p) { return !p.negated; }) > 1)
	{
		auto i = r->body.begin();
		bool first = true;

		while(i != r->body.end())
		{
			if(!i->negated)
			{
				if(first)
				{
					unsigned int j = std::distance(r->body.begin(),i);
					temp = join(i->variables,relations[j],std::next(i)->variables,relations[j+1]);
					binding = i->variables;

					++i;
					first = false;
				}
				else
				{
					temp = join(binding,temp,i->variables,relations[std::distance(r->body.begin(),i)]);
				}
				
				std::copy(i->variables.begin(),i->variables.end(),std::inserter(binding,binding.end()));
			}


			++i;
		}
	}
	else
	{
		unsigned int i = std::distance(r->body.begin(),std::find_if(r->body.begin(),r->body.end(),[](const predicate &p) { return !p.negated; }));
		const rel_ptr rel = relations[i];
		const std::vector<variable> &vars = std::next(r->body.begin(),i)->variables;
		std::set<unsigned int> *s = rel->find(vars);

		if(s)
		{
			for(unsigned int i: *s)
				temp->insert(rel->rows()[i]);
			binding = vars;
			delete s;
		}
	}

	// build index from varname to column number in temporary relation 'temp'
	std::unordered_map<std::string,unsigned int> common; // varname -> temp rel column
	auto j = binding.begin();
	while(j != binding.end())
	{
		const variable &w = *j;
		
		if(!w.bound)
			common.insert(std::make_pair(w.name,std::distance(binding.begin(),j)));
		++j;
	}

	// negated predicates
	auto i = r->body.begin();

	while(i != r->body.end())
	{
		if(i->negated)
		{
			rel_ptr rel = relations[std::distance(r->body.begin(),i)];
			relation::row b(i->variables.size(),"");
			unsigned int j = 0;

			while(j < i->variables.size())
			{
				if(i->variables[j].bound)
					b[j] = i->variables[j].instantiation;
				++j;
			}

			temp->reject([&](const relation::row &r) -> bool
			{
				j = 0;

				while(j < i->variables.size())
				{
					if(!i->variables[j].bound)
						b[j] = r[common[i->variables[j].name]];
					++j;
				}
				
				return rel->includes(b);
			});
		}

		++i;
	}
	
	// project onto head predicate
	rel_ptr ret(new relation());
	for(const relation::row &rr: temp->rows())
	{
		relation::row nr;

		for(const variable &v: r->head.variables)
			if(v.bound)
				nr.push_back(v.instantiation);
			else
				nr.push_back(rr[common[v.name]]);
		ret->insert(nr);
	}

	return ret;
}

bool derives(const std::multimap<std::string,rule_ptr> &idb, std::string a, std::string b)
{
	std::set<std::string> known;
	std::function<bool(const rule_ptr)> check;
	
	check = [&](const rule_ptr r)
	{
		return any_of(r->body.begin(),r->body.end(),[&](const predicate &p) 
		{ 
			return p.name == b || 
						 (known.insert(p.name).second && any_of(idb.lower_bound(p.name),idb.upper_bound(p.name),[&](const std::pair<std::string,rule_ptr> &q)
						 		{ return check(q.second); }));
		});
	};
	return any_of(idb.lower_bound(a),idb.upper_bound(a),[&](const std::pair<std::string,rule_ptr> &q) { return check(q.second); });
}

bool mutual_rec(const std::multimap<std::string,rule_ptr> &idb, std::string a, std::string b)
{
	return a == b || (derives(idb,a,b) && derives(idb,b,a));
}

bool is_safe(rule_ptr r)
{
	// every variable in the head must apper in a non-negated predicate in the body
	return r && std::all_of(r->head.variables.begin(),r->head.variables.end(),[&](const variable &v)
	{
		return v.bound || std::any_of(r->body.begin(),r->body.end(),[&](const predicate &p)
		{
			return !p.negated && std::find(p.variables.begin(),p.variables.end(),v) != p.variables.end();
		});
	}) && 
	// every variable occuring in a negated prediacte in the body must occur in a non-negated one too
	std::all_of(r->body.begin(),r->body.end(),[&](const predicate &p)
	{
		return !p.negated || std::all_of(p.variables.begin(),p.variables.end(),[&](const variable &v)
		{
			return v.bound || std::any_of(r->body.begin(),r->body.end(),[&](const predicate &q)
			{
				return !q.negated && std::find(q.variables.begin(),q.variables.end(),v) != q.variables.end();
			});
		});
	});
}

rel_ptr eval(std::string query, std::multimap<std::string,rule_ptr> &idb, std::map<std::string,rel_ptr> &edb)
{
	// TODO only include rules that 'query' depends upon
	
	std::list<std::string> partition; // result of partitioning the idb predicates with mutual_rec()

	for(const std::pair<std::string,rule_ptr> &p: idb)
	{
		if(!is_safe(p.second))
		{
			std::cout << *p.second << " is not safe!" << std::endl;
			return rel_ptr(0);
		}
		if(std::find(partition.begin(),partition.end(),p.first) == partition.end())
			partition.push_back(p.first);
	}

	partition.sort([&](const std::string &a, const std::string &b) -> bool
	{
		if(a == b)
			return false;

		int ab = derives(idb,a,b);
		int ba = derives(idb,b,a);

		return ab < ba;
	});

	auto idx = partition.begin();
	std::map<std::string,rel_ptr> rels(edb);

	while(idx != partition.end())
	{
		auto idx_end = std::find_if(idx,partition.end(),[&](const std::string &s) { return !mutual_rec(idb,*idx,s); });
		std::map<std::string,rel_ptr> deltas;
		std::set<rule_ptr> simple, recursive;
		const unsigned int pos = std::distance(partition.begin(),idx);

		std::for_each(idx,idx_end,[&](const std::string &s)
		{
			std::for_each(idb.lower_bound(s),idb.upper_bound(s),[&](const std::pair<std::string,rule_ptr> &p)
			{
				bool is_recu = !std::all_of(p.second->body.begin(),p.second->body.end(),[&](const predicate &pred)
					{ return edb.count(pred.name) > 0 || pos > std::distance(partition.begin(),std::find(partition.begin(),partition.end(),pred.name)); });

				if(is_recu)
					recursive.insert(p.second);
				else
					simple.insert(p.second);
			});
		});

		// eval all rules w/ body predicates in edb or <idx once
		std::cout << "one shot:" << std::endl;
		for(rule_ptr r: simple)
		{
			assert(r);
			std::cout << *r << std::endl;

			std::vector<rel_ptr> plan;
			const std::string &n = r->head.name;

			for(const predicate &p: r->body)
				plan.push_back(rels[p.name]);

			rel_ptr res = eval_rule(r,plan);

			if(res)
			{
				rel_ptr old = rels.count(n) ? rels[n] : 0;

				if(old)
					old->insert(res);
				else
					rels.insert(std::make_pair(n,res));
				
				std::cout << *res << std::endl;
			}
		}

		// eval all rec rules in parallel until fixpoint is reached
		std::cout << "recursive first:" << std::endl;
		bool modified;
		
		for(rule_ptr r: recursive)
		{
			assert(r);
			std::cout << *r << std::endl;

			std::vector<rel_ptr> plan;
			const std::string &n = r->head.name;

			for(const predicate &p: r->body)
				plan.push_back(rels[p.name]);

			rel_ptr res = eval_rule(r,plan);

			if(res)
			{
				rel_ptr d = deltas.count(n) ? deltas[n] : 0;

				if(d)
					d->insert(res);
				else
					deltas.insert(std::make_pair(n,res));
					
				std::cout << *res << std::endl;
			}
		}
		
		std::cout << "recursive delta:" << std::endl;
		do
		{
			modified = false;
			std::list<std::pair<std::string,rel_ptr>> new_deltas;

			for(const rule_ptr r: recursive)
			{
				assert(r);
				std::cout << *r << std::endl;

				std::vector<rel_ptr> plan(r->body.size(),rel_ptr(0));
				unsigned int sub = std::pow(2,r->body.size()) - 2;	// 1: current, 0: delta
				rel_ptr res;

				do
				{
					unsigned int pi = 0;

					while(pi < r->body.size())
					{
						const std::string &pn = std::next(r->body.begin(),pi)->name;

						if(sub & (1 << pi))
						{
							assert(rels.count(pn) && rels[pn]);
							plan[pi] = rels[pn];
						}
						else
						{
							if(!deltas.count(pn) || !deltas[pn])
								goto out;

							plan[pi] = deltas[pn];
						}

						++pi;
					}
					
					res = eval_rule(r,plan);
					new_deltas.push_back(std::make_pair(r->head.name,res));
					std::cout << *res << std::endl;

					out: ;
				}
				while(sub--);
			}

			// merge old deltas with 'rel'
			for(const std::pair<std::string,rel_ptr> &p: deltas)
			{
				if(rels.count(p.first))
					rels[p.first]->insert(p.second);
			}
			deltas.clear();

			// set new deltas, set 'modified'
			for(const std::pair<std::string,rel_ptr> &p: new_deltas)
			{
				if(rels.count(p.first))
				{
					rel_ptr cur = rels[p.first];
					for(const relation::row &r: p.second->rows())
						modified |= !cur->includes(r);
				}

				modified |= p.second->rows().size() > 0;
				
				if(deltas.count(p.first))
					deltas[p.first]->insert(p.second);
				else
					deltas.insert(p);
			}
			new_deltas.clear();
		}
		while(modified);
		
		idx = idx_end;
	}

	assert(rels.count(query));
	return rels[query];
}
