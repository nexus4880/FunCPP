#pragma once

#include <string>
#include <optional>
#include <vmmdll.h>
#include <unordered_map>

std::unordered_map<uint32_t, std::string> nameCache;

uintptr_t g_payloadBufferAddress = 0;
uintptr_t g_gameBaseAddress = 0;
VMM_HANDLE g_hVMM = NULL;
VMMDLL_SCATTER_HANDLE g_hScatter = NULL;

DWORD pids[2]{ 0, 0 };

constexpr auto GAME_NAME = "GZWClientSteam-Win64-Shipping.exe";
constexpr auto OVERLAY_NAME = "MilkRen.Client.exe";

constexpr uintptr_t GWORLD_OFFSET = 0x8B49588;
constexpr DWORD MILKREN_BUFFER_OFFSET = 64;

bool MemReadRaw(const void* address, void* buffer, DWORD size, bool overlay = false, bool ignoreCache = false) {
    return ignoreCache ?
        VMMDLL_MemReadEx(g_hVMM, pids[overlay ? 1 : 0], (ULONG64)address, (PBYTE)buffer, size, nullptr, VMMDLL_FLAG_NOCACHE) :
        VMMDLL_MemRead(g_hVMM, pids[overlay ? 1 : 0], (ULONG64)address, (PBYTE)buffer, size)
        == TRUE;
}

bool MemWriteRaw(void* address, void* buffer, DWORD size, bool overlay = false) {
    return VMMDLL_MemWrite(g_hVMM, pids[overlay ? 1 : 0], (ULONG64)address, (PBYTE)buffer, size) == TRUE;
}

template<typename T>
T MemRead(uintptr_t address, bool overlay = false, bool ignoreCache = false) {
    T t{};
    MemReadRaw((void*)address, &t, sizeof(T), overlay, ignoreCache);

    return t;
}

template<typename T>
bool ScatterPrepareRead(uintptr_t address, T* buffer, bool ignoreCache = false) {
    return VMMDLL_Scatter_PrepareEx(g_hScatter, address, sizeof(T), (PBYTE)buffer, NULL);
}

bool ClearScatterRequests()
{
    return VMMDLL_Scatter_Clear(g_hScatter, pids[0], NULL);
}

bool ExecuteScatterRead()
{
    return VMMDLL_Scatter_ExecuteRead(g_hScatter);
}
bool MemInitialize() {
    LPSTR args[3]{ (LPSTR)"", (LPSTR)"-device", (LPSTR)"fpga" };
    g_hVMM = VMMDLL_Initialize(3, args);
    g_hScatter = VMMDLL_Scatter_Initialize(g_hVMM, pids[0], NULL);
    if (!g_hVMM || g_hVMM == INVALID_HANDLE_VALUE || g_hScatter == INVALID_HANDLE_VALUE || !g_hScatter) {
        return false;
    }

    if (!VMMDLL_PidGetFromName(g_hVMM, (LPSTR)GAME_NAME, &pids[0])) {
        VMMDLL_Close(g_hVMM);

        return false;
    }

    g_gameBaseAddress = VMMDLL_ProcessGetModuleBaseU(g_hVMM, pids[0], (LPSTR)GAME_NAME);
    if (!g_gameBaseAddress) {
        VMMDLL_Close(g_hVMM);

        return false;
    }

    if (!VMMDLL_PidGetFromName(g_hVMM, (LPSTR)OVERLAY_NAME, &pids[1])) {
        VMMDLL_Close(g_hVMM);

        return false;
    }

    const auto overlayBase = VMMDLL_ProcessGetModuleBaseU(g_hVMM, pids[1], (LPSTR)OVERLAY_NAME);
    if (!overlayBase) {
        VMMDLL_Close(g_hVMM);

        return false;
    }

    g_payloadBufferAddress = MemRead<uintptr_t>(overlayBase + 0x3010, true);
    if (!g_payloadBufferAddress) {
        VMMDLL_Close(g_hVMM);

        return false;
    }

    return true;
}

void MemUninitialize() {
    VMMDLL_Close(g_hVMM);
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

std::string GetFNameFromID(uint32_t nameIndex) {
    if (nameCache.contains(nameIndex)) {
        return nameCache[nameIndex];
    }

    uint32_t Block = nameIndex >> 16;
    uint16_t Offset = static_cast<uint16_t>(nameIndex);
    uintptr_t FNamePool = g_gameBaseAddress + 0x08932300 + 0x10;
    uintptr_t NamePoolChunk = MemRead<uintptr_t>(FNamePool + (Block * sizeof(uintptr_t)));
    uintptr_t FNameEntry = NamePoolChunk + (2 * Offset);
    uint16_t FNameEntryHeader = MemRead<uint16_t>(FNameEntry);
    uintptr_t StrPtr = FNameEntry + 0x2;
    uint32_t StrLength = FNameEntryHeader >> 0x6;
    if (StrLength <= 0) {
        return "";
    }

    if (StrLength > 99) {
        StrLength = 99;
    }

    std::string actorName(StrLength, '\0');
    //char* actorNameBuffer = new char[StrLength];
    if (!MemReadRaw((void*)StrPtr, (void*)actorName.data(), StrLength)) {
        std::cerr << "Failed to read?" << std::endl;
    }
    
    actorName[StrLength] = '\0';
    nameCache[nameIndex] = actorName;

    return actorName;
}