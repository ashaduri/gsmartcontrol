/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2024 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/

#include "storage_property_repository.h"

#include <algorithm>
#include <string>
#include <utility>




const std::vector<StorageProperty>& StoragePropertyRepository::get_properties() const
{
	return properties_;
}



std::vector<StorageProperty>& StoragePropertyRepository::get_properties_ref()
{
	return properties_;
}



StorageProperty StoragePropertyRepository::lookup_property(
		const std::string& generic_name, StorageProperty::Section section) const
{
	for (const auto& p : properties_) {
		if (section != StorageProperty::Section::Unknown && p.section != section)
			continue;

		if (p.generic_name == generic_name)
			return p;
	}
	return {};  // check with .empty()
}



void StoragePropertyRepository::set_properties(std::vector<StorageProperty> properties)
{
	properties_ = std::move(properties);
}



void StoragePropertyRepository::add_property(StorageProperty property)
{
	properties_.push_back(std::move(property));
}



void StoragePropertyRepository::clear()
{
	properties_.clear();
}



bool StoragePropertyRepository::has_properties_for_section(StorageProperty::Section section) const
{
	return std::any_of(properties_.begin(), properties_.end(),
			[section](const StorageProperty& p) { return p.section == section; });
}

