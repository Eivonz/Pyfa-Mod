#pragma once
#include <Windows.h>
#include "logging.h"

HMODULE LoadOrignalLibrary(LPCWCHAR TARGETDLL) {
	TCHAR tzPath[MAX_PATH];
	HMODULE hTARGETDLL;

	if (GetSystemDirectory(tzPath, MAX_PATH) == 0) {
		Log() << "Error in GetSystemDirectory";
		return NULL;
	}

	lstrcat(tzPath, L"\\");
	lstrcat(tzPath, TARGETDLL);

	hTARGETDLL = LoadLibrary(tzPath);
	if (hTARGETDLL == NULL) {
		Log() << "Error loading original DLL : " << tzPath;
		return NULL;
	}

	return hTARGETDLL;
}



namespace oleacc {
//	LPCWCHAR TARGETDLL = L"oleacc.dll";
	HMODULE hTARGETDLL;

	CONST int imports = 28;

	extern "C" {
		FARPROC g_AccGetRunningUtilityState;
		FARPROC g_AccNotifyTouchInteraction;
		FARPROC g_AccSetRunningUtilityState;
		FARPROC g_AccessibleChildren;
		FARPROC g_AccessibleObjectFromEvent;
		FARPROC g_AccessibleObjectFromPoint;
		FARPROC g_AccessibleObjectFromWindow;
		FARPROC g_AccessibleObjectFromWindowTimeout;
		FARPROC g_CreateStdAccessibleObject;
		FARPROC g_CreateStdAccessibleProxyA;
		FARPROC g_CreateStdAccessibleProxyW;
		FARPROC g_DllCanUnloadNow;
		FARPROC g_DllGetClassObject;
		FARPROC g_DllRegisterServer;
		FARPROC g_DllUnregisterServer;
		FARPROC g_GetOleaccVersionInfo;
		FARPROC g_GetProcessHandleFromHwnd;
		FARPROC g_GetRoleTextA;
		FARPROC g_GetRoleTextW;
		FARPROC g_GetStateTextA;
		FARPROC g_GetStateTextW;
		FARPROC g_IID_IAccessible;
		FARPROC g_IID_IAccessibleHandler;
		FARPROC g_LIBID_Accessibility;
		FARPROC g_LresultFromObject;
		FARPROC g_ObjectFromLresult;
		FARPROC g_PropMgrClient_LookupProp;
		FARPROC g_WindowFromAccessibleObject;
	}

	bool LoadProcAddresses(LPCWCHAR TARGETDLL) {
		hTARGETDLL = LoadOrignalLibrary(TARGETDLL);
		if (hTARGETDLL == NULL)
			return false;

		g_AccGetRunningUtilityState = GetProcAddress(hTARGETDLL, "AccGetRunningUtilityState");
		g_AccNotifyTouchInteraction = GetProcAddress(hTARGETDLL, "AccNotifyTouchInteraction");
		g_AccSetRunningUtilityState = GetProcAddress(hTARGETDLL, "AccSetRunningUtilityState");
		g_AccessibleChildren = GetProcAddress(hTARGETDLL, "AccessibleChildren");
		g_AccessibleObjectFromEvent = GetProcAddress(hTARGETDLL, "AccessibleObjectFromEvent");
		g_AccessibleObjectFromPoint = GetProcAddress(hTARGETDLL, "AccessibleObjectFromPoint");
		g_AccessibleObjectFromWindow = GetProcAddress(hTARGETDLL, "AccessibleObjectFromWindow");
		g_AccessibleObjectFromWindowTimeout = GetProcAddress(hTARGETDLL, "AccessibleObjectFromWindowTimeout");
		g_CreateStdAccessibleObject = GetProcAddress(hTARGETDLL, "CreateStdAccessibleObject");
		g_CreateStdAccessibleProxyA = GetProcAddress(hTARGETDLL, "CreateStdAccessibleProxyA");
		g_CreateStdAccessibleProxyW = GetProcAddress(hTARGETDLL, "CreateStdAccessibleProxyW");
		g_DllCanUnloadNow = GetProcAddress(hTARGETDLL, "DllCanUnloadNow");
		g_DllGetClassObject = GetProcAddress(hTARGETDLL, "DllGetClassObject");
		g_DllRegisterServer = GetProcAddress(hTARGETDLL, "DllRegisterServer");
		g_DllUnregisterServer = GetProcAddress(hTARGETDLL, "DllUnregisterServer");
		g_GetOleaccVersionInfo = GetProcAddress(hTARGETDLL, "GetOleaccVersionInfo");
		g_GetProcessHandleFromHwnd = GetProcAddress(hTARGETDLL, "GetProcessHandleFromHwnd");
		g_GetRoleTextA = GetProcAddress(hTARGETDLL, "GetRoleTextA");
		g_GetRoleTextW = GetProcAddress(hTARGETDLL, "GetRoleTextW");
		g_GetStateTextA = GetProcAddress(hTARGETDLL, "GetStateTextA");
		g_GetStateTextW = GetProcAddress(hTARGETDLL, "GetStateTextW");
		g_IID_IAccessible = GetProcAddress(hTARGETDLL, "IID_IAccessible");
		g_IID_IAccessibleHandler = GetProcAddress(hTARGETDLL, "IID_IAccessibleHandler");
		g_LIBID_Accessibility = GetProcAddress(hTARGETDLL, "LIBID_Accessibility");
		g_LresultFromObject = GetProcAddress(hTARGETDLL, "LresultFromObject");
		g_ObjectFromLresult = GetProcAddress(hTARGETDLL, "ObjectFromLresult");
		g_PropMgrClient_LookupProp = GetProcAddress(hTARGETDLL, "PropMgrClient_LookupProp");
		g_WindowFromAccessibleObject = GetProcAddress(hTARGETDLL, "WindowFromAccessibleObject");

		return true;
	}
}


