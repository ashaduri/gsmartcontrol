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

#include <ostream>  // std::ostream definition

#include "dstream.h"
#include "dstate.h"
#include "dchannel.h"

// #include <iostream>  // tmp



namespace debug_internal {

	/// Null stream buf - discards anything that is sent to it.
	class NullStreamBuf : public std::streambuf {

		protected:
			int overflow(int) { return 0; }
			int sync() { return 0; }
	};



	/// Null streambuffer object
	static NullStreamBuf s_null_streambuf;

	/// Null ostream - discards anything that is sent to it.
	static std::ostream s_null_stream(&s_null_streambuf);


	/// Get null streambuf, see s_null_streambuf.
	std::streambuf& get_null_streambuf()
	{
		return s_null_streambuf;
	}

	/// Get null ostream, see s_null_stream.
	std::ostream& get_null_stream()
	{
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

// 		std::cerr << "SENDING: " << oss.str();
		std::vector<debug_channel_base_ptr>::iterator iter = dos_->channels_.begin();
		for (; iter != dos_->channels_.end(); ++iter) {
			// send() locks the channel if needed
			(*iter)->send(dos_->level_, dos_->domain_, flags,
					get_debug_state().get_indent_level(), is_first_line, oss_.str());
		}
		oss_.str("");  // clear the buffer
		oss_.clear();  // clear the flags
	}



}









/// @}
