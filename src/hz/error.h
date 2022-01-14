/******************************************************************************
License: Zlib
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
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
#include <utility>

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
		ErrorBase(std::string type, ErrorLevel level, std::string message)
				: type_(std::move(type)), level_(level), message_(std::move(message))
		{ }

		/// Constructor
		ErrorBase(std::string type, ErrorLevel level)
				: type_(std::move(type)), level_(level)
		{ }

		/// Defaulted
		ErrorBase(const ErrorBase& other) = default;

		/// Defaulted
		ErrorBase(ErrorBase&& other) = default;

		/// Defaulted
		ErrorBase& operator=(const ErrorBase&) = default;

		/// Defaulted
		ErrorBase& operator=(ErrorBase&&) = default;


		/// Virtual destructor
		virtual ~ErrorBase() = default;


		/// Clone this object
		[[nodiscard]] virtual ErrorBase* clone() = 0;  // needed for copying by base pointers


		/// Get std::type_info for the error code type.
		[[nodiscard]] virtual const std::type_info& get_code_type_info() const = 0;

		
		/// Get error code of type \c CodeMemberType
		template<class CodeMemberType>
		CodeMemberType get_code() const  // this may throw on bad cast!
		{
			if (get_code_type_info() != typeid(CodeMemberType))
				throw type_mismatch(get_code_type_info(), typeid(CodeMemberType));
			return static_cast<const Error<CodeMemberType>*>(this)->code;
		}

		/// Get error code of type \c CodeMemberType
		template<class CodeMemberType>
		bool get_code(CodeMemberType& put_it_here) const  // this doesn't throw
		{
			if (get_code_type_info() != typeid(CodeMemberType))
				return false;
			put_it_here = static_cast<const Error<CodeMemberType>*>(this)->get_code_member();
			return true;
		}


		/// Increase the level (severity) of the error
		ErrorLevel level_inc()
		{
			if (level_ == ErrorLevel::fatal)
				return level_;
			return (level_ = static_cast<ErrorLevel>(static_cast<int>(level_) << 1));
		}

		/// Decrease the level (severity) of the error
		ErrorLevel level_dec()
		{
			if (level_ == ErrorLevel::none)
				return level_;
			return (level_ = static_cast<ErrorLevel>(static_cast<int>(level_) >> 1));
		}

		/// Get error level (severity)
		[[nodiscard]] ErrorLevel get_level() const
		{
			return level_;
		}


		/// Get error type
		[[nodiscard]] std::string get_type() const
		{
			return type_;
		}

		/// Get error message
		[[nodiscard]] std::string get_message() const
		{
			return message_;
		}


	protected:

		/// Set error type
		void set_type(std::string type)
		{
			type_ = std::move(type);
		}


		/// Set error level
		void set_level(ErrorLevel level)
		{
			level_ = level;
		}


		/// Set error message
		void set_message(std::string message)
		{
			message_ = std::move(message);
		}


	private:

		std::string type_;  ///< Error type
		ErrorLevel level_ = ErrorLevel::none;  ///< Error severity
		std::string message_;  ///< Error message

};





// Provides some common stuff for Error to ease template specializations.
template<typename CodeType>
class ErrorCodeHolder : public ErrorBase {
	protected:

		/// Constructor
		ErrorCodeHolder(const std::string& type, ErrorLevel level, const CodeType& code,
				const std::string& msg)
			: ErrorBase(type, level, msg), code_(code)
		{ }

		/// Constructor
		ErrorCodeHolder(const std::string& type, ErrorLevel level, const CodeType& code)
			: ErrorBase(type, level), code_(code)
		{ }

	public:

		// Reimplemented from ErrorBase
		[[nodiscard]] const std::type_info& get_code_type_info() const override
		{
			return typeid(CodeType);
		}

		// Reimplemented from ErrorBase
		[[nodiscard]] const CodeType& get_code_member() const
		{
			return code_;
		}

	private:

		CodeType code_ = CodeType();  ///< Error code. We have a class specialization for references too

};



// Specialization for void, helpful for custom messages
template<>
class ErrorCodeHolder<void> : public ErrorBase {
	protected:

		/// Constructor
		ErrorCodeHolder(const std::string& type, ErrorLevel level, const std::string& msg)
			: ErrorBase(type, level, msg)
		{ }

	public:

		// Reimplemented from ErrorBase
		[[nodiscard]] const std::type_info& get_code_type_info() const override
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
		Error(const std::string& type, ErrorLevel level, const CodeType& code,
				const std::string& msg)
			: ErrorCodeHolder<CodeType>(type, level, code, msg)
		{ }

		// Reimplemented from ErrorBase
		ErrorBase* clone() override
		{
			return new Error(ErrorCodeHolder<CodeType>::get_type(), ErrorCodeHolder<CodeType>::get_level(),
					ErrorCodeHolder<CodeType>::get_code_member(), ErrorCodeHolder<CodeType>::get_message());
		}
};




/// Error class specialization for void (helpful for custom messages).
template<>
class Error<void> : public ErrorCodeHolder<void> {
	public:

		Error(const std::string& type, ErrorLevel level, const std::string& msg)
			: ErrorCodeHolder<void>(type, level, msg)
		{ }

		// Reimplemented from ErrorBase
		ErrorBase* clone() override
		{
			return new Error(ErrorCodeHolder<void>::get_type(), ErrorCodeHolder<void>::get_level(),
					ErrorCodeHolder<void>::get_message());
		}
};




/// Error class specialization for int (can be used for signals, errno).
/// Message is automatically constructed if not provided.
template<>
class Error<int> : public ErrorCodeHolder<int> {
	public:

		/// Constructor
		Error(const std::string& type, ErrorLevel level, int code, const std::string& msg)
			: ErrorCodeHolder<int>(type, level, code, msg)
		{ }

		/// Constructor
		Error(const std::string& type, ErrorLevel level, int code)
			: ErrorCodeHolder<int>(type, level, code)
		{
			if (type == "errno") {
				this->set_message(std::error_code(code, std::system_category()).message());

			} else if (type == "signal") {
				// hz::signal_string should be translated already
				this->set_message("Child exited with signal: " + hz::signal_to_string(code));

			} else {  // nothing else supported here. use constructor with a message.
				DBG_ASSERT(0);
			}
		}

		// Reimplemented from ErrorBase
		ErrorBase* clone() override
		{
			return new Error(ErrorCodeHolder<int>::get_type(), ErrorCodeHolder<int>::get_level(),
					ErrorCodeHolder<int>::get_code_member(), ErrorCodeHolder<int>::get_message());
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
