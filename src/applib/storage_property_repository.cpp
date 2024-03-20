/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2024 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/

#include "storage_property_repository.h"




const std::vector<AtaStorageProperty>& StoragePropertyRepository::get_properties() const
{
	return properties_;
}



std::vector<AtaStorageProperty>& StoragePropertyRepository::get_properties_ref()
{
	return properties_;
}



AtaStorageProperty StoragePropertyRepository::lookup_property(
		const std::string& generic_name, AtaStorageProperty::Section section, AtaStorageProperty::SubSection subsection) const
{
	for (const auto& p : properties_) {
		if (section != AtaStorageProperty::Section::unknown && p.section != section)
			continue;
		if (subsection != AtaStorageProperty::SubSection::unknown && p.subsection != subsection)
			continue;

		if (p.generic_name == generic_name)
			return p;
	}
	return {};  // check with .empty()
}



void StoragePropertyRepository::set_properties(std::vector<AtaStorageProperty> properties)
{
	properties_ = std::move(properties);
}



void StoragePropertyRepository::add_property(AtaStorageProperty property)
{
	properties_.push_back(std::move(property));
}



void StoragePropertyRepository::clear()
{
	properties_.clear();
}

