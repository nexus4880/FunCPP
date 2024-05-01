#pragma once

#include <Windows.h>
#include <string>
#include <optional>
#include <psapi.h>

HANDLE g_hGame = NULL;
HANDLE g_hMilkRen = NULL;

bool MemReadRaw(const void* address, void* buffer, size_t size, HANDLE handle = NULL) {
    if (handle == NULL) {
        handle = g_hGame;
    }

    return ReadProcessMemory(handle, address, buffer, size, NULL) == TRUE;
}

template<typename T>
T MemRead(uintptr_t address, HANDLE handle = NULL) {
    T t{};
    MemReadRaw((void*)address, &t, sizeof(T), handle);

    return t;
}

std::optional<std::wstring> ReadFString(uintptr_t address) {
    const auto nameAddress = MemRead<uintptr_t>(address);
    if (!nameAddress) {
        return std::nullopt;
    }

    const auto stringLength = MemRead<int>(address + 0x8);
    if (stringLength <= 0 || stringLength >= 255) {
        return std::nullopt;
    }

    std::wstring result(stringLength, L'\0');

    if (!MemReadRaw((void*)nameAddress, result.data(), stringLength * sizeof(wchar_t))) {
        return std::nullopt;
    }

    return result;
}

uintptr_t GetBaseAddress(std::wstring moduleName, HANDLE handle) {
    HMODULE hMods[1024];
    DWORD cbNeeded;
    unsigned int i;

    if (EnumProcessModules(handle, hMods, sizeof(hMods), &cbNeeded))
    {
        for (i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
        {
            TCHAR szModName[MAX_PATH];
            if (GetModuleFileNameEx(handle, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR)))
            {
                std::wstring wstrModName = szModName;
                std::wstring wstrModContain = moduleName;
                if (wstrModName.find(wstrModContain) != std::wstring::npos)
                {
                    return (uintptr_t)hMods[i];
                }
            }
        }
    }

    return 0;
}