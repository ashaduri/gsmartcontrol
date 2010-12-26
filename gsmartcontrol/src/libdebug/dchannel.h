/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef LIBDEBUG_DCHANNEL_H
#define LIBDEBUG_DCHANNEL_H

#include <string>
#include <ostream>  // std::ostream (iosfwd is not enough for win32 and suncc)

#include "hz/intrusive_ptr.h"
#include "hz/sync.h"

#include "dflags.h"


class DebugChannelBase;

typedef hz::intrusive_ptr<DebugChannelBase> debug_channel_base_ptr;
typedef hz::intrusive_ptr<const DebugChannelBase> debug_channel_base_const_ptr;



// TODO: Per-channel flags (instead of per-debug-stream flags)!


// All channels must be inherited from here.
class DebugChannelBase : public hz::intrusive_ptr_referenced_locked<hz::SyncPolicyMtDefault> {
	public:
		virtual ~DebugChannelBase()
		{ }

// 		virtual DebugChannelBase* clone() = 0;

		virtual debug_channel_base_ptr clone_ptr() = 0;  // clone for smartpointer

		virtual debug_channel_base_const_ptr clone_ptr() const = 0;  // clone for smartpointer

		virtual void send(debug_level::flag level, const std::string& domain,
				debug_format::type& format_flags, int indent_level, bool is_first_line, const std::string& msg) = 0;
};




// a helper function for DebugChannel-s. formats a message.
std::string debug_format_message(debug_level::flag level, const std::string& domain,
				debug_format::type& format_flags, int indent_level, bool is_first_line, const std::string& msg);



// DebugChannel as a wrapper around std::ostream.
// Note: Use the _same_ channel instance for same ostreams. Only
// this way you will get proper ostream locking!
// The locking is performed manually by the caller. The smartpointer's
// refcount mutex is re-used for wrapped stream locking.
class DebugChannelOStream : public DebugChannelBase {
	public:

		DebugChannelOStream(std::ostream& os) : os_(os)
		{ }

		virtual ~DebugChannelOStream()
		{ }

// 		virtual DebugChannelBase* clone()
// 		{
// 			return new DebugChannelOStream(os_);
// 		}

		virtual debug_channel_base_ptr clone_ptr()
		{
			// this will prevent the object from being copied, which may harm the wrapped stream.
			return debug_channel_base_ptr(this);  // aka copy.
		}

		virtual debug_channel_base_const_ptr clone_ptr() const
		{
			// this will prevent the object from being copied, which may harm the wrapped stream.
			return debug_channel_base_const_ptr(this);  // aka copy.
		}

		// this function locks the wrapped stream, as long as there's only one instance of this class.
		virtual void send(debug_level::flag level, const std::string& domain,
				debug_format::type& format_flags, int indent_level, bool is_first_line, const std::string& msg)
		{
			intrusive_ptr_scoped_lock_type locker(get_ref_mutex());
			os_ << debug_format_message(level, domain, format_flags, indent_level, is_first_line, msg);
		}


		// Non-debug-API members:

		std::ostream& get_ostream()  // you should lock this channel before getting it!
		{
			return os_;
		}


	private:
		std::ostream& os_;  // wrapped stream. I think iosfwd is enough for reference.
};






#endif
