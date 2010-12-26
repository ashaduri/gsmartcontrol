/**************************************************************************
 Copyright:
      (C) 2003 - 2008  Irakli Elizbarashvili <ielizbar 'at' gmail.com>
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>

 License:

 This software is provided 'as-is', without any express or implied
 warranty.  In no event will the authors be held liable for any damages
 arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:

 1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.
 2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.
 3. This notice may not be removed or altered from any source distribution.
***************************************************************************/

#ifndef RMN_RESOURCE_NODE_H
#define RMN_RESOURCE_NODE_H

#include <string>
#include <list>  // for children list
#include <stack>  // std::stack

#include "hz/string_algo.h"  // string_split(), string_join()
#include "hz/debug.h"  // debug_*

#include "resource_base.h"
#include "resource_exception.h"



namespace rmn {


using hz::intrusive_ptr;  // used as rmn::intrusive_ptr by external users.



// these are in rmn:: namespace because it would be difficult to use
// them as members of a template class.
// these have internal linkage (const integral types).
const char PATH_DELIMITER = '/';
const char* const PATH_DELIMITER_S = "/";



template<class Data>
class resource_node : public resource_base, public Data {

	public:

		typedef resource_node* parent_node_ptr;  // must be able to assign this to node_ptr.
		// do NOT return parent_node_ptr from public functions, return node_ptr instead.
		typedef intrusive_ptr<resource_node<Data> > node_ptr;  // ref-counting smart pointer, child type.
		typedef intrusive_ptr<const resource_node<Data> > node_const_ptr;

		typedef std::list<node_ptr> child_list_t;
		typedef typename child_list_t::iterator child_iterator;
		typedef typename child_list_t::const_iterator child_const_iterator;
		typedef typename child_list_t::size_type child_size_type;

		typedef resource_node<Data> self_type;


		resource_node() : parent_(0)
		{ }

		~resource_node()
		{
#ifdef RMN_RESOURCE_NODE_DEBUG
			debug_out_dump("rmn", "Deleting node " << get_name() << "\n");
#endif
		}


		child_iterator children_begin();
		child_iterator children_end();
		child_const_iterator children_begin() const;
		child_const_iterator children_end() const;

		child_size_type get_child_count() const;


		node_ptr clone() const;  // return a deep copy of current node

		bool copy_data_from(const node_ptr& src);  // copy data from src to *this.
		bool copy_data_from(const node_const_ptr& src);  // copy data from src to *this.

		bool deep_copy_from(const node_ptr& src);  // recursively copy src to *this, replacing current node's children and name.
		bool deep_copy_from(const node_const_ptr& src);  // recursively copy src to *this, replacing current node's children and name.


		std::string get_path() const;  // get a full path from cache or generate it if not available.


		// returns false if cast failed if path not found
		template<typename T>
		inline bool get_data_by_path(const std::string& path, T& put_it_here) const;

		// This function throws if:
		// 	* no such node (rmn::no_such_node);
		//	* data empty (rmn::empty_data_retrieval);
		//	* type mismatch (rmn::type_mismatch).
		template<typename T>
		T get_data_by_path(const std::string& path) const;

		template<typename T>
		inline bool set_data_by_path(const std::string& path, T data);



		bool add_child(node_ptr p);

		node_ptr create_child(const std::string& name);

		template<typename T>
		node_ptr create_child(const std::string& name, T data);


		bool remove_node(const std::string& full_path);
		bool remove_child_node(const std::string& name);
		bool remove_child_node(node_ptr p);
		void clear_children();  // remove all children


		node_ptr get_child_node(child_size_type n);
		node_const_ptr get_child_node(child_size_type n) const;

		node_ptr get_child_node(const std::string& name);
		node_const_ptr get_child_node(const std::string& name) const;

		// build nodes up to and including path. If allow_side_construction is false,
		// allow constructing side-nodes (as opposed to subnodes only).
		bool build_nodes(const std::string& path, bool allow_side_construction = false);

		node_ptr find_node(const std::string& name);
		node_const_ptr find_node(const std::string& name) const;

		node_ptr get_root_node();
		node_const_ptr get_root_node() const;

		node_ptr get_parent();
		node_const_ptr get_parent() const;


		static bool is_abs_path(const std::string& path)
		{
			return (!path.empty() && path[0] == PATH_DELIMITER);
		}


	protected:

		bool set_parent(node_ptr p);
		bool clear_parent();

		std::string update_path_cache() const;  // update path cache. const because it doesn't change any visible members.
		void clear_path_cache() const;  // const because no public effect and callable from const methods.


		child_list_t	children_;
		parent_node_ptr parent_;

