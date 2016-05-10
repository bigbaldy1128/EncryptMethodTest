#include "windows.h"
#include <tchar.h>
#include "corinfo.h"
#include "corjit.h"


HINSTANCE hInstance;

extern "C" __declspec(dllexport) void HookJIT();


//BOOL APIENTRY DllMain( HMODULE hModule,
//					   DWORD  dwReason,
//					   LPVOID lpReserved
//					  )
//{
//	hInstance = (HINSTANCE) hModule;
//
//	HookJIT();
//
//	return TRUE;
//}

BOOL bHooked = FALSE;

ULONG_PTR *(__stdcall *p_getJit)();
typedef int (__stdcall *compileMethod_def)(ULONG_PTR classthis, ICorJitInfo *comp, 
										   CORINFO_METHOD_INFO *info, unsigned flags,         
										   BYTE **nativeEntry, ULONG  *nativeSizeOfCode);
struct JIT
{
	compileMethod_def compileMethod;
};

compileMethod_def compileMethod;

int __stdcall my_compileMethod(ULONG_PTR classthis, ICorJitInfo *comp,
							   CORINFO_METHOD_INFO *info, unsigned flags,         
							   BYTE **nativeEntry, ULONG  *nativeSizeOfCode);

extern "C" __declspec(dllexport) void HookJIT()
{
	if (bHooked) return;

	LoadLibrary(_T("mscoree.dll"));

	HMODULE hJitMod = LoadLibrary(_T("clrjit.dll"));

	if (!hJitMod)
		return;

	p_getJit = (ULONG_PTR *(__stdcall *)()) GetProcAddress(hJitMod, "getJit");

	if (p_getJit)
	{
		JIT *pJit = (JIT *) *((ULONG_PTR *) p_getJit());

		if (pJit)
		{
			DWORD OldProtect;
			VirtualProtect(pJit, sizeof (ULONG_PTR), PAGE_READWRITE, &OldProtect);
			compileMethod =  pJit->compileMethod;
			pJit->compileMethod = &my_compileMethod;
			VirtualProtect(pJit, sizeof (ULONG_PTR), OldProtect, &OldProtect);
			bHooked = TRUE;
		}
	}
}

BYTE testFunCode[0x9] =
{
	0x00,
	0x03,
	0x04,
	0x58,
	0x0A,
	0x2B,0x00,
	0x06,
	0x2A
};

int __stdcall my_compileMethod(ULONG_PTR classthis, ICorJitInfo *comp,
							   CORINFO_METHOD_INFO *info, unsigned flags,         
							   BYTE **nativeEntry, ULONG  *nativeSizeOfCode)
{
   const char *szMethodName = NULL;
   const char *szClassName = NULL;
   szMethodName = comp->getMethodName(info->ftn, &szClassName);
   BYTE* old = info->ILCode;
    if (strcmp(szMethodName, "Test") == 0)
	{		
		info->ILCode= testFunCode;
	}

	int nRet = compileMethod(classthis, comp, info, flags, nativeEntry, nativeSizeOfCode);
	info->ILCode = old;
	return nRet;
}
