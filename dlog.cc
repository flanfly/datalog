#include <iostream>

#include "dlog.hh"
#include "dsl.hh"

relation::relation(std::initializer_list<std::string> ns)
: m_names(ns), m_types(ns.size(),typeid(void)) 
{
	return;
}

relation::relation(const std::vector<std::string> &ns)
: m_names(ns), m_types(ns.size(),typeid(void))
{
	return;
}

void relation::mutate(std::function<void(std::vector<std::string> &,std::vector<row> &)> f)
{
	bool set_types = m_rows.empty();

	f(m_names,m_rows);
	
	// no missing/superflus columns
	assert(std::all_of(m_rows.cbegin(),m_rows.cend(),[&](const row &r) { return r.size() == m_names.size(); }));
	m_types.resize(m_names.size(),typeid(void));

	// update m_types. set to the types of the columns in the first m_rows if there were empty before, set m_types to typeid(void) if m_rows is empty
	if(m_rows.size())
	{
		if(set_types)
		{
			size_t col = 0;
			while(col < m_names.size())
			{
				m_types[col] = m_rows[0][col].type();
				++col;
			}
		}
	}
	else
	{
		size_t col = 0;
		while(col < m_names.size())
			m_types[col++] = typeid(void);
	}

	// typecheck. can be removed in release
	assert(all_of(m_rows.cbegin(),m_rows.cend(),[&](const std::vector<variant> &r) -> bool
	{
		size_t col = 0;
		bool ret = true;
		
		while(col < m_names.size())
		{
			ret &= m_types[col] == r[col].type();
			++col;
		}
		return ret;
	}));
}

const std::vector<std::string> &relation::names(void) const
{ 
	return m_names; 
}

const std::vector<std::type_index> &relation::types(void) const
{ 
	return m_types;
}

const std::vector<relation::row> &relation::rows(void) const
{ 
return m_rows;
}

size_t relation::column(std::string n) const
{ 
	size_t ret = std::distance(names().begin(),std::find(names().begin(),names().end(),n));
	
	assert(ret < names().size());
	return ret;
}

variable::variable(bool b, relation::variant v, std::string n)
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

predicate::predicate(std::string n, relation r)
: name(n), intensional(false) 
{
	return;
}

predicate::predicate(std::string n, std::initializer_list<variable> &lst)
: name(n), intensional(true), variables(lst)
{
	return;
} 

predicate::predicate(std::string n, const std::vector<variable> &lst)
: name(n), intensional(true), variables(lst)
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

database::database(std::initializer_list<rule> &lst) : rules(lst) {build_graph();} 
database::database(const std::list<rule> &lst) : rules(lst) { build_graph(); }

void database::build_graph(void)
{	
	std::map<std::string,rg_node *> pred_map;
	std::map<rule *,rg_node *> rule_map;

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

	std::transform(rule_map.begin(),rule_map.end(),std::inserter(rg_graph,rg_graph.begin()),[&](std::pair<rule *,rg_node *> p) { return p.second; });
	std::transform(pred_map.begin(),pred_map.end(),std::inserter(rg_graph,rg_graph.begin()),[&](std::pair<std::string,rg_node *> p) { return p.second; });
	std::copy(rg_graph.begin(),rg_graph.end(),std::inserter(extensional,extensional.begin()));

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

	for(rg_node *a: rg_graph)
		for(rg_node *b: rg_graph)
			if(a->goal == b->goal && a != b && mutual_rec(a,b))
				std::cout << (a->goal ? *a->d.goal_node : *a->d.rule_node) << " and " << (b->goal ? *b->d.goal_node : *b->d.rule_node) << " are mutual recrusive" << std::endl;
	
	for(rg_node *p: rg_graph)
		for(rg_node *q: p->children)
			extensional.erase(q);

//	for(const rg_node *r: extensional)
//	{
		//assert(!r->goal);
//		std::cout << *r->d.rule_node << " is extensional" << std::endl;
//	}
}
	
