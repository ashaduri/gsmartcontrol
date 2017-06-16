/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup libdebug
/// \weakgroup libdebug
/// @{

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


	/// Libdebug global state
	class DebugState {
		public:

			/// Debug output stream strong reference-holding pointer
			typedef hz::intrusive_ptr<DebugOutStream> out_stream_ptr;

			/// A mapping of debug levels to respective streams
			typedef std::map<debug_level::flag, out_stream_ptr> level_map_t;

			/// A mapping of domains to debug level maps with streams
			typedef std::map<std::string, level_map_t> domain_map_t;


			/// Constructor (statically called), calls setup_default_state().
			DebugState()
			{
				setup_default_state();
			}


			/// Initialize the "default" template domain, set the default enabled levels / format flags.
			/// This function is NOT thread-safe. Call it before using any
			/// other functions in MT environment. Automatically called by constructor.
			void setup_default_state();


			/// Get the domain/level mapping.
			/// This is function thread-safe in read-only context.
			domain_map_t& get_domain_map()
			{
				return domain_map;
			}


			/// Get current indentation level. This is function thread-safe.
			int get_indent_level() const
			{
				if (!indent_level_.get())
					indent_level_.reset(new int(0));
				return *indent_level_;
			}

			/// Set current indentation level. This is function thread-safe.
			void set_indent_level(int indent_level)
			{
				if (!indent_level_.get()) {
					indent_level_.reset(new int(indent_level));
				} else {
					*indent_level_ = indent_level;
				}
			}

			/// Open a debug_begin() context. This is function thread-safe.
			void push_inside_begin(bool value = true)
			{
				if (!inside_begin_.get())
					inside_begin_.reset(new std::stack<bool>());
				inside_begin_->push(value);
			}

			/// Close a debug_begin() context. This is function thread-safe.
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

			/// Check if we're inside a debug_begin() context. This is function thread-safe.
			bool get_inside_begin() const
			{
				if (!inside_begin_.get())
					inside_begin_.reset(new std::stack<bool>());

				if (inside_begin_->empty())
					return false;
				return inside_begin_->top();
			}


			/// Flush all the stream buffers. This will write prefixes too.
			/// This is function thread-safe in read-only context.
			void force_output()
			{
				for(domain_map_t::iterator iter = domain_map.begin(); iter != domain_map.end(); ++iter) {
					for(level_map_t::iterator iter2 = iter->second.begin(); iter2 != iter->second.end(); ++iter2)
						iter2->second->force_output();
				}
			}


		private:

			// Without mutable there's no on-demand allocation for these.
			// It's thread-local because it is not shared between different flows.
			// We can't provide any manual cleanup, because the only one we can do
			// is in main thread, and it's already being done with the destructor.

			mutable hz::thread_local_ptr<int> indent_level_;  ///< Current indentation level
			mutable hz::thread_local_ptr<std::stack<bool> > inside_begin_;  ///< True if inside debug_begin() / debug_end() block

			domain_map_t domain_map;  ///< Domain / debug level mapping.

	};



	/// Get global libdebug state
	DebugState& get_debug_state();



}  // ns







#endif

/// @}
