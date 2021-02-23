/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_ERROR_H
#define HZ_ERROR_H

#include <string>
#include <exception>  // for std::exception specialization
#include <typeinfo>  // std::type_info
#include <system_error>

#include "debug.h"  // DBG_ASSERT
#include "process_signal.h"  // hz::signal_to_string
#include "bad_cast_exception.h"



namespace hz {



/**
\file
Predefined error types are: "errno", "signal" (child exited with signal).
*/


/// Error level (severity)
enum class ErrorLevel {
	none = 0,  ///< No error
	dump = 1 << 0,  ///< Dump
	info = 1 << 1,  ///< Informational (default)
	warn = 1 << 2,  ///< Warning
	error = 1 << 3,  ///< Error
	fatal = 1 << 4  ///< Fatal
};



template<typename CodeType>
class Error;



/// Base class for Error<T>
class ErrorBase {
	public:

		// This is thrown in case of type conversion error
		class type_mismatch : public hz::bad_cast_except {
			public:
				type_mismatch(const std::type_info& src, const std::type_info& dest)
					: hz::bad_cast_except(src, dest, "type_mismatch",
						"Error: type mismatch. Original type: \"%s\", requested type: \"%s\".")
				{ }
		};


		/// Constructor
		ErrorBase(const std::string& type_, ErrorLevel level_, const std::string& msg)
				: type(type_), level(level_), message(msg)
		{ }

		/// Constructor
		ErrorBase(const std::string& type_, ErrorLevel level_)
				: type(type_), level(level_)
		{ }

		/// Virtual destructor
		virtual ~ErrorBase() = default;

		/// Clone this object
		virtual ErrorBase* clone() = 0;  // needed for copying by base pointers


		/// Get std::type_info for the error code type.
		virtual const std::type_info& get_code_type() const = 0;

		
		/// Get error code of type \c CodeMemberType
		template<class CodeMemberType>
		CodeMemberType get_code() const  // this may throw on bad cast!
		{
			if (get_code_type() != typeid(CodeMemberType))
				throw type_mismatch(get_code_type(), typeid(CodeMemberType));
			return static_cast<const Error<CodeMemberType>*>(this)->code;
		}

		/// Get error code of type \c CodeMemberType
		template<class CodeMemberType>
		bool get_code(CodeMemberType& put_it_here) const  // this doesn't throw
		{
			if (get_code_type() != typeid(CodeMemberType))
				return false;
			put_it_here = static_cast<const Error<CodeMemberType>*>(this)->code;
			return true;
		}


		/// Increase the level (severity) of the error
		ErrorLevel level_inc()
		{
			if (level == ErrorLevel::fatal)
				return level;
			return (level = static_cast<ErrorLevel>(static_cast<int>(level) << 1));
		}

		/// Decrease the level (severity) of the error
		ErrorLevel level_dec()
		{
			if (level == ErrorLevel::none)
				return level;
			return (level = static_cast<ErrorLevel>(static_cast<int>(level) >> 1));
		}

		/// Get error level (severity)
		ErrorLevel get_level() const
		{
			return level;
		}


		/// Get error type
		std::string get_type() const
		{
			return type;
		}

		/// Get error message
		std::string get_message() const
		{
			return message;
		}


		// no set_type, set_message - we don't allow changing those.


	protected:

		std::string type;  ///< Error type
		ErrorLevel level = ErrorLevel::none;  ///< Error severity
		std::string message;  ///< Error message

};





// Provides some common stuff for Error to ease template specializations.
template<typename CodeType>
class ErrorCodeHolder : public ErrorBase {
	protected:

		/// Constructor
		ErrorCodeHolder(const std::string& type_, ErrorLevel level_, const CodeType& code_,
				const std::string& msg)
			: ErrorBase(type_, level_, msg), code(code_)
		{ }

		/// Constructor
		ErrorCodeHolder(const std::string& type_, ErrorLevel level_, const CodeType& code_)
			: ErrorBase(type_, level_), code(code_)
		{ }

	public:

		// Reimplemented from ErrorBase
		const std::type_info& get_code_type() const override
		{
			return typeid(CodeType);
		}

		CodeType code = CodeType();  ///< Error code. We have a class specialization for references too

};



// Specialization for void, helpful for custom messages
template<>
class ErrorCodeHolder<void> : public ErrorBase {
	protected:

		/// Constructor
		ErrorCodeHolder(const std::string& type_, ErrorLevel level_, const std::string& msg)
			: ErrorBase(type_, level_, msg)
		{ }

	public:

		// Reimplemented from ErrorBase
		const std::type_info& get_code_type() const override
		{
			return typeid(void);
		}

};






/// Error class. Stores an error code of type \c CodeType.
/// Instantiate this in user code.
template<typename CodeType>
class Error : public ErrorCodeHolder<CodeType> {
	public:

		/// Constructor
		Error(const std::string& type_, ErrorLevel level_, const CodeType& code_,
				const std::string& msg)
			: ErrorCodeHolder<CodeType>(type_, level_, code_, msg)
		{ }

		// Reimplemented from ErrorBase
		ErrorBase* clone() override
		{
			return new Error(ErrorCodeHolder<CodeType>::type, ErrorCodeHolder<CodeType>::level,
					ErrorCodeHolder<CodeType>::code, ErrorCodeHolder<CodeType>::message);
		}
};




/// Error class specialization for void (helpful for custom messages).
template<>
class Error<void> : public ErrorCodeHolder<void> {
	public:

		Error(const std::string& type_, ErrorLevel level_, const std::string& msg)
			: ErrorCodeHolder<void>(type_, level_, msg)
		{ }

		// Reimplemented from ErrorBase
		ErrorBase* clone() override
		{
			return new Error(ErrorCodeHolder<void>::type, ErrorCodeHolder<void>::level,
					ErrorCodeHolder<void>::message);
		}
};




/// Error class specialization for int (can be used for signals, errno).
/// Message is automatically constructed if not provided.
template<>
class Error<int> : public ErrorCodeHolder<int> {
	public:

		/// Constructor
		Error(const std::string& type_, ErrorLevel level_, int code_, const std::string& msg)
			: ErrorCodeHolder<int>(type_, level_, code_, msg)
		{ }

		/// Constructor
		Error(const std::string& type_, ErrorLevel level_, int code_)
			: ErrorCodeHolder<int>(type_, level_, code_)
		{
			if (type == "errno") {
				message = std::error_code(code_, std::system_category()).message();

			} else if (type == "signal") {
				// hz::signal_string should be translated already
				message = "Child exited with signal: " + hz::signal_to_string(code_);

			} else {  // nothing else supported here. use constructor with a message.
				DBG_ASSERT(0);
			}
		}

		// Reimplemented from ErrorBase
		ErrorBase* clone() override
		{
			return new Error(ErrorCodeHolder<int>::type, ErrorCodeHolder<int>::level,
					ErrorCodeHolder<int>::code, ErrorCodeHolder<int>::message);
		}
};




/*
namespace {

	Error<int> e1("type1", ErrorLevel::info, 6);
	Error<std::string> e2("type2", ErrorLevel::fatal, "asdasd", "aaa");

	std::bad_alloc ex3;
// 	Error<std::exception&> e3("type3", ErrorBase::info, static_cast<std::exception&>(ex3));
	Error<std::exception&> e3("type3", ErrorLevel::info, ex3);


	Glib::IOChannelError ex4(Glib::IOChannelError::FILE_TOO_BIG, "message4");
	Error<Glib::Error&> e4("type4", ErrorLevel::info, ex4);

}
*/



}  // ns





#endif

/// @}
