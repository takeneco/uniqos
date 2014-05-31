/// @file  log.hh
//
// (C) 2011-2013 KATO Takeshi
//

#ifndef INCLUDE_LOG_HH_
#define INCLUDE_LOG_HH_

#include <output_buffer.hh>


class log : public output_buffer
{
public:
	log(u32 target=0);
	~log();
};

void log_install(int target, io_node* node);


class obj_node;

class obj_edge
{
public:
	obj_edge(const char* _name) :
		name(_name),
		parent(0)
	{}
	obj_edge(const char* _name, const obj_node& _parent) :
		name(_name),
		parent(&_parent)
	{}

	void print(output_buffer& ob, const char* type) const;

private:
	const char* const     name;
	const obj_node* const parent;
};

class obj_node
{
public:
	obj_node(const char* _type, const obj_edge& _parent) :
		type(_type),
		parent(&_parent)
	{}

	void dump(output_buffer& ob) const;

private:
	const char* const     type;
	const obj_edge* const parent;
};


#endif  // include guard

