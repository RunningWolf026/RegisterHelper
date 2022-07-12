#include "stdafx.h"
#include "GRegister.h"

#include <assert.h>
#include <boost/algorithm/string/replace.hpp>
#include <vector>
#include <map>
#include "GScopeExit.h"
#pragma comment (lib,"Advapi32.lib")
typedef LSTATUS(WINAPI *myRegDeleteKeyExW)(HKEY hKey, LPCWSTR lpSubKey, REGSAM samDesired, DWORD Reserved);

#define CHECKSUCCESS(lVal) (ERROR_SUCCESS == lVal)

#define STRINGCAPACITY 8 * 1024

const std::map<wstring, HKEY> c_mapRootKeys = 
{
{L"HKEY_CLASSES_ROOT", HKEY_CLASSES_ROOT },
{L"HKEY_CURRENT_USER", HKEY_CURRENT_USER },
{L"HKEY_LOCAL_MACHINE", HKEY_LOCAL_MACHINE },
{L"HKEY_USERS", HKEY_USERS },
{L"HKEY_PERFORMANCE_DATA", HKEY_PERFORMANCE_DATA },
{L"HKEY_CURRENT_CONFIG", HKEY_CURRENT_CONFIG },
{L"HKEY_DYN_DATA", HKEY_DYN_DATA }
};

bool isRelative(wstring &value)
{
	if (value.empty())
		return true;
    // 转义转化
    boost::algorithm::replace_all(value, L"/", L"\\");
    // 出去value最后的"\\"
	size_t nLen = value.length();
	if (value[nLen - 1] == L'\\')
    {
        value = value.substr(0, nLen - 1);
    }
    return value.at(0) != L'\\';
}

GRegDataType dataTypeToRegData(int value)
{
    GRegDataType result;
    switch (value)
    {
    case REG_SZ:
        result = rdtString;
    	break;
    case REG_EXPAND_SZ:
        result = rdtExpandString;
        break;
    case REG_DWORD:
        result = rdtInt;
        break;
    case REG_BINARY:
        result = rdtBinary;
        break;
    default:
        result = rdtUnknown;
    }
    return result;
}

GRegister::GRegister(const HKEY &hKey /*= HKEY_LOCAL_MACHINE*/, const ACCESS_MASK &hAccess /*= KEY_ALL_ACCESS*/) :
m_rootKey(hKey)
, m_currentKey(0)
, m_currentPath(L"")
, m_access(hAccess)
{

}

GRegister::~GRegister()
{
    closeKey();
}

HKEY GRegister::strToRootKey(const wstring &str)
{
	auto it = c_mapRootKeys.find(str);
	if (it != c_mapRootKeys.end())
		return it->second;
	else
		return HKEY_CLASSES_ROOT;
}

std::wstring GRegister::rootKeyToStr(const HKEY &hKey)
{
	auto it = std::find_if(c_mapRootKeys.cbegin(), c_mapRootKeys.cend(),
		[hKey](const std::pair<wstring, HKEY>&pair)
	{
		return hKey == pair.second;
	});
	if (it != c_mapRootKeys.end())
		return it->first;
	else
		return L"HKEY_CLASSES_ROOT";
}

bool GRegister::createKey(const wstring &wsSubKey)
{
    assert(m_rootKey);
    assert(wsSubKey != L"");

    wstring wsTemp = wsSubKey;
    bool bRelative = isRelative(wsTemp);
    if (!bRelative)
    {
        wsTemp = wsTemp.substr(1);
    }

    ACCESS_MASK flag = m_access & KEY_WOW64_RES;

    HKEY hKey;
    DWORD dw;
    if (CHECKSUCCESS(RegCreateKeyEx(getBaseKey(bRelative), wsTemp.c_str(), 0L, NULL, REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS | flag, NULL, &hKey, &dw)))
    {
        RegCloseKey(hKey);
        return true;
    }
    
    return false;
}

