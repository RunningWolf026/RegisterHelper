/*
@brief:对注册表操作的封装类
*/
#ifndef GREGISTER_H
#define GREGISTER_H

#include <string>
#include <vector>
#include <Windows.h>

using namespace std;

struct GRegKeyInfo
{
    DWORD numSubKeys;
    DWORD maxSubKeyLen;
    DWORD numValues;
    DWORD maxValueLen;
    DWORD maxDataLen;
    PFILETIME FileTime;
};

enum GRegDataType
{
    rdtUnknown,
    rdtString,
    rdtExpandString,
    rdtInt,
    rdtBinary
};

struct GRegDataInfo
{
    GRegDataType regData;
    int dataSize;
};

class GRegister
{
public:
    GRegister(const HKEY &hKey = HKEY_CURRENT_USER, const ACCESS_MASK &hAccess = KEY_ALL_ACCESS);
    ~GRegister();

public:
    static HKEY strToRootKey(const wstring &str);
    static wstring rootKeyToStr(const HKEY &hKey);
    bool createKey(const wstring &wsSubKey);
    bool deleteKey(const wstring &wsSubKey);
    bool deleteValue(const wstring &wsValue);

    bool openKey(const wstring &wsSubKey, const bool bCanCreate = false);
    bool openKeyReadOnly(const wstring &wsSubKey);
    void closeKey();

    bool keyExist(const wstring &wsKey);
    bool valueExist(const wstring &wsValue);
    bool keyEmpty(const wstring &wsKey);

    wstring readString(const wstring &wsFullKey);
    wstring readExpandString(const wstring &wsFullKey);
    int readInt(const wstring &wsFullKey);
    bool readBool(const wstring &wsFullKey);

    void writeString(const wstring &wsFullKey, const wstring &wsValue);
    void writeExpandString(const wstring &wsFullKey, const wstring &wsValue);
    void writeInt(const wstring &wsFullKey, DWORD nValue);
    void writeBool(const wstring &wsFullKey, const bool bValue);

    HKEY rootKey() const;
    void setRootKey(const HKEY &rootKey);

    ACCESS_MASK access() const { return m_access; }
    void setAccess(const ACCESS_MASK &access) { m_access = access; }

	GRegDataType getDataType(const wstring &name);

    vector<wstring> getKeyNames();

    vector<wstring> getValueNames();

private:
    HKEY getBaseKey(const bool relative);
    HKEY getKey(const wstring &wsKey);

    HKEY currentKey() const { return m_currentKey; }
    void setCurrentKey(const HKEY &key) { m_currentKey = key; }

    void changeKey(HKEY hKey, const wstring wsPath);

    bool getKeyInfo(GRegKeyInfo &keyInfo);
    bool getDataInfo(const wstring &wsValue, GRegDataInfo &dataInfo);
    int getDataSize(const wstring &wstr);
    
private:
    HKEY m_rootKey;
    HKEY m_currentKey;
    wstring m_currentPath;
    ACCESS_MASK m_access;
};

#endif