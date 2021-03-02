/******************************************************************************
License: Zlib
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup libdebug
/// \weakgroup libdebug
/// @{

#include <ostream>  // std::ostream definition

#include "dstream.h"
#include "dstate.h"
#include "dchannel.h"

// #include <iostream>  // tmp



namespace debug_internal {

	/// Null stream buf - discards anything that is sent to it.
	class NullStreamBuf : public std::streambuf {

		protected:
			int overflow([[maybe_unused]] int unused) override { return 0; }
			int sync() override { return 0; }
	};



	/// Get null streambuf, see s_null_streambuf.
	std::streambuf& get_null_streambuf()
	{
		static NullStreamBuf s_null_streambuf;
		return s_null_streambuf;
	}

	/// Null ostream - discards anything that is sent to it.
	std::ostream& get_null_stream()
	{
		static std::ostream s_null_stream(&get_null_streambuf());
		return s_null_stream;
	}



	void DebugStreamBuf::flush_to_channel()
	{
		debug_format::type flags = dos_->format_;
		bool is_first_line = false;
		if (get_debug_state().get_inside_begin()) {
			flags |= debug_format::first_line_only;
			if (dos_->get_is_first_line()) {
				dos_->set_is_first_line(false);  // tls
				is_first_line = true;
			}
		} else {
			dos_->set_is_first_line(true);
			is_first_line = true;
		}

		for (auto & channel : dos_->channels_) {
			// send() locks the channel if needed
			channel->send(dos_->level_, dos_->domain_, flags,
					get_debug_state().get_indent_level(), is_first_line, oss_.str());
		}
		oss_.str("");  // clear the buffer
		oss_.clear();  // clear the flags
	}



}









/// @}
