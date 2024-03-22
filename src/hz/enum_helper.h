/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2022 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef ENUM_HELPER_H
#define ENUM_HELPER_H

#include <string>
#include <vector>
#include <utility>
#include <unordered_map>
#include <algorithm>



namespace hz {


/// Helper class for defining enum-related functions.
/// In EnumExtClass it expects the following (accessible) members:
/// - static inline EnumType default_value = ...;
/// - static const std::unordered_map<EnumType, std::pair<std::string, DisplayableStringType>>& get_enum_static_map();
template <typename Enum, typename EnumExtClass, typename DisplayableStringType>
class EnumHelper {
	public:

		using EnumType = Enum;
		using EnumMapType = std::unordered_map<EnumType, std::pair<std::string, DisplayableStringType>>;


		/// Return storable name of an enum member
		[[nodiscard]] static std::string get_storable_name(EnumType enum_value)
		{
			const auto& m = get_enum_static_map();
			// Iterator: enum -> pair{storable, displayable}
			auto iter = m.find(enum_value);
			return iter != m.end() ? std::string(iter->second.first) : std::string();
		}


		/// Return an enum member by its storable name
		[[nodiscard]] static EnumType get_by_storable_name(const std::string& storable_name,
				EnumType default_value = EnumExtClass::default_value)
		{
			const auto& m = get_storable_enum_static_map();
			// Iterator: storable_name -> enum
			auto iter = m.find(storable_name);
			return iter != m.end() ? iter->second : default_value;
		}


		/// Return displayable name of an enum member
		[[nodiscard]] static DisplayableStringType get_displayable_name(EnumType enum_value)
		{
			const auto& m = get_enum_static_map();
			// Iterator: enum -> pair{storable, displayable}
			auto iter = m.find(enum_value);
			return iter != m.end() ? DisplayableStringType(iter->second.second) : DisplayableStringType();
		}


		/// Return all possible members of an enum
		[[nodiscard]] static std::vector<EnumType> getAllValues()
		{
			static const auto v = build_enum_value_list();
			return v;
		}


	private:

		/// Get a static map of storable names to enum values.
		[[nodiscard]] static const EnumMapType& get_enum_static_map()
		{
			static const auto m = EnumExtClass::build_enum_map();
			return m;
		}


		/// Get a static map of storable names to enum values.
		[[nodiscard]] static const std::unordered_map<std::string, EnumType>& get_storable_enum_static_map()
		{
			static const auto m = build_storable_enum_map();
			return m;
		}


		/// Build a map of storable names to enum values.
		static std::unordered_map<std::string, EnumType> build_storable_enum_map()
		{
			std::unordered_map<std::string, EnumType> m;
			for (const auto& [data, enum_value] : EnumExtClass::get_enum_static_map()) {
				m.emplace(data.first, enum_value);
			}
			return m;
		}


		/// Build a list of enum values from get_enum_static_map()
		static std::vector<EnumType> build_enum_value_list()
		{
			const auto& m = EnumExtClass::get_enum_static_map();
			std::vector<EnumType> v;
			v.reserve(m.size());
			for (const auto& p : m) {
				v.push_back(p.first);
			}
			std::sort(v.begin(), v.end());
			return v;
		}

};



}



#endif

/// @}
