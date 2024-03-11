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

#include <string_view>
#include <source_location>
#include <string>
#include <expected>
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



template<typename ValueType, typename ErrorType>
using ExpectedValue = std::expected<ValueType, ErrorContainer<ErrorType>>;


template<typename ErrorType>
using ExpectedVoid = std::expected<void, ErrorContainer<ErrorType>>;


template<typename ErrorData>
auto Unexpected(ErrorData&& data, std::string error_message,
		const std::source_location& loc = std::source_location::current()
		// std::stacktrace trace = std::stacktrace::current()
	)
{
	return std::unexpected(ErrorContainer<ErrorData>(std::forward<ErrorData>(data),
	        std::move(error_message),
			loc
			// trace
	));
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
