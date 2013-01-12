#include <iostream>

#include "dlog.hh"
#include "dsl.hh"

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

bool relation::insert(const relation::row &r)
{
	assert(m_rows.empty() || r.size() == m_rows[0].size());
	std::vector<variable> b;
	std::set<unsigned int> *coll;

	for(const variant &v: r)
		b.push_back(variable(true,v,""));
	
	coll = find(b);

	if(!coll || coll->empty())
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
	else if(coll)
	{
		delete coll;
	}
	
	return false;
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

predicate::predicate(std::string n, std::initializer_list<variable> &lst)
: name(n), variables(lst)
{
	return;
} 

predicate::predicate(std::string n, const std::vector<variable> &lst)
: name(n), variables(lst)
{
	return;
}

bool operator==(const predicate &a, const predicate &b)
{
	return a.name == b.name && a.variables == b.variables;
}

std::ostream &operator<<(std::ostream &os, const predicate &p)
{
	os << p.name << "(";
	
	for(const variable &v: p.variables)
		if(v == *std::prev(p.variables.end(),1))
			os << v;
		else
			os << v << ",";
	
	os << ")";
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
	
	if(r.body.size())
	{
		os << " :- ";
		for(const predicate &p: r.body)
			if(p == *std::prev(r.body.end(),1))
				os << p;
			else
				os << p << ",";
	}

	return os;
}

rg_node::rg_node(rule *r) : goal(false) { d.rule_node = r; }
rg_node::rg_node(predicate *p) : goal(true) { d.goal_node = p; }

std::set<rg_node *> build_graph(std::list<rule> &rules)
{	
	std::map<std::string,rg_node *> pred_map;
	std::map<rule *,rg_node *> rule_map;
	std::set<rg_node *> ret;

	// build node table, one for each rule and each predicate
	for(rule &r: rules)
	{
		assert(!rule_map.count(&r));

		rule_map.insert(std::make_pair(&r,new rg_node(&r)));
		
		if(!pred_map.count(r.head.name))
			pred_map.insert(std::make_pair(r.head.name,new rg_node(&r.head)));
		for(predicate &p: r.body)
			if(!pred_map.count(p.name))
				pred_map.insert(std::make_pair(p.name,new rg_node(&p)));
	}

	// connect rg_node's
	for(rule &r: rules)
	{
		rg_node *rule_node = rule_map[&r];
		rg_node *head_node = pred_map[r.head.name];

		rule_node->children.insert(head_node);

		for(predicate &p: r.body)
		{
			rg_node *pred_node = pred_map[p.name];
			
			pred_node->children.insert(rule_node);
		}
	}
	
	std::cout << "digraph G" << std::endl 
						<< "{" << std::endl;

	for(const std::pair<rule *,rg_node *> &p: rule_map)
	{
		std::cout << "\tn" << (size_t)p.second << " [label=\"" << *p.first << "\" shape=ellipse];" << std::endl;
		for(rg_node *q: p.second->children)
			std::cout << "\tn" << (size_t)p.second << " -> n" << (size_t)q << std::endl;
	}
	
	for(const std::pair<std::string,rg_node *> &p: pred_map)
	{
		std::cout << "\tn" << (size_t)p.second << " [label=\"" << p.first << "\" shape=box];" << std::endl;
		for(rg_node *q: p.second->children)
			std::cout << "\tn" << (size_t)p.second << " -> n" << (size_t)q << std::endl;
	}
	
	std::cout << "}" << std::endl;

	std::transform(rule_map.begin(),rule_map.end(),std::inserter(ret,ret.begin()),[&](std::pair<rule *,rg_node *> p) { return p.second; });
	std::transform(pred_map.begin(),pred_map.end(),std::inserter(ret,ret.begin()),[&](std::pair<std::string,rg_node *> p) { return p.second; });

	std::function<bool(const rg_node *,const rg_node *)> mutual_rec = [&](const rg_node *from, const rg_node *to) -> bool
	{
		std::set<const rg_node *> known;
		std::function<bool(const rg_node *,const rg_node *)> dfs = [&](const rg_node *cur, const rg_node *goal) -> bool
		{
			if(cur == goal)
				return true;
			else
			{
				known.insert(cur);
				return !!cur->children.size() && any_of(cur->children.begin(),cur->children.end(),[&](const rg_node *n) { return !known.count(n) && dfs(n,goal); });
			}
		};
		
		if(dfs(from,to))
		{
			known.clear();
			return dfs(to,from);
		}
		else
			return false;
	};

	for(rg_node *a: ret)
		for(rg_node *b: ret)
			if(a->goal == b->goal && a != b && mutual_rec(a,b))
				std::cout << (a->goal ? *a->d.goal_node : *a->d.rule_node) << " and " << (b->goal ? *b->d.goal_node : *b->d.rule_node) << " are mutual recrusive" << std::endl;
	
	return ret;
}

std::ostream &operator<<(std::ostream &os, const rel_ptr &a)
{
	if(!a)
	{
		os << "NULL" << std::endl;
		return os;
	}
		
	if(a->rows().empty())
	{
		os << "empty" << std::endl;
		return os;
	}

	const size_t cols = a->rows().begin()->size();
	size_t *widths = new size_t[cols];
	std::list<std::list<std::string>> text;
	size_t col = 0;

	while(col < cols)
	{
		widths[col] = 0;
	 	++col;
	}

	auto row = a->rows().begin();
	while(row != a->rows().end())
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

variable find_helper(const ub &)
{
	return variable(false,"","X");
}

variable find_helper(variant v)
{
	return variable(true,v,"");
}

// p,q predicates
// p `derives' q
// p -> q
// p occurs in the body of a rule whose head is q
/*
std::set<predicate *> derives(const predicate &q, const database &db)
{
	std::set<predicate *> ret;

	for(rg_node *n: db.rg_graph)
		if(!n->goal && n->d.rule_node->head.name == q.name)
			for(predicate &p: n->d.rule_node->body)
				ret.insert(&p);
	
	return ret;
}*/

rel_ptr join(const std::vector<variable> &a_bind,const rel_ptr a_rel,const std::vector<variable> &b_bind,const rel_ptr b_rel)
{
	assert(a_rel && b_rel);
	std::set<unsigned int> *a_idx = a_rel->find(a_bind);
	assert(a_idx);
	std::multimap<unsigned int,unsigned int> cross_vars; // a -> b
	rel_ptr ret(new relation());

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

rel_ptr eval_rule(const rule &r, const std::map<std::string,rel_ptr> &relations)
{
	rel_ptr temp(new relation());
	std::vector<variable> binding;

	if(r.body.empty())
		return temp;


	if(r.body.size() > 1)
	{
		auto i = r.body.begin();

		while(i != r.body.end())
		{
			if(i == r.body.begin())
			{
				temp = join(i->variables,relations.at(i->name),std::next(i)->variables,relations.at(std::next(i)->name));
				binding = i->variables;

				++i;
			}
			else
			{
				temp = join(binding,temp,i->variables,relations.at(i->name));
			}

			std::copy(i->variables.begin(),i->variables.end(),std::inserter(binding,binding.end()));
			++i;
		}
	}
	else
	{
		const rel_ptr rel = relations.at(r.body.front().name);
		std::set<unsigned int> *s = rel->find(r.body.front().variables);

		if(s)
		{
			for(unsigned int i: *s)
				temp->insert(rel->rows()[i]);
			binding = r.body.front().variables;
			delete s;
		}
	}

	// project onto head predicate
	std::map<std::string,unsigned int> common; // varname -> temp rel column

	for(const variable &v: r.head.variables)
	{
		if(!v.bound)
		{
			auto i = binding.begin();
			while(i != binding.end())
			{
				const variable &w = *i;
				if(!w.bound && v.name == w.name)
					common.insert(std::make_pair(v.name,std::distance(binding.begin(),i)));

				++i;
			}
		}
	}

	rel_ptr ret(new relation());
	for(const relation::row &rr: temp->rows())
	{
		relation::row nr;

		for(const variable &v: r.head.variables)
			if(v.bound)
				nr.push_back(v.instantiation);
			else
				nr.push_back(rr[common[v.name]]);
		ret->insert(nr);
	}

	return ret;
}

	
/*
bool join(const predicate &lhs, const std::vector<const predicate*> &rhs, const std::vector<const relation::row*> &rows, std::unordered_set<relation::row> &out)
{
	assert(rows.size() == rhs.size());

	std::map<std::string,unsigned int> bindings;
	size_t pred = 0;

	while(pred < rhs.size())
	{
		const std::vector<variable> &vars = rhs[pred]->variables;
		const relation::row &row = *rows[pred];
		size_t col = 0;

		while(col < vars.size())
		{
			const variable &var = vars[col];
			
			if(var.bound)
			{
				if(!(var.instantiation == row[col]))
					return false;
			}
			else
			{
				auto i = bindings.find(var.name);

				if(i != bindings.end())
				{
					if(!(i->second == row[col]))
						return false;
				}
				else
				{
					bindings.insert(std::make_pair(var.name,row[col]));
				}
			}

			++col;
		}

		++pred;
	}
	
	relation::row o;

	for(const variable &v: lhs.variables)
	{
		if(v.bound)
			o.push_back(v.instantiation);
		else
		{
			auto i = bindings.find(v.name);

			assert(i != bindings.end());
			o.push_back(i->second);
		}
	}

	out.insert(o);

	return true;
}*/
/*
rel_ptr select(const predicate &head, const predicate &rhs,rel_ptr rel)
{
	assert(rel);
	rel_ptr ret(new relation());

	ret.mutate([&](std::unordered_set<relation::row> &rows)
	{
		for(const relation::row &row: rel->rows())
		{
			
			for(const variable &var: rhs.variables)
		

rel_ptr single(const rule &r, std::map<std::string,rel_ptr> &rels)
{	
	assert(r.body.size());

	if(r.body.size() == 1)
		return select(r.head,*r.body.begin(),rels[r.body.begin()->name]);
	else
	{
		auto i = r.body.begin();
		rel_ptr temp_rel(new relation());
		predicate temp_pred;

		while(std::next(i) != r.body.end())
		{
			join(*i,*std::next(i),temp_pred,temp_rel);
			++i;
		}

		return select(r.head,temp_pred,temp_rel);
	}
	
	ret.mutate([&](std::unordered_set<relation::row> &ret_rows) -> void
	{
		std::vector<const predicate *> rhs;
		std::vector<const relation::row *> rows;
		std::function<void(std::list<predicate>::const_iterator)> f = [&](std::list<predicate>::const_iterator i)
		{
			assert(rels.count(i->name));
			const rel_ptr &rel = rels[i->name];

			rhs.push_back(&*i);
			++i;
			
			for(const relation::row &t: rel.rows())
			{
				rows.push_back(&t);
					
				if(i != r.body.end())
					f(i);
				else
					step(r.head,rhs,rows,ret_rows);
				
				rows.pop_back();
			}

			rhs.pop_back();
		};

		f(r.body.begin());
	});

	return ret;
}*/

rel_ptr eval(parse_i query, std::list<rule> &intensional, std::map<std::string,rel_ptr> &extensional)
{	
	std::map<std::string,rel_ptr> rels(extensional);
	std::set<rg_node *> rg_graph = build_graph(intensional);

	// build relations for each predicate
	for(const rg_node *n: rg_graph)
		if(n->goal && !rels.count(n->d.goal_node->name))
		{
			const predicate &p = *n->d.goal_node;

			rels.insert(std::make_pair(p.name,rel_ptr(new relation())));
		}
	
	for(const std::pair<std::string,rel_ptr> &p: rels)
		std::cout << p.first << ":" << std::endl << p.second << std::endl;

	std::set<rg_node *> roots(rg_graph);
	std::set<const rule *> worklist;

	for(rg_node *n: rg_graph)
		if(!n->goal)
			roots.erase(n);
		else
			for(rg_node *m: n->children)
				roots.erase(m);
				
	for(rg_node *n: roots)
		for(rg_node *m: n->children)
		{
			assert(!m->goal);
			worklist.insert(m->d.rule_node);
		}

	while(!worklist.empty())
	{
		bool modified = false;
		auto i = worklist.begin();
		const rule *r = *i;
		rel_ptr rel;
		
		worklist.erase(i);

		std::cout << "do " << *r << std::endl;
		rel = eval_rule(*r,rels);
		std::cout << "result:" << std::endl << rel << std::endl;

		// TODO: make more efficient
		rel_ptr old = rels[r->head.name];
		for(const relation::row &r: rel->rows())
			modified |= old->insert(r);

		if(modified)
		{
			for(rg_node *n: rg_graph)
				if(n->goal && n->d.goal_node->name == r->head.name)
					for(rg_node *m: n->children)
					{
						assert(!m->goal);
						worklist.insert(m->d.rule_node);
					}
		}	
	}
	
	assert(rels.count(query.parent.name));
	return rels[query.parent.name];
}


int main(int argc, char *argv[])
{/*
	parse parent("parent"),ancestor("ancestor"),age("age"),query("query");
	rel_ptr parent_rel(new relation()),age_rel(new relation());

	insert_row(parent_rel,std::string("john"),std::string("jack"));
	insert_row(parent_rel,std::string("john"),std::string("jim"));
	insert_row(parent_rel,std::string("jack"),std::string("jil"));

	insert_row(age_rel,std::string("john"),(unsigned int)1);
	insert_row(age_rel,std::string("jack"),(unsigned int)2);
	insert_row(age_rel,std::string("jil"),(unsigned int)3);
	insert_row(age_rel,std::string("jim"),(unsigned int)4);

	ancestor("X","Y") >> parent("X","Y");
	ancestor("X","Z") >> parent("X","Y"),ancestor("Y","Z");

	query("X","A","Y","B") >> ancestor("X","Y"),age("X","A"),age("Y","B");

	std::list<rule> all;
	std::map<std::string,rel_ptr> db;

	std::copy(ancestor.rules.begin(),ancestor.rules.end(),std::inserter(all,all.end()));
	std::copy(query.rules.begin(),query.rules.end(),std::inserter(all,all.end()));
	db.insert(std::make_pair("parent",parent_rel));
	db.insert(std::make_pair("age",age_rel));

	std::cout << eval(query("X","A","Y","B"),all,db) << std::endl;*/

	rel_ptr parent_rel(new relation());
	//std::vector<variable> ab({variable(false,"","A"),variable(false,"","B")});
	insert(parent_rel,"john","jack");
	insert(parent_rel,"john","jim");
	insert(parent_rel,"jack","jil");
	
	parse parent("parent"),ancestor("ancestor"),age("age"),query("query");
	ancestor("X","Y") >> parent("X","Y");
	ancestor("X","Z") >> parent("X","Y"),ancestor("Y","Z");

	query("X","Y") >> ancestor("X","Y");

	std::list<rule> all;
	std::map<std::string,rel_ptr> db;

	std::copy(ancestor.rules.begin(),ancestor.rules.end(),std::inserter(all,all.end()));
	std::copy(query.rules.begin(),query.rules.end(),std::inserter(all,all.end()));
	db.insert(std::make_pair("parent",parent_rel));

	std::cout << eval(query("X","Y"),all,db) << std::endl;
}
