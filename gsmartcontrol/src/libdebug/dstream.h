/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef LIBDEBUG_DSTREAM_H
#define LIBDEBUG_DSTREAM_H

#include <ostream>  // std::ostream definition
#include <streambuf>  // std::streambuf definition
#include <string>
#include <sstream>
#include <vector>

#include "hz/intrusive_ptr.h"
#include "hz/tls.h"

#include "dflags.h"
#include "dchannel.h"



namespace debug_internal {


	std::streambuf& get_null_streambuf();

	std::ostream& get_null_stream();


	// state.h includes us, so we need these forward declarations
// 	class DebugState;
// 	DebugState& get_debug_state();



	class DebugOutStream;


	class DebugStreamBuf : public std::streambuf {
		public:

			DebugStreamBuf(DebugOutStream* dos) : dos_(dos)
			{
				// in case of overflow for output, overflow() will be called to _output_ the data.

				// no buffers here - we process it char-by-char. Apart from practical reasons,
				// this is also needed to ensure that the state of the object (buffer) doesn't
				// change during calls to methods - to ensure read-only thread-safety.
				int buf_size = 0;
				if (buf_size) {
					char* buf = new char[buf_size];  // further accessible through pbase().
					setp(buf, buf + buf_size);  // Set output sequence pointers, aka output buffer
				} else {
					setp(NULL, NULL);
				}

				setg(NULL, NULL, NULL);  // Set input sequence pointers; not relevant in this class.
			}


			virtual ~DebugStreamBuf()
			{
				sync();
				delete[] pbase();  // delete the buffer
			}


			// Force output of the stringstream's contents to the channels.
			// This is function thread-safe as long as state is not modified.
			void force_output()
			{
				flush_to_channel();
			}


		protected:

			// overflow happens when a new character is to be written at the put
			// pointer pptr position, but this has reached the end pointer epptr.
			virtual int overflow(int c)  // overridden from parent
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


			// sort-of flush the buffer. only makes sense if there is a buffer.
			virtual int sync()  // overridden from parent
			{
				if (pbase() != pptr()) {  // pptr() - current position; condition is true only if there is something in the buffer.
	// 				write_out(std::string(pbase(), pptr() - pbase()));
					for (char* pos = pbase(); pos != pptr(); ++pos)
						write_char(*pos);
					setp(pbase(), epptr());  // reset the buffer's current pointer (?)
				}
				return 0;
			}


			// custom function. write contents if necessary.
	// 		void write_out(const std::string& str)  // str may be one char, or entire buffer, or in between.
	// 		{
	// 		}


			// custom function. write contents if necessary.
			void write_char(char c)
			{
				if (!oss_.get())
					oss_.reset(new std::ostringstream());

				*oss_ << c;
				if (c == '\n')  // send to channels on newline
					flush_to_channel();
			}


			// This is function thread-safe as long as state is not modified.
			void flush_to_channel();


		private:
			DebugOutStream* dos_;

			// It's thread-local because it is not shared between different flows.
			// we can't provide any manual cleanup, because the only one we can do it
			// is in main thread, and it's already being done with the destructor.
			hz::thread_local_ptr<std::ostringstream> oss_;

			DebugStreamBuf(const DebugStreamBuf& from);
	};




	typedef std::vector<debug_channel_base_ptr> channel_list_t;



	// This is returned by debug_out()
	class DebugOutStream : public std::ostream, public hz::intrusive_ptr_referenced {
		public:

			friend class DebugStreamBuf;

			DebugOutStream(debug_level::flag level, const std::string& domain, const debug_format::type& format_flags)
					: std::ostream(NULL), level_(level), domain_(domain), format_(format_flags), buf_(this)
			{
				set_enabled(true);  // sets ostream's rdbuf
			}

// 			DebugOutStream() : std::ostream(NULL), buf_(this)
// 			{
// 				set_enabled(false);
// 			}

			DebugOutStream(const DebugOutStream& other, const std::string& domain)
					: std::ostream(NULL), level_(other.level_), domain_(domain), format_(other.format_), buf_(this)
			{
				set_enabled(other.get_enabled());  // sets ostream's rdbuf
				for (channel_list_t::const_iterator iter = other.channels_.begin(); iter != other.channels_.end(); ++iter) {
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

			void set_format(const debug_format::type& format_flags)
			{
				format_ = format_flags;
			}

			debug_format::type get_format() const
			{
				return format_;
			}


			void set_enabled(bool enabled)
			{
				if (enabled)
					rdbuf(&buf_);
				else
					rdbuf(&get_null_streambuf());
			}

			bool get_enabled() const
			{
				return (rdbuf() == &buf_);
			}


			void set_channels(const channel_list_t& channels)
			{
				channels_ = channels;
			}

			channel_list_t& get_channels()
			{
				return channels_;
			}

			// this will claim the ownership of the passed parameter
			void add_channel(debug_channel_base_ptr channel)
			{
				channels_.push_back(channel);
			}


			bool get_is_first_line()
			{
				if (!is_first_line_.get())
					is_first_line_.reset(new bool(true));
				return *is_first_line_;
			}

			void set_is_first_line(bool b)
			{
				if (!is_first_line_.get()) {
					is_first_line_.reset(new bool(b));
				} else {
					*is_first_line_ = b;
				}
			}


			// Force output of buf_'s contents to the channels.
			// This also outputs a prefix if needed.
			// This is function thread-safe in read-only context.
			std::ostream& force_output()
			{
				buf_.force_output();
				return *this;
			}


		private:
			debug_level::flag level_;
			std::string domain_;
			debug_format::type format_;

			// It's thread-local because it is not shared between different flows.
			// we can't provide any manual cleanup, because the only one we can do
			// is in main thread, and it's already being done with the destructor.
			hz::thread_local_ptr<bool> is_first_line_;

			channel_list_t channels_;

			DebugStreamBuf buf_;  // not thread-local, but its buffer is.
	};





}  // ns debug_internal







#endif
