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

#ifndef LIBDEBUG_DSTREAM_H
#define LIBDEBUG_DSTREAM_H

#include <ostream>  // std::ostream definition
#include <streambuf>  // std::streambuf definition
#include <string>
#include <sstream>
#include <vector>

#include "hz/intrusive_ptr.h"

#include "dflags.h"
#include "dchannel.h"



namespace debug_internal {


	/// Get null streambuf - a streambuf which does nothing.
	std::streambuf& get_null_streambuf();

	/// Get null ostream - an ostream which does nothing.
	std::ostream& get_null_stream();


	// state.h includes us, so we need these forward declarations
// 	class DebugState;
// 	DebugState& get_debug_state();



	class DebugOutStream;


	/// Streambuf for libdebug, used in DebugOutStream implementation.
	class DebugStreamBuf : public std::streambuf {
		public:

			/// Constructor
			DebugStreamBuf(DebugOutStream* dos) : dos_(dos)
			{
				// in case of overflow for output, overflow() will be called to _output_ the data.

				// no buffers here - we process it char-by-char.
				int buf_size = 0;
				if (buf_size) {
					char* buf = new char[buf_size];  // further accessible through pbase().
					setp(buf, buf + buf_size);  // Set output sequence pointers, aka output buffer
				} else {
					setp(nullptr, nullptr);
				}

				setg(nullptr, nullptr, nullptr);  // Set input sequence pointers; not relevant in this class.
			}


			/// Disallow copying
			DebugStreamBuf(const DebugStreamBuf& from) = delete;


			/// Virtual destructor
			virtual ~DebugStreamBuf()
			{
				sync();
				delete[] pbase();  // delete the buffer
			}


			/// Force output of the stringstream's contents to the channels.
			void force_output()
			{
				flush_to_channel();
			}


		protected:

			/// Overflow happens when a new character is to be written at the put
			/// pointer pptr position, but this has reached the end pointer epptr.
			/// Reimplemented.
			virtual int overflow(int c)
			{
				sync();  // write the buffer contents if available
				if (c != traits_type::eof()) {
					if (pbase() == epptr()) {  // no buffer, write it char-by-char (epptr() - buffer end pointer)
	// 					std::string tmp;
	// 					tmp += char(c);
	// 					write_out(tmp);
						write_char(char(c));
					} else {  // we have a buffer
						// put c into buffer (the overflowed char); the rest is written in sync() earlier.
						sputc(static_cast<char>(c));
					}
				}
				return 0;
			}


			/// Sort-of flush the buffer. Only makes sense if there is a buffer.
			/// Reimplemented.
			virtual int sync()
			{
				if (pbase() != pptr()) {  // pptr() - current position; condition is true only if there is something in the buffer.
	// 				write_out(std::string(pbase(), pptr() - pbase()));
					for (char* pos = pbase(); pos != pptr(); ++pos)
						write_char(*pos);
					setp(pbase(), epptr());  // reset the buffer's current pointer (?)
				}
				return 0;
			}


			/// Write contents if necessary.
			void write_char(char c)
			{
				oss_ << c;
				if (c == '\n')  // send to channels on newline
					flush_to_channel();
			}


			/// Flush contents to debug channel.
			void flush_to_channel();


		private:

			DebugOutStream* dos_ = nullptr;  ///< Debug output stream

			std::ostringstream oss_;  ///< A buffer for output storage.

	};




	/// Debug output stream (inherits std::ostream).
	/// This is returned by debug_out().
	class DebugOutStream : public std::ostream, public hz::intrusive_ptr_referenced {
		public:

			friend class DebugStreamBuf;

			/// Constructor
			DebugOutStream(debug_level::flag level, const std::string& domain, const debug_format::type& format_flags)
					: std::ostream(nullptr), level_(level), domain_(domain), format_(format_flags), buf_(this)
			{
				set_enabled(true);  // sets ostream's rdbuf
			}

// 			DebugOutStream() : std::ostream(nullptr), buf_(this)
// 			{
// 				set_enabled(false);
// 			}

			/// Construct with settings from another DebugOutStream.
			DebugOutStream(const DebugOutStream& other, const std::string& domain)
					: std::ostream(nullptr), level_(other.level_), domain_(domain), format_(other.format_), buf_(this)
			{
				set_enabled(other.get_enabled());  // sets ostream's rdbuf
				for (std::vector<debug_channel_base_ptr>::const_iterator iter = other.channels_.begin(); iter != other.channels_.end(); ++iter) {
					// we let the object dictate the copy rules because really copying it
					// may harm the underlying locking mechanism
					channels_.push_back((*iter)->clone_ptr());
				}
			}

/*
			void set_level(debug_level::flag level)
			{
				level_ = level;
			}

			void set_domain(const std::string& domain)
			{
				domain_ = domain;
			}
*/

			/// Set format flags
			void set_format(const debug_format::type& format_flags)
			{
				format_ = format_flags;
			}

			/// Get format flags
			debug_format::type get_format() const
			{
				return format_;
			}


			/// Enable or disable output. If disabled, any data sent to this
			/// stream is discarded.
			void set_enabled(bool enabled)
			{
				if (enabled)
					rdbuf(&buf_);
				else
					rdbuf(&get_null_streambuf());
			}

			/// Check whether the stream is enabled or not.
			bool get_enabled() const
			{
				return (rdbuf() == &buf_);
			}


			/// Set channel list to send the data to.
			void set_channels(const std::vector<debug_channel_base_ptr>& channels)
			{
				channels_ = channels;
			}

			/// Get channel list
			std::vector<debug_channel_base_ptr>& get_channels()
			{
				return channels_;
			}

			/// Add a channel to channel list.
			/// This will claim the ownership of the passed parameter.
			void add_channel(debug_channel_base_ptr channel)
			{
				channels_.push_back(channel);
			}


			/// Check if the last sent output is still on the same line
			/// as the first one.
			bool get_is_first_line()
			{
				return is_first_line_;
			}

			/// Set whether we're on the first line of the output or not.
			void set_is_first_line(bool b)
			{
				is_first_line_ = b;
			}


			/// Force output of buf_'s contents to the channels.
			/// This also outputs a prefix if needed.
			std::ostream& force_output()
			{
				buf_.force_output();
				return *this;
			}


		private:

			debug_level::flag level_;  ///< Debug level of this stream
			std::string domain_;  ///< Domain of this stream
			debug_format::type format_;  ///< Format flags

			bool is_first_line_ = true;  ///< Whether it's the first line of output or not

			std::vector<debug_channel_base_ptr> channels_;  ///< Channels that the output is sent to

			DebugStreamBuf buf_;  /// Streambuf for implementation.
	};





}  // ns debug_internal







#endif

/// @}