bool GRegister::deleteKey(const wstring &wsSubKey)
{
    assert(m_rootKey);
    assert(wsSubKey != L"");

    wstring wsTemp = wsSubKey;
    bool nRelative = isRelative(wsTemp);
    if (!nRelative)
    {
        wsTemp = wsTemp.substr(1);
    }

    HKEY wsOldKey = m_currentKey;
    HKEY wsDeleteKey = getKey(wsSubKey);

    if (wsDeleteKey)
    {
		setCurrentKey(wsDeleteKey);
		GSCOPEEXIT
		{
			setCurrentKey(wsOldKey);
			RegCloseKey(wsDeleteKey);
		};
		GRegKeyInfo info;
		wstring wsKeyName;
		DWORD dwLen;
		if (getKeyInfo(info))
		{
			wsKeyName.assign(info.maxSubKeyLen + 1, NULL);
			for (int i = info.numSubKeys - 1; i >= 0; i--)
			{
				dwLen = info.maxSubKeyLen + 1;
				if (CHECKSUCCESS(RegEnumKeyEx(wsDeleteKey, DWORD(i), (wchar_t *)wsKeyName.c_str(), &dwLen, NULL, NULL, NULL, NULL)))
				{
					deleteKey(wsKeyName);
				}
			}
		}
    }

	myRegDeleteKeyExW pRegDeleteKeyExW = (myRegDeleteKeyExW)GetProcAddress(GetModuleHandle(L"advapi32.dll"), "RegDeleteKeyExW");
    if (pRegDeleteKeyExW)
    {
        return CHECKSUCCESS(pRegDeleteKeyExW(getBaseKey(nRelative), wsTemp.c_str(), m_access & KEY_WOW64_RES, 0));
    } 
    else
    {
        return CHECKSUCCESS(RegDeleteKey(getBaseKey(nRelative), wsTemp.c_str()));
    }
}

bool GRegister::deleteValue(const wstring &wsValue)
{
    assert(m_rootKey);

    return CHECKSUCCESS(RegDeleteValue(m_currentKey, wsValue.c_str()));
}

bool GRegister::openKey(const wstring &wsSubKey, const bool bCanCreate /*= false*/)
{
    assert(m_rootKey);
    assert(wsSubKey != L"");

    wstring wsTemp = wsSubKey;
    bool bRelative = isRelative(wsTemp);
    if (!bRelative)
    {
        wsTemp = wsTemp.substr(1);
    }
    HKEY hKey;
    DWORD dw;
    bool bRel = false;
    if (!bCanCreate || (wsTemp.empty()))
    {
        bRel = CHECKSUCCESS(RegOpenKeyEx(getBaseKey(bRelative), wsTemp.c_str(), 0, m_access, &hKey));
    } 
    else
    {
        bRel = CHECKSUCCESS(RegCreateKeyEx(getBaseKey(bRelative), wsTemp.c_str(), 0, NULL,
        REG_OPTION_NON_VOLATILE, m_access, NULL, &hKey, &dw));
    }

    if (bRel)
    {
        if ((m_currentKey != 0) && bRelative)
        {
            wsTemp = m_currentPath + L"\\" + wsTemp;
        }
        changeKey(hKey, wsTemp);
    }

    return bRel;
}

bool GRegister::openKeyReadOnly(const wstring &wsSubKey)
{
    assert(m_rootKey);
    assert(wsSubKey != L"");

    wstring wsTemp = wsSubKey;
    bool bRelative = isRelative(wsTemp);
    if (!bRelative)
    {
        wsTemp = wsTemp.substr(1);
    }
    HKEY hKey;
    // Preserve KEY_WOW64_XXX flags for later use
    ACCESS_MASK wowFlags = m_access & KEY_WOW64_RES;
    bool bRel = CHECKSUCCESS(RegOpenKeyEx(getBaseKey(bRelative), wsTemp.c_str(), 0, KEY_READ | wowFlags, &hKey));
    if (bRel)
    {
        if ((m_currentKey != 0) && bRelative)
        {
            wsTemp = m_currentPath + L"\\" + wsTemp;
        }
        changeKey(hKey, wsTemp);
    } 
    else
    {
        bRel = CHECKSUCCESS(RegOpenKeyEx(getBaseKey(bRelative), wsTemp.c_str(), 0, STANDARD_RIGHTS_READ | KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS | wowFlags, &hKey));
        if (bRel)
        {
            m_access = STANDARD_RIGHTS_READ | KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS | wowFlags;
            if ((m_currentKey != 0) && bRelative)
            {
                wsTemp = m_currentPath + L"\\" + wsTemp;
            }
            changeKey(hKey, wsTemp);
        } 
        else
        {
            bRel = CHECKSUCCESS(RegOpenKeyEx(getBaseKey(bRelative), wsTemp.c_str(), 0, KEY_QUERY_VALUE | wowFlags, &hKey));
            if (bRel)
            {
                m_access = KEY_QUERY_VALUE | wowFlags;
                if ((m_currentKey != 0) && bRelative)
                {
                    wsTemp = m_currentPath + L"\\" + wsTemp;
                }
                changeKey(hKey, wsTemp);
            }
        }
    }

    return bRel;
}