std::ostream &operator<<(std::ostream &os, const database &db)
{
	for(const rule &r: db.rules)
		os << r << std::endl;
	return os;
}

bool union_compatible(const relation &a, const relation &b)
{
	if(a.types().size() != b.types().size())
		return false;

	unsigned int i = 0;
	while(i < a.types().size())
		if(a.types()[i] != typeid(void) && b.types()[i] != typeid(void) && a.types()[i] != b.types()[i])
			return false;
		else
			++i;

	return a.names() == b.names();
}

relation set_union(const relation &a, const relation &b)
{
	assert(union_compatible(a,b));
	
	relation ret(a.names());

	ret.mutate([&](std::vector<std::string> &cols, std::vector<std::vector<relation::variant>> &rows)
		{ std::set_union(a.rows().cbegin(),a.rows().cend(),b.rows().cbegin(),b.rows().cend(),std::inserter(rows,rows.begin())); });

	return ret;
}

relation set_difference(const relation &a, const relation &b)
{
	assert(union_compatible(a,b));
	
	relation ret(a.names());
	
	ret.mutate([&](std::vector<std::string> &cols, std::vector<std::vector<relation::variant>> &rows)
		{ std::set_difference(a.rows().cbegin(),a.rows().cend(),b.rows().cbegin(),b.rows().cend(),std::inserter(rows,rows.begin())); });

	return ret;
}

relation set_intersection(const relation &a, const relation &b)
{
	assert(union_compatible(a,b));
	
	relation ret(a.names());
	
	ret.mutate([&](std::vector<std::string> &cols, std::vector<std::vector<relation::variant>> &rows)
		{ std::set_intersection(a.rows().cbegin(),a.rows().cend(),b.rows().cbegin(),b.rows().cend(),std::inserter(rows,rows.begin())); });

	return ret;
}

relation projection(const std::set<size_t> &cols, const relation &a)
{
	relation ret;
	
	ret.mutate([&](std::vector<std::string> &names, std::vector<std::vector<relation::variant>> &rows)
	{
		for(size_t c: cols)
		{
			assert(c < a.names().size());
			names.push_back(a.names()[c]);
		}
	
		rows.reserve(a.rows().size());
		for(const std::vector<relation::variant> &v: a.rows())
		{
			rows.push_back(std::vector<relation::variant>());
			auto &r = rows.back();

			for(size_t c: cols)
				r.push_back(v.at(c));
		}
	});

	return ret;
}

relation projection(std::initializer_list<size_t> cols, const relation &a)
{
	std::set<size_t> s(cols);
	return projection(s,a);
}

relation selection(std::function<bool(const std::vector<relation::variant>&)> p, const relation &a)
{
	relation ret(a.names());

	ret.mutate([&](std::vector<std::string> &names, std::vector<std::vector<relation::variant>> &rows)
	{
		for(const std::vector<relation::variant> &r: a.rows())
			if(p(r))
				rows.push_back(r);
	});

	return ret;
}
	
relation rename(const relation &a, std::string from, std::string to)
{
	assert(std::find(a.names().begin(),a.names().end(),from) != a.names().end() && std::find(a.names().begin(),a.names().end(),to) == a.names().end());
	
	relation ret(a);
	
	ret.mutate([&](std::vector<std::string> &names, std::vector<std::vector<relation::variant>> &rows)
		{ *std::find(names.begin(),names.end(),from) = to; });

	return ret;
}

relation join(const relation &a, const relation &b, std::string attr)
{
	relation ret(a.names());

	ret.mutate([&](std::vector<std::string> &ns, std::vector<relation::row> &rows)
	{
		unsigned int a_col = a.column(attr);
		unsigned int b_col = b.column(attr);

		for(const std::string &a_ns: a.names())
			ns.push_back("a." + a_ns); 
		for(const std::string &b_ns: b.names())
			ns.push_back("b." + b_ns); 

		for(const relation::row &a_row: a.rows())
			for(const relation::row &b_row: b.rows())
				if(a_row[a_col] == b_row[b_col])
				{
					rows.push_back(relation::row());

					copy(a_row.begin(),a_row.end(),std::inserter(rows.back(),rows.back().end()));
					copy(b_row.begin(),b_row.end(),std::inserter(rows.back(),rows.back().end()));
				}
	});

	return ret;
}

