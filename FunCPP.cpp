#include <iostream>
#include <string>
#include <stdint.h>
#include <vector>
#include <format>
#include "Payload.hpp"
#include "Memory.hpp"
#include "Engine.hpp"

constexpr uintptr_t GWORLD_OFFSET = 0x43795D8;
constexpr DWORD GAME_PID = 20028;
constexpr DWORD MILKREN_PID = 19308;
constexpr DWORD MILKREN_BUFFER_OFFSET = 64;

int main() {
    g_hGame = OpenProcess(PROCESS_ALL_ACCESS, false, GAME_PID);
    if (!g_hGame) {
        return 1;
    }

    const auto baseAddress = GetBaseAddress(L"HarshDoorstop-Win64-Shipping.exe", g_hGame);
    if (!baseAddress) {
        CloseHandle(g_hGame);

        return 1;
    }

    g_hMilkRen = OpenProcess(PROCESS_ALL_ACCESS, false, MILKREN_PID);
    if (!g_hMilkRen) {
        CloseHandle(g_hGame);

        return 1;
    }

    const auto milkRenBase = GetBaseAddress(L"MilkRen.Client.exe", g_hMilkRen);
    if (!milkRenBase) {
        CloseHandle(g_hGame);
        CloseHandle(g_hMilkRen);

        return 1;
    }
    const auto bufferAddress = MemRead<uintptr_t>(milkRenBase + 0x3010, g_hMilkRen);
    if (!bufferAddress) {
        CloseHandle(g_hGame);
        CloseHandle(g_hMilkRen);

        return 1;
    }

    Payload payload(256 * 1024);

    while (GetAsyncKeyState(VK_END) == 0) {
        static int frames = 0;
        const auto gameWorld = MemRead<uintptr_t>(baseAddress + GWORLD_OFFSET);
        if (!gameWorld) {
            continue;
        }

        const auto owningGameInstance = MemRead<uintptr_t>(gameWorld + 0x188);
        if (!owningGameInstance) {
            continue;
        }

        const auto localPlayersArray = MemRead<TArray<uintptr_t>>(owningGameInstance + 0x38);
        if (!localPlayersArray.Data || localPlayersArray.NumElements <= 0) {
            continue;
        }

        const auto localPlayer = MemRead<uintptr_t>((uintptr_t)localPlayersArray.Data);
        if (!localPlayer) {
            continue;
        }

        const auto localPlayerController = MemRead<uintptr_t>(localPlayer + 0x30);
        if (!localPlayerController) {
            continue;
        }

        const auto localPlayerState = MemRead<uintptr_t>(localPlayerController + 0x228);
        if (!localPlayerState) {
            continue;
        }

        const auto localPlayerCameraManager = MemRead<uintptr_t>(localPlayerController + 0x2B8);
        if (!localPlayerCameraManager) {
            continue;
        }

        const auto localPlayerRoot = MemRead<uintptr_t>(localPlayerCameraManager + 0x130);
        if (!localPlayerRoot) {
            continue;
        }

        const auto localPlayerTeamNum = MemRead<uint8_t>(localPlayerState + 0x348);
        const auto fov = MemRead<float>(localPlayerCameraManager + 0x1A60 + 0x10 + 0x18);
        const auto cameraPosition = MemRead<Vector3>(localPlayerRoot + 0x11C);
        const auto cameraRotation = MemRead<Vector3>(localPlayerRoot + 0x128);
        const Camera camera{ cameraRotation, cameraPosition, fov };
        const auto matrix = CreateMatrix(cameraRotation);

        const auto gameState = MemRead<uintptr_t>(gameWorld + 0x130);
        if (!gameState) {
            continue;
        }

        const auto playerArray = MemRead<TArray<uintptr_t>>(gameState + 0x238);
        if (!playerArray.Data) {
            continue;
        }

        payload.initialized = true;
        for (auto playerIndex = 0; playerIndex < playerArray.NumElements; playerIndex++) {
            const auto playerState = MemRead<uintptr_t>((uintptr_t)playerArray.Data + playerIndex * 0x8);
            if (!playerState) {
                continue;
            }

            const auto teamNum = MemRead<uint8_t>(playerState + 0x348);
            if (teamNum == localPlayerTeamNum) {
                continue;
            }

            const auto playerPawn = MemRead<uintptr_t>(playerState + 0x280);
            if (!playerPawn) {
                continue;
            }

            const auto playerController = MemRead<uintptr_t>(playerPawn + 0x238);
            if (!playerController) {
                continue;
            }

            const auto playerCharacter = MemRead<uintptr_t>(playerController + 0x260);
            if (!playerCharacter) {
                std::cout << "NULL" << std::endl;

                continue;
            }

            const auto playerRootComponent = MemRead<uintptr_t>(playerPawn + 0x130);
            if (!playerRootComponent) {
                continue;
            }

            const auto playerLocation = MemRead<Location>(playerRootComponent + 0x11C);
            const auto screenPosition = WorldToScreen(playerLocation.position, matrix, camera);
            if (screenPosition.x <= 0.f || screenPosition.x >= 1920.f ||
                screenPosition.y <= 0.f || screenPosition.y >= 1080.f) {
                continue;
            }
            
            std::optional<std::wstring> playerNameOptional = ReadFString(playerState + 0x300);
            const MilkRenLabel nameLabel{
                playerNameOptional.value_or(L"Player"),
                screenPosition.x,
                screenPosition.y,
                12.f,
                0xFFFF0000
            };

            payload.labels.push_back(nameLabel);
        }

        const MilkRenLabel label2{ std::to_wstring(frames), 20.f, 20.f, 12.f, 0xFFFFFFFF };
        payload.labels.push_back(label2);
        frames++;

        size_t size = 0;
        const auto pBuffer = payload.Serialize(size);
        WriteProcessMemory(g_hMilkRen, LPVOID(bufferAddress + MILKREN_BUFFER_OFFSET), pBuffer, size, NULL);
        payload.Reset();
    }

    payload.Reset();
    size_t size = 0;
    const auto pBuffer = payload.Serialize(size);
    WriteProcessMemory(g_hMilkRen, LPVOID(bufferAddress + MILKREN_BUFFER_OFFSET), pBuffer, size, NULL);

    CloseHandle(g_hGame);
    CloseHandle(g_hMilkRen);
}
