/******************************************************************************
License: Zlib
Copyright:
	(C) 2024 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_ERROR_H
#define HZ_ERROR_H

#include <source_location>
#include <string>
#include <tl/expected.hpp>
#include <utility>
// #include <stacktrace>



namespace hz {



/// ErrorContainer is a generic class that can carry any type of error data.
/// Based on:
/// Exceptionally Bad: The Misuse of Exceptions in C++ & How to Do Better - Peter Muldoon - CppCon 2023
template<typename ErrorData>
class ErrorContainer {
	public:

		/// Constructor
		ErrorContainer(ErrorData data, std::string error_message,
				const std::source_location& loc = std::source_location::current()
				// std::stacktrace trace = std::stacktrace::current()
			)
			: error_message_(std::move(error_message)),
			data_(std::move(data)),
			location_(loc)
			// backtrace_(trace)
		{ }


		/// Get the error data
		[[nodiscard]] const ErrorData& data() const noexcept
		{
			return data_;
		}


		/// Get the error data reference
		// [[nodiscard]] ErrorData& get_data_ref()
		// {
		// 	return data_;
		// }


		/// Get the error message
		[[nodiscard]] const std::string& message() const noexcept
		{
			return error_message_;
		}


		/// Get the error message reference
		// [[nodiscard]] std::string& get_message_ref()
		// {
		// 	return error_message_;
		// }


		/// Get the error source location
		[[nodiscard]] const std::source_location& where() const
		{
			return location_;
		}


		/// Get the error stack trace
		// [[nodiscard]] const std::stacktrace& stack() const
		// {
		// 	return backtrace_;
		// }


	private:

		std::string error_message_;
		ErrorData data_;
		std::source_location location_;
		// std::stacktrace backtrace_;
};



/// ExpectedValue is a wrapper around tl::expected that uses ErrorContainer as the error type.
template<typename ValueType, typename ErrorType>
using ExpectedValue = tl::expected<ValueType, ErrorContainer<ErrorType>>;


/// ExpectedVoid is a wrapper around tl::expected that uses ErrorContainer as the error type and void as value.
template<typename ErrorType>
using ExpectedVoid = tl::expected<void, ErrorContainer<ErrorType>>;


/// Unexpected creates an unexpected value with an ErrorContainer as error.
template<typename ErrorContainerWithData>
auto UnexpectedFromContainer(const ErrorContainerWithData& container)
{
	return tl::unexpected(container);
}


/// Unexpected creates an unexpected value with an ErrorContainer.
template<typename ErrorData>
auto Unexpected(ErrorData&& data, std::string error_message,
		const std::source_location& loc = std::source_location::current()
		// std::stacktrace trace = std::stacktrace::current()
	)
{
	return tl::unexpected(ErrorContainer<ErrorData>(std::forward<ErrorData>(data),
			std::move(error_message),
			loc
			// trace
	));
}


/// UnexpectedFrom creates an unexpected value from ExpectedValue or ExpectedVoid
/// containing an error.
template<typename ExpectedValueT>
auto UnexpectedFrom(ExpectedValueT unexpected_value)
{
	return tl::unexpected(unexpected_value.error());
}



/*
std::ostream& operator<< (std::ostream& os, const std::source_location& location)
{
	return os << location.file_name() << "("
		<< location.line() << ":"
		<< location.column() << "), function `"
		<< location.function_name() << "`";
}


std::ostream& operator<< (std::ostream& os, const std::stacktrace& backtrace)
{
	if (backtrace.size() <= 3>) {
		os << "No stack trace available.\n";
		return os;
	}
	if (backtrace.size() > 3) {
		os << "Stack trace (most recent call last):\n";
		for (auto iter = backtrace.begin(); iter != (backtrace.end() - 3); ++iter) {
			os << iter->source_file() << "(" << iter->source_line()
			<< ") : " << iter->description() << "\n";
		}
	}
	return os;
}
*/




}  // ns





#endif

/// @}