		mutable std::string path_cache_;  // path cache

};






template<class Data> inline
typename resource_node<Data>::child_iterator
resource_node<Data>::children_begin()
{
	return children_.begin();
}

template<class Data> inline
typename resource_node<Data>::child_iterator
resource_node<Data>::children_end()
{
	return children_.end();
}


template<class Data> inline
typename resource_node<Data>::child_const_iterator
resource_node<Data>::children_begin() const
{
	return children_.begin();
}

template<class Data> inline
typename resource_node<Data>::child_const_iterator
resource_node<Data>::children_end() const
{
	return children_.end();
}



template<class Data> inline
typename resource_node<Data>::child_size_type
resource_node<Data>::get_child_count() const
{
	return children_.size();
}





template<class Data> inline
typename resource_node<Data>::node_ptr
resource_node<Data>::clone() const
{
	node_ptr dest(new self_type());
	dest->deep_copy_from(this);
	return dest;
}



// Copy data from src.
template<class Data> inline
bool resource_node<Data>::copy_data_from(const typename resource_node<Data>::node_const_ptr& src)
{
	return Data::copy_data_from(src);
}


// Copy data from src.
template<class Data> inline
bool resource_node<Data>::copy_data_from(const typename resource_node<Data>::node_ptr& src)
{
	return copy_data_from(node_const_ptr(src));
}




template<class Data> inline
bool resource_node<Data>::deep_copy_from(const typename resource_node<Data>::node_const_ptr& src)
{
	if (!src)
		return false;

	copy_data_from(&src);
	set_name(src->get_name());

	children_.clear();
	for (child_const_iterator iter = src->children_.begin(); iter != src->children_.end(); ++iter) {
		node_ptr n(new self_type);
		n->deep_copy_from(*iter);
		add_child(n);
	}

	return true;
}


template<class Data> inline
bool resource_node<Data>::deep_copy_from(const typename resource_node<Data>::node_ptr& src)
{
	return deep_copy_from(node_const_ptr(src));
}




// get the current node's full path. retrieve it from cache if available.
template<class Data> inline
std::string resource_node<Data>::get_path() const
{
	if (path_cache_.empty())
		update_path_cache();  // this updates path_
	return path_cache_;
}




template<class Data> template<typename T> inline
bool resource_node<Data>::get_data_by_path(const std::string& path, T& put_it_here) const
{
	node_ptr p = find_node(path);
	if (!p)
		return false;
	return p->get_data(put_it_here);
}



// This function may throw. See prototype above.
template<class Data> template<typename T> inline
T resource_node<Data>::get_data_by_path(const std::string& path) const
{
	node_ptr p = find_node(path);
	if (!p)
		THROW_FATAL(no_such_node(path));
	return p->template get_data<T>();
}



template<class Data> template<typename T> inline
bool resource_node<Data>::set_data_by_path(const std::string& path, T data)
{
	node_ptr p = find_node(path);
	if (!p) {
		build_nodes(path);  // build if doesn't exist
		p = find_node(path);
	}
	p->set_data(data);
	return true;
}




// add a node to child list and set ourselves as child's parent.
template<class Data> inline
bool resource_node<Data>::resource_node::add_child(node_ptr p)
{
	if (!p)
		return false;

	if (p->get_parent()) {  // FIXME: maybe reparent?
		debug_out_warn("rmn", "resource_node::add_child(): this node has a parent already!\n");
		return false;
	}
#ifdef RMN_RESOURCE_NODE_DEBUG
// 	debug_out_dump("rmn", p->get_name() << " refcount is " << p->ref_count() << " (before insertion)\n");
#endif
	if (!get_child_node(p->get_name())) {
		children_.push_back(p);
		p->set_parent(this);
#ifdef RMN_RESOURCE_NODE_DEBUG
// 	debug_out_dump("rmn", p->get_name() << " refcount is " << p->ref_count() << " (after insertion)\n");
#endif

	} else {  // we already have a child by that name!
		return false;
	}

	p->clear_path_cache();  // clear its path cache

	return true;
}




template<class Data> inline
typename resource_node<Data>::node_ptr
resource_node<Data>::create_child(const std::string& name)
{
	node_ptr child(new self_type);
	child->set_name(name);
	node_ptr cur = this;
	if (!cur->add_child(child))
		return node_ptr(0);
	return child;
}




template<class Data> template<typename T> inline
typename resource_node<Data>::node_ptr
resource_node<Data>::create_child(const std::string& name, T data)
{
	node_ptr child = create_child(name);
	if (!child)
		return node_ptr(0);
	child->set_data(data);
	return child;
}




// get node by name from child list
template<class Data> inline
typename resource_node<Data>::node_ptr
resource_node<Data>::get_child_node(const std::string& name)
{
	return hz::ptr_const_cast<resource_node<Data> >(node_const_ptr(
			static_cast<const resource_node<Data>*>(this)->get_child_node(name)));
}




// get node by name from child list
template<class Data> inline
typename resource_node<Data>::node_const_ptr
resource_node<Data>::get_child_node(const std::string& name) const
{
	if (name.empty())
		return node_const_ptr(0);

	child_const_iterator iter = std::find_if(children_.begin(), children_.end(), compare_name(name));
	if (iter == children_.end())
		return node_const_ptr(0);

	return node_const_ptr(*iter);
}



// get node by index from child list
template<class Data> inline
typename resource_node<Data>::node_ptr
resource_node<Data>::get_child_node(typename resource_node<Data>::child_size_type n)
{
	return hz::ptr_const_cast<resource_node<Data> >(node_const_ptr(
			static_cast<const resource_node<Data>*>(this)->get_child_node(n)));
}



// get node by index from child list
template<class Data> inline
typename resource_node<Data>::node_const_ptr
resource_node<Data>::get_child_node(typename resource_node<Data>::child_size_type n) const
{
	if (n >= children_.size())
		return node_const_ptr(0);

	child_size_type i = 0;
	child_const_iterator iter = children_.begin();
	while (i != n) {
		++iter;
		++i;
	}

	return node_const_ptr(*iter);
}




// remove from child list and clear its parent.
template<class Data> inline
bool resource_node<Data>::remove_node(const std::string& full_path)
{
	if (full_path.empty())
		return false;

	node_ptr r(find_node(full_path));
	if (!r)  // not found
		return false;

	node_ptr parent = r->get_parent();
	if (!parent)  // huh?
		return false;

	return parent->remove_child_node(r);
}



template<class Data> inline
bool resource_node<Data>::remove_child_node(const std::string& name)
{
	if (name.empty())
		return false;

// 	unsigned int pos = path.rfind(PATH_DELIMITER);
// 	if (pos == std::string::npos)  // separator not found
// 		return false;
// 	std::string base(path.begin() + pos, path.end());  // base name from the path.

	child_iterator iter = std::find_if(children_.begin(), children_.end(), compare_name(name));
	if (iter != children_.end()) {
		(*iter)->clear_parent();
		children_.erase(iter);
	}

	return true;
}



template<class Data> inline
bool resource_node<Data>::remove_child_node(node_ptr p)
{
	if (!p)
		return false;
	return remove_child_node(p->get_name());
}



template<class Data> inline
void resource_node<Data>::clear_children()
{
	return children_.clear();
}




// create nodes from path
template<class Data> inline
bool resource_node<Data>::build_nodes(const std::string& path, bool allow_side_construction)
{
	if (path.empty())
		return false;

	if (path == PATH_DELIMITER_S) {  // root
		debug_out_warn("rmn", "resource_node::build_nodes(\"/\")!\n");
		return false;
	}

	std::string our_path = get_path();

	// Path to construct, absolute.
	std::string constr_path = ((path[0] == PATH_DELIMITER) ? path : (our_path + PATH_DELIMITER_S + path));
	std::list<std::string> components, components_canonical;

	// Split to components. This will skip empty results (double slashes, etc...)
	hz::string_split(constr_path, PATH_DELIMITER, components, true);

	// We don't compare paths directly - they may contain "..", double-slashes, etc...
	for (std::list<std::string>::const_iterator iter = components.begin(); iter != components.end(); ++iter) {
		if (*iter == ".")
			continue;  // nothing

		if (*iter == "..") {
			if (components_canonical.empty()) {
				debug_out_warn("rmn", "resource_node::build_nodes(\"" + path + "\"): Too many up-dirs.\n");
				return false;
			}
			components_canonical.pop_back();

		} else {
			components_canonical.push_back(*iter);
		}
	}

	std::string canonical_path = PATH_DELIMITER_S + hz::string_join(components, PATH_DELIMITER);

	// For absolute paths we accept only ones which are our subnodes.
	// This is a security measure - a code with access only to
	// a subnode shouldn't be able to write to other unrelated nodes!
	// Think "/config" and "/secure", where a code should be able to construct
	// by "/config/a/b" or "a/b", but not "/secure/a/b".

	// Now we can compare - both paths are canonical.
	bool is_subpath = (canonical_path.compare(0, our_path.size(), our_path) == 0);
	if (!allow_side_construction && !is_subpath) {
		// Requested path is not our subnode and side construction is disallowed.
		return false;
	}

	node_ptr cur(0);  // the base object which we begin construction from.

	// For our subpaths we can optimize away part of traversing.
	if (is_subpath) {
		std::list<std::string> our_components;
		hz::string_split(our_path, PATH_DELIMITER, our_components, true);

		// remove common ancestry
		for (std::list<std::string>::const_iterator iter = our_components.begin(); iter != our_components.end(); ++iter) {
			components_canonical.pop_front();
		}
		cur = this;  // start constructing from us

	} else {
		cur = get_root_node();
	}

	// Create them
	for (std::list<std::string>::const_iterator iter = components_canonical.begin(); iter != components_canonical.end(); ++iter) {
		node_ptr child = cur->get_child_node(*iter);  // get child node by name
		if (child) {  // if child with such name exists
			cur = child;  // switch to it

		} else {  // child doesn't exist, create it
			node_ptr new_child(new resource_node);
			new_child->set_name(*iter);
			cur->add_child(new_child);
			cur = new_child;  // or new_child.get() ?
		}
	}

	return true;
}



// accepts absolute and relative paths.
template<class Data> inline
typename resource_node<Data>::node_ptr
resource_node<Data>::find_node(const std::string& path)
{
	return hz::ptr_const_cast<resource_node<Data> >(node_const_ptr(
			static_cast<const resource_node<Data>*>(this)->find_node(path)));
}



// accepts absolute and relative paths.
template<class Data> inline
typename resource_node<Data>::node_const_ptr
resource_node<Data>::find_node(const std::string& path) const
{
	if (path.empty())
		return node_ptr(0);

	if (path[0] == PATH_DELIMITER) {  // absolute path
		std::string rel_path(path.begin() + 1, path.end());  // remove the first slash.
		if (rel_path.empty())  // "/" was requested
			return get_root_node();
		return get_root_node()->find_node(rel_path);
	}

	// ourselves
	if (get_name() == path) {
		return node_const_ptr(this);
	}


	node_const_ptr cur(this);

	std::list<std::string> components;
	hz::string_split(path, PATH_DELIMITER, components, true);  // this will skip empty results

	for (std::list<std::string>::const_iterator iter = components.begin(); iter != components.end(); ++iter) {

		if (*iter == ".")  // this has no effect
			continue;

		if (*iter == "..") {
			cur = cur->get_parent();

		} else {
			node_const_ptr child = cur->get_child_node(*iter);  // get child node by name
			cur = child;  // switch to it
		}

		if (!cur)
			return node_const_ptr(0);  // either child is 0, or parent not found.
	}

	return cur;
}




// return a root node of current tree
template<class Data> inline
typename resource_node<Data>::node_ptr
resource_node<Data>::get_root_node()
{
	return hz::ptr_const_cast<resource_node<Data> >(node_const_ptr(
			static_cast<const resource_node<Data>*>(this)->get_root_node()));
}



// return a root node of current tree
template<class Data> inline
typename resource_node<Data>::node_const_ptr
resource_node<Data>::get_root_node() const
{
	if (get_name() == PATH_DELIMITER_S)  // we are root
		return node_const_ptr(this);

	node_const_ptr cur = this;
	while (true) {
		node_const_ptr par = cur->get_parent();
		if (!par)
			break;
		cur = par;
	}

	return cur;
}




template<class Data> inline
typename resource_node<Data>::node_ptr
resource_node<Data>::get_parent()
{
	return parent_;  // in whatever format it's in.
}



template<class Data> inline
typename resource_node<Data>::node_const_ptr
resource_node<Data>::get_parent() const
{
	return parent_;  // in whatever format it's in.
}




template<class Data> inline
bool resource_node<Data>::set_parent(typename resource_node<Data>::node_ptr p)
{
	if (!p)
		return false;
	if (parent_)
		clear_parent();
	parent_ = p.get();
	return true;
}



template<class Data> inline
bool resource_node<Data>::clear_parent()
{
	if (!parent_) {
		debug_out_warn("rmn", "resource_node::clear_parent(): no parent exists.\n");
		return false;
	}

	parent_ = 0;

	return true;
}



// generate path for this node in hierarchy, cache it in path_cache_.
template<class Data> inline
std::string resource_node<Data>::update_path_cache() const
{
	std::stack<std::string> stk;

	node_const_ptr cur = this;
	while(cur) {  // until root is reached
		stk.push(cur->get_name());
		cur = cur->get_parent();
	}

	path_cache_.clear();
	while( ! stk.empty() ) {
		std::string name = stk.top();
		path_cache_ += name;
		if (name != PATH_DELIMITER_S && stk.size() > 1)
			path_cache_ += PATH_DELIMITER_S;
		stk.pop();
	}

	return path_cache_;
}



template<class Data> inline
void resource_node<Data>::clear_path_cache() const
{
	path_cache_.clear();
}




} // namespace




#endif
