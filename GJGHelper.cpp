#include "GJGHelper.h"

#include <atlbase.h>
#include <tchar.h>
#include <atlstr.h>
#include <xstring>


void setRegValue(HKEY& hKey);

#if (defined _WIN32 || defined _WIN64)
// ��ȫ��ȡ����ʵϵͳ��Ϣ   
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

// ��ȡ����ϵͳλ��   
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
	//��ȡ���λ��
	if (0 != strcmp(argv[1], "-SoftBits"))
	{
		return;
	}

	int nSoftBits = atoi(argv[2]);
	//int nSoftBits = 64;
	//��ȡϵͳλ��
	int nSystemBits = GetSystemBits();
	//�ۺ��ж�д��ע���λ��
	HKEY hKey;
	std::wstring subAppPathsKey = L"SOFTWARE\\GrandSoft\\GJG\\1.0";
	if (nSystemBits == 64 && nSoftBits == 32)    //64λϵͳ�£�����32λ���������
	{
		subAppPathsKey = L"SOFTWARE\\WOW6432Node\\GrandSoft\\GJG\\1.0";
	}


	std::wstring subVersion = L"Version";
	std::wstring subPath = L"Path";

	//��ѯ���ˣ��򿪣����򴴽�һ��
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
	//Window-API �޸�ע���
	int nBits = (sizeof(void*) == 4 ? 32 : 64);
	std::wstring subVersion = L"Version";
	//std::wstring subBits = L"Bits";
	std::wstring subPath = L"Path";

	//1.д��Verison��
	CString data = ("1.0.0.0");
	int nLength = data.GetLength() * 2;
	LPBYTE pByte = new BYTE[nLength];
	memcpy(pByte, (VOID*)LPCTSTR(data), nLength);
	LONG nRes1 = RegSetValueEx(hKey, subVersion.c_str(), 0, REG_SZ, (BYTE*)pByte, nLength);

	//2.д��Path��
	CString pathData = ("C:\\Program Files\\Grandsoft\\GJG\\1.0");
	nLength = pathData.GetLength() * 2;
	LPBYTE pPathByte = new BYTE[nLength];
	memcpy(pPathByte, (VOID*)LPCTSTR(pathData), nLength);
	nRes1 = RegSetValueEx(hKey, subPath.c_str(), 0, REG_SZ, (BYTE*)pPathByte, nLength);

}


