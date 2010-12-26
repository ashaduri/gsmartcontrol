/**************************************************************************
 Copyright:
      (C) 2000 - 2009  Kevlin Henney
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_boost_1_0.txt file
***************************************************************************/

#ifndef HZ_ANY_TYPE_HOLDER_H
#define HZ_ANY_TYPE_HOLDER_H

#include "hz_config.h"  // feature macros

/*
Internal header, do not include manually.
*/

#if !(defined DISABLE_RTTI && DISABLE_RTTI)
	#include <typeinfo>  // std::type_info
#endif

#include <iosfwd>  // std::ostream
#include <string>

#if !(defined DISABLE_ANY_CONVERT && DISABLE_ANY_CONVERT)
	#include "any_convert.h"  // any_convert
#endif


namespace hz {



namespace internal {




// base class for content
struct AnyHolderBase {

	virtual ~AnyHolderBase() { }

#if !(defined DISABLE_RTTI && DISABLE_RTTI)
	virtual const std::type_info& type() const = 0;
#endif
	virtual AnyHolderBase* clone() const = 0;

	virtual void to_stream(std::ostream& os) const = 0;


#if !(defined DISABLE_ANY_CONVERT && DISABLE_ANY_CONVERT)

	virtual bool convert(bool& val) const = 0;

	virtual bool convert(char& val) const = 0;
	virtual bool convert(signed char& val) const = 0;
	virtual bool convert(unsigned char& val) const = 0;
	virtual bool convert(wchar_t& val) const = 0;

	virtual bool convert(short int& val) const = 0;
	virtual bool convert(unsigned short int& val) const = 0;
	virtual bool convert(int& val) const = 0;
	virtual bool convert(unsigned int& val) const = 0;
	virtual bool convert(long int& val) const = 0;
	virtual bool convert(unsigned long int& val) const = 0;
#if !(defined DISABLE_LL_INT && DISABLE_LL_INT)
	virtual bool convert(long long int& val) const = 0;
#endif
#if !(defined DISABLE_ULL_INT && DISABLE_ULL_INT)
	virtual bool convert(unsigned long long int& val) const = 0;
#endif

	virtual bool convert(double& val) const = 0;
	virtual bool convert(float& val) const = 0;
	virtual bool convert(long double& val) const = 0;

	virtual bool convert(std::string& val) const = 0;

#endif  // DISABLE_ANY_CONVERT

};





// child class for content, holds the actual value
template<typename ValueType>
struct AnyHolder : public AnyHolderBase {

	typedef ValueType value_type;

	AnyHolder(const ValueType& val)
		: value(val)
	{ }


#if !(defined DISABLE_RTTI && DISABLE_RTTI)
	const std::type_info& type() const
	{
		return typeid(ValueType);
	}
#endif

	AnyHolderBase* clone() const
	{
		return new AnyHolder(value);
	}


	inline void to_stream(std::ostream& os) const;
// 	{
// 		AnyPrinter<ValueType>::to_stream(os, value);
// 	}



#if !(defined DISABLE_ANY_CONVERT && DISABLE_ANY_CONVERT)

	bool convert(bool& val) const { return any_convert(value, val); }

	bool convert(char& val) const { return any_convert(value, val); }
	bool convert(signed char& val) const { return any_convert(value, val); }
	bool convert(unsigned char& val) const { return any_convert(value, val); }
	bool convert(wchar_t& val) const { return any_convert(value, val); }

	bool convert(short int& val) const { return any_convert(value, val); }
	bool convert(unsigned short int& val) const { return any_convert(value, val); }
	bool convert(int& val) const { return any_convert(value, val); }
	bool convert(unsigned int& val) const { return any_convert(value, val); }
	bool convert(long int& val) const { return any_convert(value, val); }
	bool convert(unsigned long int& val) const { return any_convert(value, val); }
#if !(defined DISABLE_LL_INT && DISABLE_LL_INT)
	bool convert(long long int& val) const { return any_convert(value, val); }
#endif
#if !(defined DISABLE_ULL_INT && DISABLE_ULL_INT)
	bool convert(unsigned long long int& val) const { return any_convert(value, val); }
#endif

	bool convert(double& val) const { return any_convert(value, val); }
	bool convert(float& val) const { return any_convert(value, val); }
	bool convert(long double& val) const { return any_convert(value, val); }

	bool convert(std::string& val) const { return any_convert(value, val); }

#endif // DISABLE_ANY_CONVERT


	ValueType value;  // the actual data
};








}  // ns
}  // ns



#endif
