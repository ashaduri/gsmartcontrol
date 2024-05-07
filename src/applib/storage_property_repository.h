/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2024 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
#ifndef STORAGE_PROPERTY_REPOSITORY_H
#define STORAGE_PROPERTY_REPOSITORY_H

#include <string>
#include <vector>
#include "storage_property.h"


/// A repository of properties. Used to store and look up drive properties.
class StoragePropertyRepository {
	public:

		/// Get all properties
		[[nodiscard]] const std::vector<StorageProperty>& get_properties() const;

		/// Get all properties
		[[nodiscard]] std::vector<StorageProperty>& get_properties_ref();


		/// Find a property.
		/// If section is Section::Unknown, search in all sections.
		[[nodiscard]] StorageProperty lookup_property(const std::string& generic_name,
				StorageProperty::Section section = StorageProperty::Section::Unknown) const;


		/// Set properties
		void set_properties(std::vector<StorageProperty> properties);

		/// Add a property
		void add_property(StorageProperty property);

		/// Clear all properties
		void clear();


	private:

		std::vector<StorageProperty> properties_;  ///< Parsed data properties

};


#endif  // STORAGE_PROPERTY_REPOSITORY_H
