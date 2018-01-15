/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
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
#include <memory>

#include "dflags.h"


class DebugChannelBase;

/// Strong reference-holding pointer
using DebugChannelBasePtr = std::shared_ptr<DebugChannelBase>;



// TODO Per-channel flags (instead of per-debug-stream flags).


/// All channels must inherit this.
class DebugChannelBase {
	public:

		/// Virtual destructor
		virtual ~DebugChannelBase() = default;

		/// Send a message to channel
		virtual void send(debug_level::flag level, const std::string& domain,
				debug_format::type& format_flags, int indent_level, bool is_first_line, const std::string& msg) = 0;
};




/// Helper function for DebugChannel objects, formats a message.
std::string debug_format_message(debug_level::flag level, const std::string& domain,
				debug_format::type& format_flags, int indent_level, bool is_first_line, const std::string& msg);



/// std::ostream wrapper as a DebugChannel.
class DebugChannelOStream : public DebugChannelBase {
	public:

		/// Constructor
		explicit DebugChannelOStream(std::ostream& os) : os_(os)
		{ }

		/// Reimplemented from DebugChannelBase.
		void send(debug_level::flag level, const std::string& domain,
				debug_format::type& format_flags, int indent_level, bool is_first_line, const std::string& msg) override
		{
			os_ << debug_format_message(level, domain, format_flags, indent_level, is_first_line, msg);
		}


		// Non-debug-API members:

		/// Get the ostream.
		std::ostream& get_ostream()
		{
			return os_;
		}


	private:

		std::ostream& os_;  ///< Wrapped ostream
};






#endif

/// @}
