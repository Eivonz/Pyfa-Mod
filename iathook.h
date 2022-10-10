// This file contains code from
// https://github.com/stevemk14ebr/PolyHook_2_0/blob/master/sources/IatHook.cpp
// which is licensed under the MIT License.
// See PolyHook_2_0-LICENSE for more information.

#pragma once


#include <climits>
#include <cstdint>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <CommCtrl.h>
#include <Uxtheme.h>
#include <WindowsX.h>
#include <Vssym32.h>

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Uxtheme.lib")


template <typename T, typename T1, typename T2>
constexpr T RVA2VA(T1 base, T2 rva)
{
	return reinterpret_cast<T>(reinterpret_cast<ULONG_PTR>(base) + rva);
}

template <typename T>
constexpr T DataDirectoryFromModuleBase(void* moduleBase, size_t entryID)
{
	auto dosHdr = reinterpret_cast<PIMAGE_DOS_HEADER>(moduleBase);
	auto ntHdr = RVA2VA<PIMAGE_NT_HEADERS>(moduleBase, dosHdr->e_lfanew);
	auto dataDir = ntHdr->OptionalHeader.DataDirectory;
	return RVA2VA<T>(moduleBase, dataDir[entryID].VirtualAddress);
}

PIMAGE_THUNK_DATA FindAddressByName(void* moduleBase, PIMAGE_THUNK_DATA impName, PIMAGE_THUNK_DATA impAddr, const char* funcName)
{
	for (; impName->u1.Ordinal; ++impName, ++impAddr)
	{
		if (IMAGE_SNAP_BY_ORDINAL(impName->u1.Ordinal))
			continue;

		auto import = RVA2VA<PIMAGE_IMPORT_BY_NAME>(moduleBase, impName->u1.AddressOfData);
		if (strcmp(import->Name, funcName) != 0)
			continue;
		return impAddr;
	}
	return nullptr;
}

PIMAGE_THUNK_DATA FindAddressByOrdinal(void* moduleBase, PIMAGE_THUNK_DATA impName, PIMAGE_THUNK_DATA impAddr, uint16_t ordinal)
{
	for (; impName->u1.Ordinal; ++impName, ++impAddr)
	{
		if (IMAGE_SNAP_BY_ORDINAL(impName->u1.Ordinal) && IMAGE_ORDINAL(impName->u1.Ordinal) == ordinal)
			return impAddr;
	}
	return nullptr;
}

PIMAGE_THUNK_DATA FindIatThunkInModule(void* moduleBase, const char* dllName, const char* funcName)
{
	auto imports = DataDirectoryFromModuleBase<PIMAGE_IMPORT_DESCRIPTOR>(moduleBase, IMAGE_DIRECTORY_ENTRY_IMPORT);
	for (; imports->Name; ++imports)
	{
		if (_stricmp(RVA2VA<LPCSTR>(moduleBase, imports->Name), dllName) != 0)
			continue;

		auto origThunk = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->OriginalFirstThunk);
		auto thunk = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->FirstThunk);
		return FindAddressByName(moduleBase, origThunk, thunk, funcName);
	}
	return nullptr;
}

PIMAGE_THUNK_DATA FindDelayLoadThunkInModule(void* moduleBase, const char* dllName, const char* funcName)
{
	auto imports = DataDirectoryFromModuleBase<PIMAGE_DELAYLOAD_DESCRIPTOR>(moduleBase, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
	for (; imports->DllNameRVA; ++imports)
	{
		if (_stricmp(RVA2VA<LPCSTR>(moduleBase, imports->DllNameRVA), dllName) != 0)
			continue;

		auto impName = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportNameTableRVA);
		auto impAddr = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportAddressTableRVA);
		return FindAddressByName(moduleBase, impName, impAddr, funcName);
	}
	return nullptr;
}

