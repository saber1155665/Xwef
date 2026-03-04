// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include "query_proc_name.h"

#include <Windows.h>
#include <detours.h>



HMODULE GetSelfModuleHandle()
{
	//Windows内存与结构管理结构体
	MEMORY_BASIC_INFORMATION mbi;
	//查询内存的状态
	return ((::VirtualQuery(GetSelfModuleHandle, &mbi, sizeof(mbi)) != 0) ? (HMODULE)mbi.AllocationBase : NULL);
}

// 等效实现PathRemoveFileSpec（支持ANSI/Unicode，兼容Windows路径规则）
BOOL MyPathRemoveFileSpec(_Inout_ LPTSTR pszPath)
{
    // 入参校验：空指针/空字符串直接返回失败
    if (pszPath == NULL || *pszPath == _T('\0'))
    {
        return FALSE;
    }

    LPTSTR pszEnd = pszPath + _tcslen(pszPath) - 1; // 指向字符串最后一个字符

    // 步骤1：跳过路径末尾的所有路径分隔符（\ /）
    while (pszEnd >= pszPath)
    {
        if (*pszEnd != _T('\\') && *pszEnd != _T('/'))
        {
            break; // 找到非分隔符字符，停止跳过
        }
        pszEnd--;
    }

    // 步骤2：如果只剩根路径（如C:\、\\server\share\），直接返回（不修改）
    // 先判断是否是UNC路径（\\开头）
    BOOL bIsUNC = (pszPath[0] == _T('\\') && pszPath[1] == _T('\\'));
    if (bIsUNC)
    {
        // UNC路径格式：\\server\share\... → 至少要保留\\server\share\
		//
        LPTSTR pszShareEnd = pszEnd;
        int nSeparatorCount = 0;
        // 统计UNC路径中有效分隔符数量（找到share后的分隔符）
        while (pszShareEnd >= pszPath && nSeparatorCount < 3)
        {
            if (*pszShareEnd == _T('\\') || *pszShareEnd == _T('/'))
            {
                nSeparatorCount++;
            }
            pszShareEnd--;
        }
        // 如果已经是\\server\share\层级，不修改
        if (nSeparatorCount >= 3)
        {
            return TRUE;
        }
    }
    else
    {
        // 本地路径：判断是否是根目录（如C:\、D:\）
        if (pszEnd == pszPath + 1 && pszPath[1] == _T(':') && 
            (pszPath[2] == _T('\\') || pszPath[2] == _T('/') || pszPath[2] == _T('\0')))
        {
            return TRUE; // 如C:、C:\，不修改
        }
    }

    // 步骤3：从后往前找最后一个路径分隔符
    while (pszEnd >= pszPath)
    {
        if (*pszEnd == _T('\\') || *pszEnd == _T('/'))
        {
            // 找到分隔符，截断字符串（保留分隔符）
            *(pszEnd + 1) = _T('\0');
            return TRUE;
        }
        pszEnd--;
    }

    // 步骤4：无有效分隔符（如只有文件名"file.txt"），清空字符串
    *pszPath = _T('\0');
    return FALSE;
}




typedef BOOL (WINAPI *PFuncGetVersionExW)(
    __inout LPOSVERSIONINFOW lpVersionInformation
    );

PFuncGetVersionExW g_pFuncGetVersionExW = NULL;


BOOL WINAPI HOOKGetVersionExW(
    __inout LPOSVERSIONINFOW lpVersionInformation
    ) {
		BOOL bRst = FALSE;
		if (g_pFuncGetVersionExW) {
            if (NULL == lpVersionInformation || IsBadWritePtr(lpVersionInformation, sizeof(OSVERSIONINFOW))) {
                return FALSE;
            }
			bRst = g_pFuncGetVersionExW(lpVersionInformation);
            if (!bRst) {
                return FALSE;
            }
			WCHAR wchBUf[MAX_PATH] = { 0 };
			GetModuleFileName(GetSelfModuleHandle(),wchBUf,  MAX_PATH);
			MyPathRemoveFileSpec(wchBUf);
			std::wstring wstr = wchBUf;
			wstr += L"XWef.ini";

			// 步骤3：读取INI的system.build键值
			TCHAR szBuildStr[32] = { 0 };
			GetPrivateProfileString(
				_T("system"),    // section名
				_T("build"),     // key名
				_T("10248"),        // 默认值（读取失败时返回）
				szBuildStr,      // 存储结果的缓冲区
				_countof(szBuildStr),  // 缓冲区大小
				wstr.c_str()    // INI完整路径
			);

			// 步骤4：字符串转int + 合法性校验
			DWORD dwBuild =  static_cast<unsigned int >(_ttoi(szBuildStr));
			lpVersionInformation->dwBuildNumber = dwBuild;
		}
		return bRst;
}


typedef DWORD (WINAPI *PFuncGetVersion) (
    VOID
    );
PFuncGetVersion g_pFuncGetVersion = NULL;

DWORD WINAPI HOOKGetVersion (
    VOID
    ) {
		DWORD dw = 0;
		if (g_pFuncGetVersion) {
			dw = g_pFuncGetVersion();
			WCHAR wchBUf[MAX_PATH] = { 0 };
			GetModuleFileName(GetSelfModuleHandle(),wchBUf,  MAX_PATH);
			MyPathRemoveFileSpec(wchBUf);
			std::wstring wstr = wchBUf;
			wstr += L"XWef.ini";

			// 步骤3：读取INI的system.build键值
			TCHAR szBuildStr[32] = { 0 };
			GetPrivateProfileString(
				_T("system"),    // section名
				_T("build"),     // key名
				_T("10248"),        // 默认值（读取失败时返回）
				szBuildStr,      // 存储结果的缓冲区
				_countof(szBuildStr),  // 缓冲区大小
				wstr.c_str()    // INI完整路径
			);
			// 步骤4：字符串转int + 合法性校验
			DWORD dwBuild =  static_cast<unsigned int >(_ttoi(szBuildStr));
			dwBuild <<= 16;

			dw = (dw & 0x0000FFFF) + dwBuild;
		}
		return dw;
 }

void WINAPI Func() {
			//wchar_t filePath[MAX_PATH];
			//if (GetModuleFileNameW(NULL, filePath, MAX_PATH) != 0) {
			//// 提取文件名作为进程名
			//wchar_t* fileName = mywcsrchr(filePath, '\\');
			//if (fileName != NULL && ( 
			//			(0 == mywcsicmp(fileName + 1, L"WmiPrvSE.exe"))
			//			|| (0 == mywcsicmp(fileName + 1, L"cmd.exe"))
			//	)){
					HMODULE hMod = LoadLibrary(L"Kernelbase.dll");
					g_pFuncGetVersionExW = (PFuncGetVersionExW)GetProcAddress(hMod, "GetVersionExW"); 
					g_pFuncGetVersion =  (PFuncGetVersion)GetProcAddress(hMod, "GetVersion");
					if (NULL == g_pFuncGetVersionExW || NULL == g_pFuncGetVersion) {
						return;
					}
					DetourTransactionBegin();
					DetourUpdateThread(GetCurrentThread());
					DetourAttach(&(PVOID&)g_pFuncGetVersionExW, HOOKGetVersionExW);
					DetourAttach(&(PVOID&)g_pFuncGetVersion, HOOKGetVersion);
					DetourTransactionCommit();
					// 创建线程（立即返回，线程稍后执行，Loader Lock已释放）
					//CreateThread(NULL, 0, SafeRegAccessThread, NULL, 0, NULL);
			//	}
			//}
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		{
			DisableThreadLibraryCalls(hModule);
            Func();
		}
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
