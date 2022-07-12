#include "GJGHelper.h"

#include <atlbase.h>
#include <tchar.h>
#include <atlstr.h>
#include <xstring>


void setRegValue(HKEY& hKey);

#if (defined _WIN32 || defined _WIN64)
// 安全的取得真实系统信息   
void SafeGetNativeSystemInfo(__out LPSYSTEM_INFO lpSystemInfo)
{
	if (NULL == lpSystemInfo)
		return;
	typedef VOID(WINAPI *LPFN_GetNativeSystemInfo)(LPSYSTEM_INFO lpSystemInfo);
	LPFN_GetNativeSystemInfo nsInfo = (LPFN_GetNativeSystemInfo)GetProcAddress(GetModuleHandle(L"kernel32"), "GetNativeSystemInfo");
	if (NULL != nsInfo)
	{
		nsInfo(lpSystemInfo);
	}
	else
	{
		GetSystemInfo(lpSystemInfo);
	}
}
#endif

// 获取操作系统位数   
int GetSystemBits()
{
#if (defined _WIN32 || defined _WIN64)
	SYSTEM_INFO si;
	SafeGetNativeSystemInfo(&si);
	if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
		si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
	{
		return 64;
	}
	return 32;
#else
	return 64;
#endif
}

void setProductReg(int argc, char *argv[])
{
	if (argc < 3)
	{
		return;
	}
	//获取软件位数
	if (0 != strcmp(argv[1], "-SoftBits"))
	{
		return;
	}

	int nSoftBits = atoi(argv[2]);
	//int nSoftBits = 64;
	//获取系统位数
	int nSystemBits = GetSystemBits();
	//综合判断写入注册表位置
	HKEY hKey;
	std::wstring subAppPathsKey = L"SOFTWARE\\GrandSoft\\GJG\\1.0";
	if (nSystemBits == 64 && nSoftBits == 32)    //64位系统下，兼容32位软件的申请
	{
		subAppPathsKey = L"SOFTWARE\\WOW6432Node\\GrandSoft\\GJG\\1.0";
	}


	std::wstring subVersion = L"Version";
	std::wstring subPath = L"Path";

	//查询到了，打开；否则创建一个
	DWORD dw;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, subAppPathsKey.c_str(), 0L, NULL, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS | KEY_WOW64_64KEY, NULL, &hKey, &dw) == ERROR_SUCCESS)
	{
		setRegValue(hKey);
	}

	RegCloseKey(hKey);


}

void setRegValue(HKEY& hKey)
{
	//Window-API 修改注册表
	int nBits = (sizeof(void*) == 4 ? 32 : 64);
	std::wstring subVersion = L"Version";
	//std::wstring subBits = L"Bits";
	std::wstring subPath = L"Path";

	//1.写入Verison项
	CString data = ("1.0.0.0");
	int nLength = data.GetLength() * 2;
	LPBYTE pByte = new BYTE[nLength];
	memcpy(pByte, (VOID*)LPCTSTR(data), nLength);
	LONG nRes1 = RegSetValueEx(hKey, subVersion.c_str(), 0, REG_SZ, (BYTE*)pByte, nLength);

	//2.写入Path项
	CString pathData = ("C:\\Program Files\\Grandsoft\\GJG\\1.0");
	nLength = pathData.GetLength() * 2;
	LPBYTE pPathByte = new BYTE[nLength];
	memcpy(pPathByte, (VOID*)LPCTSTR(pathData), nLength);
	nRes1 = RegSetValueEx(hKey, subPath.c_str(), 0, REG_SZ, (BYTE*)pPathByte, nLength);

}