std::ostream &operator<<(std::ostream &os, const relation &a)
{
	const size_t cols = a.names().size();
	size_t *widths = new size_t[cols];
	std::list<std::list<std::string>> text;
	size_t col = 0;

	while(col < cols)
	{
		widths[col] = a.names()[col].size();
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

	// header
	col = 0;
	while(col < cols)
	{
		os << "| " << std::setw(widths[col]) << a.names()[col] << " ";
		++col;
	}
	os << "|" << std::endl;
	
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
std::set<predicate *> derives(const predicate &q, const database &db)
{
	std::set<predicate *> ret;

	for(rg_node *n: db.rg_graph)
		if(!n->goal && n->d.rule_node->head.name == q.name)
			for(predicate &p: n->d.rule_node->body)
				ret.insert(&p);
	
	return ret;
}

relation step(const predicate &rule_a, const relation &rel_a, const predicate &rule_b, const relation &rel_b)
{
	std::function<std::list<std::function<bool(const relation::row&)>>(const predicate&)> selectors = [](const predicate &rule) -> std::list<std::function<bool(const relation::row&)>>
	{
		std::list<std::function<bool(const relation::row&)>> preds;
		unsigned int col = 0;

		while(col < rule.variables.size())
		{
			if(rule.variables[col].bound)
				preds.push_back([col,&rule](const relation::row &row)
					{ return row[col] == rule.variables[col].instantiation; });
			++col;
		}
		
		return preds;
	};

	auto preds_a = selectors(rule_a);
	auto preds_b = selectors(rule_b);

	// b -> a
	std::map<size_t,size_t> common;
	size_t idx_a = 0;

	while(idx_a < rule_a.variables.size())
	{
		const variable &v_a = rule_a.variables[idx_a];
		if(!v_a.bound)
		{
			size_t idx_b = 0;
			while(idx_b < rule_b.variables.size())
			{
				const variable &v_b = rule_b.variables[idx_b];
				if(!v_b.bound && v_a.name == v_b.name)
					common.insert(std::make_pair(idx_b,idx_a));
				++idx_b;
			}
		}
		++idx_a;
	}

	for(const std::pair<size_t,size_t> &p: common)
		std::cout << "common: " << p.first << " and " << p.second << std::endl;
	std::cout << std::endl;

	relation ret(rel_a.names());

	ret.mutate([&](std::vector<std::string> &ns, std::vector<relation::row> &rows) -> void
	{
		size_t ni_b = 0;
		while(ni_b < rel_b.names().size())
		{
			if(!common.count(ni_b))
				ns.push_back(rel_b.names()[ni_b]);
			++ni_b;
		}
		std::cout << ns.size() << " columns: ";
		for(const std::string &s: ns)
			std::cout << s << " ";
		std::cout << std::endl;
		for(const relation::row &row_a: rel_a.rows())
			for(const relation::row &row_b: rel_b.rows())
				if(std::all_of(common.begin(),common.end(),[&](const std::pair<size_t,size_t> &p) { return row_b[p.first] == row_a[p.second]; }))
				{
					size_t idx = 0;

					rows.push_back(relation::row());
					std::copy(row_a.begin(),row_a.end(),std::inserter(rows.back(),rows.back().begin()));
					
					while(idx < row_b.size())
						if(!common.count(idx))
							rows.back().push_back(row_b[idx++]);
						else
							++idx;
				}
	std::cout << ret << std::endl;
	});

	std::cout << ret << std::endl;
	return ret;
}

relation step(const rule &r, std::map<std::string,relation> &rels)
{	
	relation ret(rels[r.head.name].names());

	if(r.body.size() >= 2)
	{
		step(r.body.front(),rels[r.body.front().name],*std::next(r.body.begin()),rels[std::next(r.body.begin())->name]);
	}

	for(const predicate &p: r.body)
		ret = set_union(ret,rels[p.name]);
		
	return ret;
}

void eval(database &db, parse_i query)
{	
	std::map<std::string,relation> rels;

	// build relations for each predicate
	for(const rg_node *n: db.rg_graph)
		if(n->goal && !rels.count(n->d.goal_node->name))
		{
			const predicate &p = *n->d.goal_node;
			std::vector<std::string> ns(query.parent.columns);

			rels.insert(std::make_pair(p.name,relation(ns)));
		}
		else if(rels.count(n->d.goal_node->name))
			assert(rels[n->d.goal_node->name].names().size() == n->d.goal_node->variables.size());
	
	// fill extensional relations
	std::cout << "extensional" << std::endl;
	for(const rg_node *n: db.extensional)
	{
		std::cout << (n->goal ? *n->d.goal_node : *n->d.rule_node) << std::endl;

		if(!n->goal && n->d.rule_node->body.empty())
		{
			const predicate &p = n->d.rule_node->head;

			assert(all_of(p.variables.begin(),p.variables.end(),[&](const variable &v) { return v.bound; }));
			assert(rels.count(p.name));

			rels[p.name].mutate([&](std::vector<std::string> &names, std::vector<std::vector<relation::variant>> &rows)
			{
				std::vector<relation::variant> row;

				row.reserve(names.size());
				for(const variable &v: p.variables)
					row.push_back(v.instantiation);
				rows.push_back(row);
			});
		}
	}

	
	for(const std::pair<std::string,relation> &p: rels)
		std::cout << p.first << ":" << std::endl << p.second << std::endl;

	std::set<const rule *> worklist;

	for(rg_node *n: db.extensional)
	{
		if(!n->goal && n->children.size())
			n = *n->children.begin();

		for(rg_node *m: n->children)
		{
			assert(!m->goal);
			worklist.insert(m->d.rule_node);
		}
	}

	while(!worklist.empty())
	{
		bool modified = false;
		auto i = worklist.begin();
		const rule *r = *i;
		relation rel;
		
		worklist.erase(i);

		std::cout << "do " << *r << std::endl;
		rel = step(*r,rels);

		modified = rel.rows() != rels[r->head.name].rows();

		if(modified)
		{
			for(rg_node *n: db.rg_graph)
				if(n->goal && n->d.goal_node->name == r->head.name)
					for(rg_node *m: n->children)
					{
						assert(!m->goal);
						worklist.insert(m->d.rule_node);
					}
			rels[r->head.name] = rel;
		}
	}

	std::set<predicate *> ders = derives(query.parent.rules.back().head,db);
	std::cout << "targets" << std::endl;
	for(const predicate *p: ders)
		std::cout << *p << std::endl;

	std::cout << rels[query.parent.rules.back().head.name] << std::endl;
}


int main(int argc, char *argv[])
{
	/*relation sdm = make_relation("Name","isLoli"), moriya = make_relation("Name","isLoli");

	insert_row(sdm,std::string("Koa"),false);
	insert_row(sdm,std::string("Patchouli"),false);
	insert_row(sdm,std::string("Flandre"),true);
	insert_row(sdm,std::string("Remilia"),true);
	insert_row(sdm,std::string("Sakuya"),false);
	insert_row(sdm,std::string("Hong"),false);

	
	insert_row(moriya,std::string("Sanae"),false);
	insert_row(moriya,std::string("Suwako"),true);
	insert_row(moriya,std::string("Kanako"),false);
	
	std::cout << sdm << std::endl 
						<< moriya << std::endl
						<< "UNION" << std::endl << set_union(sdm,moriya) << std::endl 
						<< "DIFFERENCE" << std::endl << set_difference(sdm,moriya) << std::endl 
						<< "INTERSECTION" << std::endl << set_intersection(sdm,moriya) << std::endl
						<< "PROJECTION 0" << std::endl << projection({0},moriya) << std::endl
						<< "PROJECTION 1" << std::endl << projection({1},moriya) << std::endl
						<< "PROJECTION 0,1" << std::endl << projection({0,1},moriya) << std::endl
						<< "SELECTION if(isLoli)" << std::endl << selection([](const std::vector<relation::variant> &r) { return boost::get<bool>(r[1]); },sdm) << std::endl
						<< "RENAME isLoli/isCute" << std::endl << rename(moriya,"isLoli","isCute") << std::endl;*/
	
	/*rule parent1(predicate("parent",{bound<std::string>("john"),bound<std::string>("jim")})), 
			 parent2(predicate("parent",{bound<std::string>("john"),bound<std::string>("jack")})), 
			 parent3(predicate("parent",{bound<std::string>("jack"),bound<std::string>("jil")}));
	
	rule ancestor1(predicate("ancestor",{symbolic("X"),symbolic("Y")}),{predicate("parent",{symbolic("X"),symbolic("Y")})}),
			 ancestor2(predicate("ancestor",{symbolic("X"),symbolic("Y")}),{predicate("ancestor",{symbolic("X"),symbolic("Y")}),predicate("parent",{symbolic("Y"),symbolic("Z")})});*/
	
	parse parent("parent",{"par","child"}),ancestor("ancestor",{"anc","desc"}),query("query",{"anc","desc"});

	parent("john","jack");
	parent("john","jim");
	parent("jack","jil");

	ancestor("X","Y") >> parent("X","Y");
	ancestor("X","Z") >> parent("X","Y"),ancestor("Y","Z");

	query("X","Y") >> ancestor("X","Y");

	std::list<rule> all;

	std::copy(parent.rules.begin(),parent.rules.end(),std::inserter(all,all.begin()));
	std::copy(ancestor.rules.begin(),ancestor.rules.end(),std::inserter(all,all.end()));
	std::copy(query.rules.begin(),query.rules.end(),std::inserter(all,all.end()));
	database db(all);

	eval(db,query("X","Y"));

	/*parse p1("p1"),p2("p2"),p3("p3"),p4("p4"),p5("p5"),p6("p6"),p7("p7"),p8("p8"),p9("p9");

	p1("X") >> p3("Y"),p4("Y");
	p2("X") >> p4("Y"),p5("Y");
	p3("X") >> p6("Y"),p4("Y"),p3("Y");
	p4("X") >> p5("Y"),p3("Y");
	p3("X") >> p6("Y");
	p5("X") >> p5("Y"),p7("Y");
	p5("X") >> p6("Y");
	p7("X") >> p8("Y"),p9("Y");

	std::list<rule> all;

	std::copy(p1.rules.begin(),p1.rules.end(),std::inserter(all,all.begin()));
	std::copy(p2.rules.begin(),p2.rules.end(),std::inserter(all,all.begin()));
	std::copy(p3.rules.begin(),p3.rules.end(),std::inserter(all,all.begin()));
	std::copy(p4.rules.begin(),p4.rules.end(),std::inserter(all,all.begin()));
	std::copy(p5.rules.begin(),p5.rules.end(),std::inserter(all,all.begin()));
	std::copy(p6.rules.begin(),p6.rules.end(),std::inserter(all,all.begin()));
	std::copy(p7.rules.begin(),p7.rules.end(),std::inserter(all,all.begin()));
	std::copy(p8.rules.begin(),p8.rules.end(),std::inserter(all,all.begin()));
	std::copy(p9.rules.begin(),p9.rules.end(),std::inserter(all,all.begin()));
	database db(all);*/

	return 0;
}
