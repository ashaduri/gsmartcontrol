/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2024 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
#ifndef STORAGE_PROPERTY_REPOSITORY_H
#define STORAGE_PROPERTY_REPOSITORY_H

#include <vector>
#include "ata_storage_property.h"


/// A repository of properties. Used to store and look up drive properties.
class StoragePropertyRepository {
	public:

		/// Get all properties
		[[nodiscard]] const std::vector<AtaStorageProperty>& get_properties() const;

		/// Get all properties
		[[nodiscard]] std::vector<AtaStorageProperty>& get_properties_ref();


		/// Find a property.
		/// If section is Section::Unknown, search in all sections.
		[[nodiscard]] AtaStorageProperty lookup_property(const std::string& generic_name,
				AtaStorageProperty::Section section = AtaStorageProperty::Section::Unknown) const;


		/// Set properties
		void set_properties(std::vector<AtaStorageProperty> properties);

		/// Add a property
		void add_property(AtaStorageProperty property);

		/// Clear all properties
		void clear();


	private:

		std::vector<AtaStorageProperty> properties_;  ///< Parsed data properties

};


#endif  // STORAGE_PROPERTY_REPOSITORY_H
