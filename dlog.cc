#include <iostream>

#include "dlog.hh"
#include "dsl.hh"

void relation::mutate(std::function<void(std::unordered_set<row> &)> f)
{
	f(m_rows);
	
	if(m_rows.size())
	{
		std::vector<std::type_index> ts;
		
		for(const variant &v: *m_rows.begin())
			ts.push_back(v.type());

		// typecheck. can be removed in release
		assert(all_of(m_rows.cbegin(),m_rows.cend(),[&](const std::vector<variant> &r) -> bool
		{
			size_t col = 0;
			bool ret = true;
		
			while(col < r.size())
			{
				ret &= ts[col] == r[col].type();
				++col;
			}
			return ret;
		}));
	}
}

const std::unordered_set<relation::row> &relation::rows(void) const
{ 
	return m_rows;
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

bool step(const predicate &lhs, const std::vector<const predicate*> &rhs, const std::vector<const relation::row*> &rows, std::unordered_set<relation::row> &out)
{
	assert(rows.size() == rhs.size());

	std::map<std::string,variant> bindings;
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
}

relation single(const rule &r, std::map<std::string,relation> &rels)
{	
	relation ret(rels[r.head.name]);
	
	ret.mutate([&](std::unordered_set<relation::row> &ret_rows) -> void
	{
		std::vector<const predicate *> rhs;
		std::vector<const relation::row *> rows;
		std::function<void(std::list<predicate>::const_iterator)> f = [&](std::list<predicate>::const_iterator i)
		{
			assert(rels.count(i->name));
			const relation &rel = rels[i->name];

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
}

relation eval(parse_i query, std::list<rule> &intensional, std::map<std::string,relation> &extensional)
{	
	std::map<std::string,relation> rels(extensional);
	std::set<rg_node *> rg_graph = build_graph(intensional);

	// build relations for each predicate
	for(const rg_node *n: rg_graph)
		if(n->goal && !rels.count(n->d.goal_node->name))
		{
			const predicate &p = *n->d.goal_node;

			rels.insert(std::make_pair(p.name,relation()));
		}
	
	for(const std::pair<std::string,relation> &p: rels)
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
		relation rel;
		
		worklist.erase(i);

		std::cout << "do " << *r << std::endl;
		rel = single(*r,rels);

		modified = rel.rows() != rels[r->head.name].rows();

		if(modified)
		{
			for(rg_node *n: rg_graph)
				if(n->goal && n->d.goal_node->name == r->head.name)
					for(rg_node *m: n->children)
					{
						assert(!m->goal);
						worklist.insert(m->d.rule_node);
					}
			rels[r->head.name] = rel;
		}	
	}
	
	assert(rels.count(query.parent.name));
	return rels[query.parent.name];
}


int main(int argc, char *argv[])
{
	parse parent("parent"),ancestor("ancestor"),query("query");
	relation parent_rel;

	insert_row(parent_rel,std::string("john"),std::string("jack"));
	insert_row(parent_rel,std::string("john"),std::string("jim"));
	insert_row(parent_rel,std::string("jack"),std::string("jil"));

	ancestor("X","Y") >> parent("X","Y");
	ancestor("X","Z") >> parent("X","Y"),ancestor("Y","Z");

	query("X","Y") >> ancestor("X","Y");

	std::list<rule> all;
	std::map<std::string,relation> db;

	std::copy(parent.rules.begin(),parent.rules.end(),std::inserter(all,all.begin()));
	std::copy(ancestor.rules.begin(),ancestor.rules.end(),std::inserter(all,all.end()));
	std::copy(query.rules.begin(),query.rules.end(),std::inserter(all,all.end()));
	db.insert(std::make_pair("parent",parent_rel));

	std::cout << eval(query("X","Y"),all,db) << std::endl;

	return 0;
}