void GRegister::closeKey()
{
    if (m_currentKey != 0)
    {
        RegCloseKey(m_currentKey);
        m_currentKey = 0;
        m_currentPath = L"";
    }
}

bool GRegister::keyExist(const wstring &wsKey)
{
    return getKey(wsKey) != 0;
}

bool GRegister::valueExist(const wstring &wsValue)
{
    GRegDataInfo regDataInfo;
    
    return getDataInfo(wsValue, regDataInfo);
}

bool GRegister::keyEmpty(const wstring &wsKey)
{
    auto regHasNoSubKeys = [&]()
    {
        bool bResult = false;
        if (openKeyReadOnly(wsKey))
        {
			bResult = getKeyNames().empty();
            closeKey();
        }
        return bResult;
    };

    auto regHasNoValues = [&]()
    {
        bool bResult = false;
        if (openKeyReadOnly(wsKey))
        {
            vector<wstring> oValueNames = getValueNames();
            bResult = (oValueNames.empty()) ||
                ((oValueNames.size() == 1) && (oValueNames.at(0).empty()) && (readString(L"").empty())); // 这后面条件翻译自delphi
            closeKey();
        }
        return bResult;
    };

    return regHasNoSubKeys() && regHasNoValues();
}

std::wstring GRegister::readString(const wstring &wsFullKey)
{
    int nLen = getDataSize(wsFullKey);
    if (nLen <= 0)
    {
        return L"";
    }

    DWORD dwType = REG_NONE;
    wchar_t szString[STRINGCAPACITY] = { 0 };
    memset(szString, 0, STRINGCAPACITY * sizeof(wchar_t));

    if (CHECKSUCCESS(RegQueryValueEx(m_currentKey, wsFullKey.c_str(), NULL, &dwType, (BYTE *)szString, (LPDWORD)&nLen)))
    {
        /* 查询成功 */
        return (dataTypeToRegData(dwType) == rdtString ? szString : L"");
    }
    /* 查询失败 */
    return L"";
}

std::wstring GRegister::readExpandString(const wstring &wsFullKey)
{
    int nLen = getDataSize(wsFullKey);
    if (nLen <= 0)
    {
        return L"";
    }

    DWORD dwType = REG_NONE;
    wchar_t szString[STRINGCAPACITY] = { 0 };
    memset(szString, 0, STRINGCAPACITY * sizeof(wchar_t));

    if (CHECKSUCCESS(RegQueryValueEx(m_currentKey, wsFullKey.c_str(), NULL, &dwType, (BYTE *)szString, (LPDWORD)&nLen)))
    {
        /* 查询成功 */
        return (dataTypeToRegData(dwType) == rdtExpandString ? szString : L"");
    }
    /* 查询失败 */
    return L"";
}

int GRegister::readInt(const wstring &wsFullKey)
{
    DWORD dwType = REG_NONE;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwDest;

    if (CHECKSUCCESS(RegQueryValueEx(m_currentKey, wsFullKey.c_str(), NULL, &dwType, (BYTE *)&dwDest, &dwSize)))
    {
        /* 查询成功 */
        return (dataTypeToRegData(dwType) == rdtInt ? (int)dwDest : -1);
    }

    return -1;
}

bool GRegister::readBool(const wstring &wsFullKey)
{
	//在Delphi版本里，如果要查询的键值不存，会抛异常，这里是返回-1来处理，如果键值不存的话，默认返回false。
	int nValue = readInt(wsFullKey);
	if (nValue < 0)
	{
		return false;
	}
	else
		return nValue != 0;;
}

void GRegister::writeString(const wstring &wsFullKey, const wstring &wsValue)
{
    RegSetValueEx(m_currentKey, wsFullKey.c_str(), 0L, REG_SZ, (const BYTE *)wsValue.c_str(), (DWORD)(wcslen(wsValue.c_str()) + 1) * sizeof(wchar_t));
}

void GRegister::writeExpandString(const wstring &wsFullKey, const wstring &wsValue)
{
    RegSetValueEx(m_currentKey, wsFullKey.c_str(), 0L, REG_EXPAND_SZ, (const BYTE *)wsValue.c_str(), (DWORD)(wcslen(wsValue.c_str()) + 1) * sizeof(wchar_t));
}

void GRegister::writeInt(const wstring &wsFullKey, DWORD nValue)
{
    RegSetValueEx(m_currentKey, wsFullKey.c_str(), 0L, REG_DWORD, (const BYTE *)&nValue, sizeof(DWORD));
}

void GRegister::writeBool(const wstring &wsFullKey, const bool bValue)
{
    int nVal = bValue ? 1 : 0;
    writeInt(wsFullKey, nVal);
}

