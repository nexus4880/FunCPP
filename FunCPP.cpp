#include <iostream>
#include <string>
#include <stdint.h>
#include <vector>
#include <format>
#include "Payload.hpp"
#include "Memory.hpp"
#include "Engine.hpp"

int main() {
    if (!MemInitialize()) {
        return false;
    }

    Payload payload(256 * 1024);

    while (GetAsyncKeyState(VK_ESCAPE) == 0) {
        static int frames = 0;
        const auto gameWorld = MemRead<uintptr_t>(g_gameBaseAddress + GWORLD_OFFSET);
        if (!gameWorld) {
            continue;
        }

        const auto owningGameInstance = MemRead<uintptr_t>(gameWorld + 0x01B8);
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

        const auto localPlayerState = MemRead<uintptr_t>(localPlayerController + 0x02A8);
        if (!localPlayerState) {
            continue;
        }

        const auto localPlayerCameraManager = MemRead<uintptr_t>(localPlayerController + 0x0358);
        if (!localPlayerCameraManager) {
            continue;
        }

        const auto localPlayerRoot = MemRead<uintptr_t>(localPlayerCameraManager + 0x01A8);
        if (!localPlayerRoot) {
            continue;
        }

        const auto fov = MemRead<float>(localPlayerCameraManager + 0x1320 + 0x10 + 0x0030);
        const auto cameraLocation = MemRead<Location>(localPlayerRoot + 0x0130, false, true);
        const Camera camera{ cameraLocation.rotation, cameraLocation.position, fov };
        const auto matrix = CreateMatrix(cameraLocation.rotation);

        const auto gameState = MemRead<uintptr_t>(gameWorld + 0x0158);
        if (!gameState) {
            continue;
        }

        const auto playerArray = MemRead<TArray<uintptr_t>>(gameState + 0x02B8);
        if (!playerArray.Data) {
            continue;
        }

        payload.initialized = true;
        for (auto playerIndex = 0; playerIndex < playerArray.NumElements; playerIndex++) {
            const auto playerState = MemRead<uintptr_t>((uintptr_t)playerArray.Data + playerIndex * 0x8);
            if (!playerState) {
                continue;
            }

            const auto playerPawn = MemRead<uintptr_t>(playerState + 0x0318);
            if (!playerPawn) {
                continue;
            }

            const auto playerRootComponent = MemRead<uintptr_t>(playerPawn + 0x01A8);
            if (!playerRootComponent) {
                continue;
            }

            const auto playerLocation = MemRead<Location>(playerRootComponent + 0x0130);
            const auto screenPosition = WorldToScreen(playerLocation.position, matrix, camera);
            if (screenPosition.x <= 0.0 || screenPosition.x >= 1920.0 ||
                screenPosition.y <= 0.0 || screenPosition.y >= 1080.0) {
                continue;
            }
            
            std::optional<std::wstring> playerNameOptional = ReadFString(playerState + 0x0338);
            const MilkRenLabel nameLabel{
                playerNameOptional.value_or(L"Player"),
                (float)screenPosition.x,
                (float)screenPosition.y,
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
        MemWriteRaw((void*)(g_payloadBufferAddress + MILKREN_BUFFER_OFFSET), (void*)pBuffer, (DWORD)size, true);
        payload.Reset();
    }

    payload.Reset();
    size_t size = 0;
    const auto pBuffer = payload.Serialize(size);
    MemWriteRaw((void*)(g_payloadBufferAddress + MILKREN_BUFFER_OFFSET), (void*)pBuffer, (DWORD)size, true);

    MemUninitialize();
}
