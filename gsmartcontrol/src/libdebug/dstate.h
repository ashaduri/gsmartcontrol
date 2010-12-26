/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef LIBDEBUG_DSTATE_H
#define LIBDEBUG_DSTATE_H

#include <string>
#include <map>
#include <stack>

#include "hz/intrusive_ptr.h"
#include "hz/tls.h"
#include "hz/exceptions.h"  // THROW_FATAL

#include "dflags.h"
#include "dstream.h"
#include "dexcept.h"

#include "dstate_pub.h"  // public interface part of this header



namespace debug_internal {


	// domain name "default" - template for all new domains.
	// domain name "all" - used for manipulating all domains.


	class DebugState {
		public:

			typedef hz::intrusive_ptr<DebugOutStream> out_stream_ptr;
			typedef std::map<debug_level::flag, out_stream_ptr> level_map_t;
			typedef std::map<std::string, level_map_t> domain_map_t;


			DebugState()
			{
				setup_default_state();
			}


			// This function is NOT thread-safe. Call it before using any
			// other functions in MT environment.
			// Automatically called by constructor, so usually no problem there.
			void setup_default_state();


			// This is function thread-safe in read-only context.
			domain_map_t& get_domain_map()
			{
				return domain_map;
			}


			// This is function thread-safe.
			int get_indent_level() const
			{
				if (!indent_level_.get())
					indent_level_.reset(new int(0));
				return *indent_level_;
			}

			// This is function thread-safe.
			void set_indent_level(int indent_level)
			{
				if (!indent_level_.get()) {
					indent_level_.reset(new int(indent_level));
				} else {
					*indent_level_ = indent_level;
				}
			}

			// This is function thread-safe.
			void push_inside_begin(bool value = true)
			{
				if (!inside_begin_.get())
					inside_begin_.reset(new std::stack<bool>());
				inside_begin_->push(value);
			}

			// This is function thread-safe.
			bool pop_inside_begin()
			{
				if (!inside_begin_.get())
					inside_begin_.reset(new std::stack<bool>());

				if (inside_begin_->empty())
					THROW_FATAL(debug_usage_error("DebugState::pop_inside_begin(): Begin / End stack underflow! Mismatched begin()/end()?"));
				bool val = inside_begin_->top();
				inside_begin_->pop();
				return val;
			}

			// This is function thread-safe.
			bool get_inside_begin() const
			{
				if (!inside_begin_.get())
					inside_begin_.reset(new std::stack<bool>());

				if (inside_begin_->empty())
					return false;
				return inside_begin_->top();
			}


			// Flush all the buffers. This will write prefixes too.
			// This is function thread-safe in read-only context.
			void force_output()
			{
				for(domain_map_t::iterator iter = domain_map.begin(); iter != domain_map.end(); ++iter) {
					for(level_map_t::iterator iter2 = iter->second.begin(); iter2 != iter->second.end(); ++iter2)
						iter2->second->force_output();
				}
			}


		private:

			// without mutable there's no on-demand allocation for these
			// It's thread-local because it is not shared between different flows.
			// we can't provide any manual cleanup, because the only one we can do
			// is in main thread, and it's already being done with the destructor.
			mutable hz::thread_local_ptr<int> indent_level_;
			mutable hz::thread_local_ptr<std::stack<bool> > inside_begin_;  // true if inside begin() / end() block

			domain_map_t domain_map;

	};



	DebugState& get_debug_state();



}  // ns







#endif
