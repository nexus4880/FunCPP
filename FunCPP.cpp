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
            std::cerr << "No gameWorld" << std::endl;

            continue;
        }

        const auto owningGameInstance = MemRead<uintptr_t>(gameWorld + 0x01B8);
        if (!owningGameInstance) {
            std::cerr << "No owningGameInstance" << std::endl;

            continue;
        }

        const auto localPlayersArray = MemRead<TArray<uintptr_t>>(owningGameInstance + 0x38);
        if (!localPlayersArray.Data || localPlayersArray.NumElements <= 0) {
            std::cerr << "No localPlayersArray.Data || localPlayersArray.NumElements" << std::endl;

            continue;
        }

        const auto localPlayer = MemRead<uintptr_t>((uintptr_t)localPlayersArray.Data);
        if (!localPlayer) {
            std::cerr << "No localPlayer" << std::endl;

            continue;
        }

        const auto localPlayerController = MemRead<uintptr_t>(localPlayer + 0x30);
        if (!localPlayerController) {
            std::cerr << "No localPlayerController" << std::endl;

            continue;
        }

        const auto localPlayerState = MemRead<uintptr_t>(localPlayerController + 0x02A8);
        if (!localPlayerState) {
            std::cerr << "No localPlayerState" << std::endl;

            continue;
        }

        const auto localPlayerCameraManager = MemRead<uintptr_t>(localPlayerController + 0x0358);
        if (!localPlayerCameraManager) {
            std::cerr << "No localPlayerCameraManager" << std::endl;

            continue;
        }

        const auto localPlayerRoot = MemRead<uintptr_t>(localPlayerCameraManager + 0x01A8);
        if (!localPlayerRoot) {
            std::cerr << "No localPlayerRoot" << std::endl;

            continue;
        }

        const auto fov = MemRead<float>(localPlayerCameraManager + 0x1320 + 0x10 + 0x0030);
        const auto cameraLocation = MemRead<Location>(localPlayerRoot + 0x0130, false, true);
        const Camera camera{ cameraLocation.rotation, cameraLocation.position, fov };
        const auto matrix = CreateMatrix(cameraLocation.rotation);

        const auto gameState = MemRead<uintptr_t>(gameWorld + 0x0158);
        if (!gameState) {
            std::cerr << "No gameState" << std::endl;

            continue;
        }

        const auto playerArray = MemRead<TArray<uintptr_t>>(gameState + 0x02B8);
        if (!playerArray.Data) {
            std::cerr << "No playerArray.Data" << std::endl;

            continue;
        }

        payload.initialized = true;
        const auto levelsArray = MemRead<TArray<uintptr_t>>(gameWorld + 0x0170);
        for (auto i = 0; i < levelsArray.NumElements; i++) {
            const auto level = MemRead<uintptr_t>((uintptr_t)levelsArray.Data + i * sizeof(uintptr_t));
            const auto actorsArray = MemRead<TArray<uintptr_t>>(level + 0x0098);
            for (auto j = 0; j < actorsArray.NumElements; j++) {
                const auto actor = MemRead<uintptr_t>((uintptr_t)actorsArray.Data + j * sizeof(uintptr_t));
                if (!actor) {
                    continue;
                }

                const auto fname = MemRead<uint32_t>(actor + 0x0018);
                const auto actorName = GetFNameFromID(fname);
                if (actorName != "BP_MFG_Soldier_C") {
                    continue;
                }

                const auto location = GetActorLocation(actor);
                const auto screenPosition = WorldToScreen(location.position, matrix, camera);

                const MilkRenLabel label{
                    L"AI",
                    (float)screenPosition.x, (float)screenPosition.y,
                    12.f, MilkRenColor(255, 0, 255, 255)
                };

                payload.labels.push_back(label);
            }
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
