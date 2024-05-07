#include <stdint.h>

#include <format>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "Engine.hpp"
#include "Memory.hpp"
#include "Payload.hpp"
#include <thread>

int main() {
    if (!MemInitialize()) {
        std::cerr << "Failed to Init DMA, make sure the overlay is running" << std::endl;
        return 1;
    }

    std::cout << "Started with no issues!" << std::endl;

    Payload payload(256 * 1024);
    const auto gameWorld = MemRead<uintptr_t>(g_gameBaseAddress + GWORLD_OFFSET);
    if (!gameWorld) {
        std::cerr << "No gameWorld" << std::endl;
        return 1;
    }

    const auto owningGameInstance = MemRead<uintptr_t>(gameWorld + 0x01B8);
    if (!owningGameInstance) {
        std::cerr << "No owningGameInstance" << std::endl;
        return 1;
    }

    const auto localPlayersArray = MemRead<TArray<uintptr_t>>(owningGameInstance + 0x38);
    if (!localPlayersArray.Data || localPlayersArray.NumElements <= 0) {
        std::cerr << "No localPlayersArray.Data || localPlayersArray.NumElements" << std::endl;
        return 1;
    }

    const auto localPlayer = MemRead<uintptr_t>((uintptr_t)localPlayersArray.Data);
    if (!localPlayer) {
        std::cerr << "No localPlayer" << std::endl;
        return 1;
    }

    const auto localPlayerController = MemRead<uintptr_t>(localPlayer + 0x30);
    if (!localPlayerController) {
        std::cerr << "No localPlayerController" << std::endl;
        return 1;
    }

    const auto localPlayerState = MemRead<uintptr_t>(localPlayerController + 0x02A8);
    if (!localPlayerState) {
        std::cerr << "No localPlayerState" << std::endl;
        return 1;
    }

    const auto localPlayerCameraManager = MemRead<uintptr_t>(localPlayerController + 0x0358);
    if (!localPlayerCameraManager) {
        std::cerr << "No localPlayerCameraManager" << std::endl;
        return 1;
    }

    const auto gameState = MemRead<uintptr_t>(gameWorld + 0x0158);
    if (!gameState) {
        std::cerr << "No gameState" << std::endl;
        return 1;
    }

    const auto localPlayerRoot = MemRead<uintptr_t>(localPlayerCameraManager + 0x01A8);
    if (!localPlayerRoot) {
        std::cerr << "No localPlayerRoot" << std::endl;
        return 1;
    }

    std::unordered_map<int, uintptr_t> readActors;
    std::thread* pThread = nullptr;
    bool shouldThreadBeRan = true;
    while (GetAsyncKeyState(VK_ESCAPE) == 0) {
        static int frames = 0;
        const auto fov = MemRead<float>(localPlayerCameraManager + 0x1320 + 0x10 + 0x0030);
        const auto cameraLocation = MemRead<Location>(localPlayerRoot + 0x0130, false, true);
        const Camera camera{ cameraLocation.rotation, cameraLocation.position, fov };
        const auto matrix = CreateMatrix(cameraLocation.rotation);

        const auto playerArray = MemRead<TArray<uintptr_t>>(gameState + 0x02B8);
        if (!playerArray.Data) {
            std::cerr << "No playerArray.Data" << std::endl;
            continue;
        }

        payload.initialized = true;
        const auto levelsArray = MemRead<TArray<uintptr_t>>(gameWorld + 0x0170);
        auto lambda = [&]() {
            while (shouldThreadBeRan) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                for (int levelIndex = 0; levelIndex < levelsArray.NumElements; levelIndex++) {
                    const auto levelAddress = MemRead<uintptr_t>((uintptr_t)levelsArray.Data + levelIndex * sizeof(uintptr_t));
                    if (!levelAddress) {
                        continue;
                    }

                    if (std::find(Cache::levelArray.begin(), Cache::levelArray.end(), levelAddress) != Cache::levelArray.end()) {
                        continue;
                    }

                    Cache::levelArray.emplace_back(Level{ levelAddress });
                }

                for (auto levelIndex = 0; levelIndex < Cache::levelArray.size(); levelIndex++) {
                    Level& level = Cache::levelArray[levelIndex];
                    level.actors = MemRead<TArray<uintptr_t>>(level.m_address + 0x0098);
                    for (int actorIndex = 0; actorIndex < level.actors.NumElements; actorIndex++) {
                        const auto actor = MemRead<uintptr_t>((uintptr_t)level.actors.Data + actorIndex * sizeof(uintptr_t));
                        if (!actor) {
                            continue;
                        }

                        const auto fname = MemRead<uint32_t>(actor + 0x0018);
                        const auto actorName = GetFNameFromID(fname);
                        if (actorName != "BP_MFG_Soldier_C") {
                            continue;
                        }

                        const auto isDead = MemRead<bool>(actor + 0x0CF0);
                        if (isDead) {
                            continue;
                        }

                        const auto isAI = MemRead<bool>(actor + 0x0CE5);
                        Soldier soldier{};
                        soldier.m_address = actor;
                        soldier.m_isAI = isAI;
                        if (!isAI) {
                            const auto teamId = MemRead<uint8_t>(actor + 0x778);
                            soldier.m_teamSide = teamId;
                            const auto playerState = MemRead<uintptr_t>(actor + 0x02C0);
                            soldier.m_playerState = playerState;
                            if (playerState) {
                                soldier.m_name = L"Player: " + std::to_wstring(teamId);
                            }
                            else {
                                soldier.m_name = std::to_wstring(teamId);
                            }
                        }
                        else {
                            soldier.m_name = L"AI";
                        }

                        level.m_soldiers[soldier.m_address] = soldier;
                    }
                }
            }
            };

        auto copy = Cache::levelArray;
        for (auto& level : copy) {
            for (auto kvp : level.m_soldiers) {
                auto actor = kvp.first;
                const auto isDead = MemRead<bool>(actor + 0x0CF0);
                if (isDead) {
                    continue;
                }

                const auto location = GetActorLocation(actor);
                const auto screenPosition = WorldToScreen(location.position, matrix, camera);
                //const Soldier& soldier = level.m_soldiers[actor];
                const MilkRenLabel label{ L"ENEMY", (float)screenPosition.x, (float)screenPosition.y, 12.f, 0xFFFFFFFF };
                payload.labels.push_back(label);
            }
        }

        if (!pThread) {
            pThread = new std::thread(lambda);
        }

        const MilkRenLabel label2{ std::to_wstring(frames), 20.f, 20.f, 30.f, 0xFFFFFFFF };
        payload.labels.push_back(label2);
        frames++;
        size_t size = 0;
        const auto pBuffer = payload.Serialize(size);
        std::cout << size << std::endl;
        if (!MemWriteRaw((void*)(g_payloadBufferAddress + MILKREN_BUFFER_OFFSET), (void*)pBuffer, (DWORD)size, true)) {
            std::cerr << "FAILED TO WRITE TO OVERLAY" << std::endl;
        }

        payload.Reset();
    }

    pThread->join();
    payload.Reset();
    size_t size = 0;
    const auto pBuffer = payload.Serialize(size);
    MemWriteRaw((void*)(g_payloadBufferAddress + MILKREN_BUFFER_OFFSET), (void*)pBuffer, (DWORD)size, true);
    MemUninitialize();
}