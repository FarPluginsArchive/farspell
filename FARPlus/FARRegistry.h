/* $Header: /cvsroot/farplus/FARPlus/FARRegistry.h,v 1.2 2002/09/03 06:32:11 yole Exp $
   FAR+Plus: registry access class interface
   (C) 2001-02 Dmitry Jemerov <yole@yole.ru>
*/

#ifndef __FARREGISTRY_H
#define __FARREGISTRY_H

#include <windows.h>
#include "FARString.h"

#if _MSC_VER >= 1000
#pragma once
#endif

class FarRegistry
{
private:
	class BaseIterator
	{
	protected:
		HKEY fEnumKey;
		int fEnumIndex;
		BaseIterator (HKEY enumKey)
			: fEnumKey (enumKey), fEnumIndex (0) {};

	public:
		BaseIterator (BaseIterator &other)
		{
			fEnumKey = other.fEnumKey;
			fEnumIndex = other.fEnumIndex;
			other.fEnumKey = NULL;
		}
		virtual ~BaseIterator();
	};

protected:
	FarString fRootKey;

    HKEY CreateRegKey (HKEY hRoot, const char *Key);
    HKEY OpenRegKey (HKEY hRoot, const char *Key, bool readOnly);

public:
	class KeyIterator: public BaseIterator
	{
	private:
		friend class FarRegistry;
        KeyIterator (HKEY enumKey)
			: BaseIterator (enumKey) {};

	public:
		bool NextKey (FarString &keyName);
	};

	class ValueIterator: public BaseIterator
	{
	private:
		friend class FarRegistry;
		ValueIterator (HKEY enumKey)
			: BaseIterator (enumKey) {};

	public:
		bool NextValue (FarString &valueName, FarString &valueData);
	};

	FarRegistry (const char *rootKey)
		: fRootKey (rootKey) {};
	FarRegistry (const char *rootKeyStart, const char *rootKeyBody)
		: fRootKey (rootKeyStart)
	{
		fRootKey += rootKeyBody;
	}

    bool SetRegKey(const char *Key,const char *ValueName,
        const char *ValueData,HKEY hRoot=HKEY_CURRENT_USER);
    bool SetRegKey(const char *Key,const char *ValueName,
        const void*ValueData, DWORD ValueSize,HKEY hRoot=HKEY_CURRENT_USER);
    bool SetRegKey(const char *Key,const char *ValueName,
        DWORD ValueData,HKEY hRoot=HKEY_CURRENT_USER);

    int GetValueSize(const char *Key, const char *ValueName, HKEY hRoot=HKEY_CURRENT_USER);

    int GetRegKey (const char *Key, const char *ValueName,
        char *ValueData,const char *Default,DWORD DataSize,HKEY hRoot=HKEY_CURRENT_USER);
    int GetRegKey (const char *Key, const char *ValueName,
        void *ValueData,DWORD &DataSize,const void *Default,DWORD DefaultSize,HKEY hRoot=HKEY_CURRENT_USER);
    int GetRegKey (const char *Key, const char *ValueName,
        int &ValueData,DWORD Default,HKEY hRoot=HKEY_CURRENT_USER);
    int GetRegKey (const char *Key, const char *ValueName,
        DWORD Default,HKEY hRoot=HKEY_CURRENT_USER);
	bool GetRegBool (const char *Key, const char *ValueName,
		bool Default, HKEY hRoot=HKEY_CURRENT_USER);
	FarString GetRegStr (const char *Key, const char *ValueName,
		const char *Default, HKEY hRoot=HKEY_CURRENT_USER);
    template <class TChar>
    FarStringT<TChar> GetFarString(const char *Key, const char *ValueName,
      const FarStringT<TChar> &Default, HKEY hRoot=HKEY_CURRENT_USER);
    template <class TChar>
    bool SetFarString(const char *Key, const char *ValueName,
      const FarStringT<TChar> &Value, HKEY hRoot=HKEY_CURRENT_USER);

    int DeleteRegKey (const char *RootKey, const char *KeyName, 
        HKEY hRoot=HKEY_CURRENT_USER);

	bool KeyExists (const char *key, const char *valueName,
		HKEY hRoot = HKEY_CURRENT_USER);

	KeyIterator EnumKeys (const char *Key, HKEY hRoot = HKEY_CURRENT_USER);
	ValueIterator EnumValues (const char *Key, HKEY hRoot = HKEY_CURRENT_USER);
};

#ifdef __GNUC__
// Explicit template instatiations
template FarStringT<char> FarRegistry::GetFarString (const char *Key, 
	const char *ValueName, const FarStringT<char> &Default, HKEY hRoot);
template FarStringT<wchar_t> FarRegistry::GetFarString (const char *Key, 
	const char *ValueName, const FarStringT<wchar_t> &Default, HKEY hRoot);
template bool FarRegistry::SetFarString(const char *Key, const char *ValueName,
      const FarStringT<char> &Value, HKEY hRoot);
template bool FarRegistry::SetFarString(const char *Key, const char *ValueName,
      const FarStringT<wchar_t> &Value, HKEY hRoot);
#endif

inline bool FarRegistry::GetRegBool (const char *Key, const char *ValueName, 
									 bool Default, HKEY hRoot/* =HKEY_CURRENT_USER */)
{
	return GetRegKey (Key, ValueName, Default, hRoot) ? true : false;
}

#endif