namespace version {
	//LPCWCHAR TARGETDLL = L"version.dll";
	HMODULE hTARGETDLL;

	CONST int imports = 28;

	extern "C" {
		FARPROC g_GetFileVersionInfoA;
		FARPROC g_GetFileVersionInfoByHandle;
		FARPROC g_GetFileVersionInfoExA;
		FARPROC g_GetFileVersionInfoExW;
		FARPROC g_GetFileVersionInfoSizeA;
		FARPROC g_GetFileVersionInfoSizeExA;
		FARPROC g_GetFileVersionInfoSizeExW;
		FARPROC g_GetFileVersionInfoSizeW;
		FARPROC g_GetFileVersionInfoW;
		FARPROC g_VerFindFileA;
		FARPROC g_VerFindFileW;
		FARPROC g_VerInstallFileA;
		FARPROC g_VerInstallFileW;
		FARPROC g_VerLanguageNameA;
		FARPROC g_VerLanguageNameW;
		FARPROC g_VerQueryValueA;
		FARPROC g_VerQueryValueW;
	}

	bool LoadProcAddresses(LPCWCHAR TARGETDLL) {
		hTARGETDLL = LoadOrignalLibrary(TARGETDLL);
		if (hTARGETDLL == NULL)
			return false;

		g_GetFileVersionInfoA = GetProcAddress(hTARGETDLL, "GetFileVersionInfoA");
		g_GetFileVersionInfoByHandle = GetProcAddress(hTARGETDLL, "GetFileVersionInfoByHandle");
		g_GetFileVersionInfoExA = GetProcAddress(hTARGETDLL, "GetFileVersionInfoExA");
		g_GetFileVersionInfoExW = GetProcAddress(hTARGETDLL, "GetFileVersionInfoExW");
		g_GetFileVersionInfoSizeA = GetProcAddress(hTARGETDLL, "GetFileVersionInfoSizeA");
		g_GetFileVersionInfoSizeExA = GetProcAddress(hTARGETDLL, "GetFileVersionInfoSizeExA");
		g_GetFileVersionInfoSizeExW = GetProcAddress(hTARGETDLL, "GetFileVersionInfoSizeExW");
		g_GetFileVersionInfoSizeW = GetProcAddress(hTARGETDLL, "GetFileVersionInfoSizeW");
		g_GetFileVersionInfoW = GetProcAddress(hTARGETDLL, "GetFileVersionInfoW");
		g_VerFindFileA = GetProcAddress(hTARGETDLL, "VerFindFileA");
		g_VerFindFileW = GetProcAddress(hTARGETDLL, "VerFindFileW");
		g_VerInstallFileA = GetProcAddress(hTARGETDLL, "VerInstallFileA");
		g_VerInstallFileW = GetProcAddress(hTARGETDLL, "VerInstallFileW");
		g_VerLanguageNameA = GetProcAddress(hTARGETDLL, "VerLanguageNameA");
		g_VerLanguageNameW = GetProcAddress(hTARGETDLL, "VerLanguageNameW");
		g_VerQueryValueA = GetProcAddress(hTARGETDLL, "VerQueryValueA");
		g_VerQueryValueW = GetProcAddress(hTARGETDLL, "VerQueryValueW");

		return true;
	}
}