PIMAGE_THUNK_DATA FindDelayLoadThunkInModule(void* moduleBase, const char* dllName, uint16_t ordinal)
{
	auto imports = DataDirectoryFromModuleBase<PIMAGE_DELAYLOAD_DESCRIPTOR>(moduleBase, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
	for (; imports->DllNameRVA; ++imports)
	{
		if (_stricmp(RVA2VA<LPCSTR>(moduleBase, imports->DllNameRVA), dllName) != 0)
			continue;

		auto impName = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportNameTableRVA);
		auto impAddr = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportAddressTableRVA);
		return FindAddressByOrdinal(moduleBase, impName, impAddr, ordinal);
	}
	return nullptr;
}





/*
    Patching the IAT of a specific library, replacing a function address with another function address

    hModule         :   Module handle of targeted DLL module to patch, fx. HMODULE module = GetModuleHandle(L"wxmsw30u_core_vc140_x64.dll");
                        If null, iterate over every module looking for the specific import
    lpModuleName    :   Name of imported module, fx. "user32.dll"
    lpProcName      :   Name of function to hijack
    ProxyFunc       :   Function address to overwrite IAT with

*/
BOOL PatchIAT(HMODULE hModule, const char* lpModuleName, const char* lpProcName, PROC ProxyFunc) {

    PIMAGE_DOS_HEADER dosHeaders = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((DWORD_PTR)hModule + dosHeaders->e_lfanew);

    PIMAGE_IMPORT_DESCRIPTOR importDescriptor = NULL;
    IMAGE_DATA_DIRECTORY importsDirectory = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    importDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(importsDirectory.VirtualAddress + (DWORD_PTR)hModule);
    LPCSTR libraryName = NULL;
    HMODULE library = NULL;
    PIMAGE_IMPORT_BY_NAME functionName = NULL;

    BOOL found = false;
    while (importDescriptor->Characteristics && importDescriptor->Name)
    {
        PSTR importName = (PSTR)((PBYTE)hModule + importDescriptor->Name);
        if (_stricmp(importName, lpModuleName) == 0) {
            found = true;
            break;
        }
        importDescriptor++;
    }
    if (!found) {
        //Log() << "Couldnt not find lpModuleName";
        return false;
    }

    // Retrieve address of function for later comparison
    PROC baseGetProcAddress = (PROC)GetProcAddress(GetModuleHandleA(lpModuleName), lpProcName);

    // From the lpModuleName import descriptor, go over its IAT thunks to
    // find the one used by the rest of the code to call GetProcAddress
    PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA)((PBYTE)hModule + importDescriptor->FirstThunk);
    while (thunk->u1.Function) {
        PROC* funcStorage = (PROC*)&thunk->u1.Function;
        // Found it, now let's patch it
        if (*funcStorage == baseGetProcAddress) {
            // Get the memory page where the info is stored
            MEMORY_BASIC_INFORMATION mbi;
            VirtualQuery(funcStorage, &mbi, sizeof(MEMORY_BASIC_INFORMATION));

            // Try to change the page to be writable if it's not already
            if (!VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_READWRITE, &mbi.Protect)) {
                //Log() << "Failed during VirtualProtect";
                return FALSE;
            }

            // Update IAT to use the address of proxy function
            *funcStorage = (PROC)ProxyFunc;

            // Restore the old flag on the page
            DWORD dwOldProtect;
            VirtualProtect(mbi.BaseAddress, mbi.RegionSize, mbi.Protect, &dwOldProtect);

            // Profit
            return true;
        }
        thunk++;
    }
    return true;
}

/*
    // Not required since we are now using detours instead
    HMODULE module = GetModuleHandle(L"wxmsw30u_core_vc140_x64.dll");
    if (module == NULL) {
        return false;
    }
    if (!PatchIAT(module, "user32.dll", "GetSysColor", (PROC)_GetSysColor)) {
        MsgBox(L"Error patching IAT for GetSysColor!");
        return false;
    }
*/