HKEY GRegister::rootKey() const
{
    return m_rootKey;
}

void GRegister::setRootKey(const HKEY &rootKey)
{
    m_rootKey = rootKey;
}

GRegDataType GRegister::getDataType(const wstring &name)
{
    GRegDataInfo oInfo;

    if (getDataInfo(name, oInfo))
    {
        return oInfo.regData;
    }
    else
    {
        return rdtUnknown;
    }
}

vector<wstring> GRegister::getKeyNames()
{
    vector<wstring> oKeyNames;
    GRegKeyInfo oKeyInfo;
    if (getKeyInfo(oKeyInfo))
    {
		wstring strKey = L"";
        strKey.assign(oKeyInfo.maxSubKeyLen + 1, NULL);
		DWORD dwLen = 0;
        for (DWORD index = 0; index < oKeyInfo.numSubKeys; ++index)
        {
            dwLen = oKeyInfo.maxSubKeyLen + 1;
            RegEnumKeyEx(m_currentKey, index, (wchar_t *)strKey.c_str(), &dwLen, NULL, NULL, NULL, NULL);
            oKeyNames.push_back(strKey);
        }
    }
    return oKeyNames;
}

vector<wstring> GRegister::getValueNames()
{
    vector<wstring> oValueNames;
    GRegKeyInfo oKeyInfo;
    if (getKeyInfo(oKeyInfo))
    {
		wstring strValue;
        strValue.assign(oKeyInfo.maxValueLen + 1, NULL);
		DWORD dwLen = 0;
        for (DWORD index = 0; index < oKeyInfo.numValues; ++index)
        {
            dwLen = oKeyInfo.maxValueLen + 1;
            RegEnumValue(m_currentKey, index, (wchar_t *)strValue.c_str(), &dwLen, NULL, NULL, NULL, NULL);
            oValueNames.push_back(strValue);
        }
    }
    return oValueNames;
}

bool GRegister::getKeyInfo(GRegKeyInfo &keyInfo)
{
    memset(&keyInfo, 0, sizeof(GRegKeyInfo));

    bool bRel = CHECKSUCCESS(RegQueryInfoKey(m_currentKey, NULL, NULL, NULL, &keyInfo.numSubKeys, &keyInfo.maxSubKeyLen, NULL,
        &keyInfo.numValues, &keyInfo.maxValueLen, &keyInfo.maxDataLen, NULL, keyInfo.FileTime));

    /*if (SysLocale.FarEast and(osvi.dwPlatformId = VER_PLATFORM_WIN32_NT))
    {
    keyInfo.maxSubKeyLen += keyInfo.maxSubKeyLen;
    keyInfo.MaxValueLen += keyInfo.MaxValueLen;
    }*/
    return bRel;
}

HKEY GRegister::getBaseKey(const bool relative)
{
    if ((m_currentKey == 0) || !relative)
    {
        return m_rootKey;
    }
    else
    {
        return m_currentKey;
    }
}

HKEY GRegister::getKey(const wstring &wsKey)
{
    wstring wsTemp = wsKey;
    bool nRelative = isRelative(wsTemp);
    if (!nRelative)
    {
        wsTemp = wsTemp.substr(1);
    }
    HKEY hKey;
    ACCESS_MASK oldAccess = m_access;
	GSCOPEEXIT
	{
		m_access = oldAccess;
	};
    m_access = STANDARD_RIGHTS_READ | KEY_QUERY_VALUE |
		KEY_ENUMERATE_SUB_KEYS | (oldAccess & KEY_WOW64_RES);

	RegOpenKeyEx(getBaseKey(nRelative), wsTemp.c_str(), 0L, m_access, &hKey);
    return hKey;
}

void GRegister::changeKey(HKEY hKey, const wstring wsPath)
{
    closeKey();
    m_currentKey = hKey;
    m_currentPath = wsPath;
}

bool GRegister::getDataInfo(const wstring &wsValue, GRegDataInfo &dataInfo)
{
    memset(&dataInfo, NULL, sizeof(dataInfo));
    DWORD dataType;
    bool bRel = CHECKSUCCESS(RegQueryValueEx(m_currentKey, wsValue.c_str(), NULL, &dataType, NULL, (LPDWORD)&dataInfo.dataSize));
    dataInfo.regData = dataTypeToRegData(dataType);
    return bRel;
}

int GRegister::getDataSize(const wstring &wstr)
{
    GRegDataInfo regDataInfo;
    if (getDataInfo(wstr, regDataInfo))
    {
        return regDataInfo.dataSize;
    }
    else
    {
        return 0;
    }
}
