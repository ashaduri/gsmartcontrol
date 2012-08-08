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

#ifndef LIBDEBUG_DCHANNEL_H
#define LIBDEBUG_DCHANNEL_H

#include <string>
#include <ostream>  // std::ostream (iosfwd is not enough for win32 and suncc)

#include "hz/intrusive_ptr.h"
#include "hz/sync.h"

#include "dflags.h"


class DebugChannelBase;

/// Strong reference-holding pointer
typedef hz::intrusive_ptr<DebugChannelBase> debug_channel_base_ptr;

/// Strong reference-holding pointer
typedef hz::intrusive_ptr<const DebugChannelBase> debug_channel_base_const_ptr;



// TODO Per-channel flags (instead of per-debug-stream flags).


/// All channels must inherit this.
class DebugChannelBase : public hz::intrusive_ptr_referenced_locked<hz::SyncPolicyMtDefault> {
	public:

		/// Virtual destructor
		virtual ~DebugChannelBase()
		{ }

// 		virtual DebugChannelBase* clone() = 0;

		/// Clone the channel and return a strong reference-holding pointer
		virtual debug_channel_base_ptr clone_ptr() = 0;

		/// Clone the channel and return a strong reference-holding pointer
		virtual debug_channel_base_const_ptr clone_ptr() const = 0;

		/// Send a message to channel
		virtual void send(debug_level::flag level, const std::string& domain,
				debug_format::type& format_flags, int indent_level, bool is_first_line, const std::string& msg) = 0;
};




/// Helper function for DebugChannel objects, formats a message.
std::string debug_format_message(debug_level::flag level, const std::string& domain,
				debug_format::type& format_flags, int indent_level, bool is_first_line, const std::string& msg);



/// std::ostream wrapper as a DebugChannel.
/// Note: Use the _same_ channel instance for the same ostreams, only
/// this way you will get proper ostream locking. Other than for send(),
/// the locking must be performed manually by the caller.
/// The smartpointer's refcount mutex is reused for wrapped stream locking.
class DebugChannelOStream : public DebugChannelBase {
	public:

		/// Constructor
		DebugChannelOStream(std::ostream& os) : os_(os)
		{ }

		/// Virtual destructor
		virtual ~DebugChannelOStream()
		{ }

		// Reimplemented
		virtual debug_channel_base_ptr clone_ptr()
		{
			// this will prevent the object from being copied, which may harm the wrapped stream.
			return debug_channel_base_ptr(this);  // aka copy.
		}

		// Reimplemented
		virtual debug_channel_base_const_ptr clone_ptr() const
		{
			// this will prevent the object from being copied, which may harm the wrapped stream.
			return debug_channel_base_const_ptr(this);  // aka copy.
		}

		/// Reimplemented from DebugChannelBase. This function locks the
		/// wrapped stream, as long as there's only one instance of this class.
		virtual void send(debug_level::flag level, const std::string& domain,
				debug_format::type& format_flags, int indent_level, bool is_first_line, const std::string& msg)
		{
			intrusive_ptr_scoped_lock_type locker(get_ref_mutex());
			os_ << debug_format_message(level, domain, format_flags, indent_level, is_first_line, msg);
		}


		// Non-debug-API members:

		/// Get the ostream. Lock the channel for using the ostream.
		std::ostream& get_ostream()
		{
			return os_;
		}


	private:

		std::ostream& os_;  ///< Wrapped ostream
};






#endif

/// @}
