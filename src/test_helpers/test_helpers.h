/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2022 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup test_helpers
/// \weakgroup test_helpers
/// @{

#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include "leaf_ns.h"

#include <utility>



/// From https://github.com/boostorg/leaf/issues/42
/// \tparam TryBlock A lambda
/// \tparam E... Any number of error values with _different_ types.
/// \return true if try_block fails AND produces at least one error object that
/// compares equal to the specified error value of the same type.
template<class... E, class TryBlock>
bool try_expect_errors(TryBlock&& try_block, E&& ... expected)
{
	return leaf::try_handle_all(
		[&]() -> leaf::result<bool> {
			auto r = std::forward<TryBlock>(try_block)();
			if (r) {
				return false;
			}
			return r.error();
		},
		[&](E& e) {
			return e == std::forward<E>(expected);
		}...,
		[] {
			return false;
		});
}




#endif

/// @}
