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

void RefreshPlayerList(const uintptr_t gameWorld, bool* shouldThreadBeRan) {
    while (*shouldThreadBeRan) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        const auto levelsTArray = MemRead<TArray<uintptr_t>>(gameWorld + 0x0170);
        const auto levelsArray = levelsTArray.AsVector();
        //printf_s("Read %i levels at %p\n", levelsTArray.NumElements, (void*)levelsTArray.Data);
        Cache::levelArrayLock.lock();
        if (Cache::levelArray.size() > 0) {
            Cache::levelArrayLock.unlock();

            continue;
        }

        for (auto levelAddress : levelsArray) {
            if (!levelAddress) {
                continue;
            }

            Cache::levelArray.push_back(Level{ levelAddress });
        }

        for (Level& level : Cache::levelArray) {
            level.actors = MemRead<TArray<uintptr_t>>(level.m_address + 0x0098);
            //printf_s("Level %p has %i actors at %p\n", (void*)level.m_address, level.actors.NumElements, (void*)level.actors.Data);
            const auto actors = level.actors.AsVector();
            for (const auto actor : actors) {
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
                    if (level.m_soldiers.contains(actor)) {
                        level.m_soldiers[actor].m_isDead = true;
                    }

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
                    std::wstring factionName = L"";
                    switch (soldier.m_teamSide) {
                    case 0: {
                        factionName = L"[LRI] ";
                        break;
                    }
                    case 1: {
                        factionName = L"[MSS] ";
                        break;
                    }
                    case 2: {
                        factionName = L"[CSI] ";
                        break;
                    }
                    default:{
                        factionName = L"[UNK] ";
                        break;
                    }
                    }

                    soldier.m_name = factionName + L"Player: ";
                }
                else {
                    soldier.m_name = L"AI: ";
                }

                level.m_soldiers[soldier.m_address] = soldier;
            }
        }

        Cache::levelArrayLock.unlock();
    }
}

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

    printf_s("[GameWorld] %s: %p\n", GetFNameFromID(MemRead<int32_t>(gameWorld + 0x18)).c_str(), (void*)gameWorld);
    const auto owningGameInstance = MemRead<uintptr_t>(gameWorld + 0x01B8);
    if (!owningGameInstance) {
        std::cerr << "No owningGameInstance" << std::endl;
        return 1;
    }

    printf_s("[OwningGameInstance] %s: %p\n", GetFNameFromID(MemRead<int32_t>(owningGameInstance + 0x18)).c_str(), (void*)owningGameInstance);
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

    printf_s("[LocalPlayer] %s: %p\n", GetFNameFromID(MemRead<int32_t>(localPlayer + 0x18)).c_str(), (void*)localPlayer);
    const auto localPlayerController = MemRead<uintptr_t>(localPlayer + 0x30);
    if (!localPlayerController) {
        std::cerr << "No localPlayerController" << std::endl;
        return 1;
    }
    
    printf_s("[LocalPlayerController] %s: %p\n", GetFNameFromID(MemRead<int32_t>(localPlayerController + 0x18)).c_str(), (void*)localPlayerController);

    const auto localPlayerTeamId = MemRead<uint8_t>(localPlayerController + 0x778);
    printf_s("LocalPlayerTeamId: %d\n", localPlayerTeamId);

    const auto localPlayerPawn = MemRead<uintptr_t>(localPlayerController + 0x0348);
    if (!localPlayerPawn) {
        std::cerr << "No localPlayerPawn" << std::endl;
        return 1;
    }

    printf_s("[localPlayerPawn] %s: %p\n", GetFNameFromID(MemRead<int32_t>(localPlayerPawn + 0x18)).c_str(), (void*)localPlayerPawn);
    const auto localPlayerState = MemRead<uintptr_t>(localPlayerController + 0x02A8);
    if (!localPlayerState) {
        std::cerr << "No localPlayerState" << std::endl;
        return 1;
    }

    printf_s("[LocalPlayerState] %s: %p\n", GetFNameFromID(MemRead<int32_t>(localPlayerState + 0x18)).c_str(), (void*)localPlayerState);
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

    printf_s("[GameState] %s: %p\n", GetFNameFromID(MemRead<int32_t>(gameState + 0x18)).c_str(), (void*)gameState);
    const auto localPlayerRoot = MemRead<uintptr_t>(localPlayerCameraManager + 0x01A8);
    if (!localPlayerRoot) {
        std::cerr << "No localPlayerRoot" << std::endl;
        return 1;
    }

    std::unordered_map<int, uintptr_t> readActors;
    bool shouldThreadBeRan = true;
    std::thread* pThread = new std::thread(RefreshPlayerList, gameWorld, &shouldThreadBeRan);
    while (GetAsyncKeyState(VK_ESCAPE) == 0) {
        static int frames = 0;

        const auto fov = MemRead<float>(localPlayerCameraManager + 0x1320 + 0x10 + 0x0030);
        const auto cameraLocation = MemRead<Location>(localPlayerRoot + 0x0130, false, true);
        const Camera camera{ cameraLocation.rotation, cameraLocation.position, fov };
        const auto matrix = CreateMatrix(cameraLocation.rotation);

        payload.initialized = true;
        if (Cache::levelArrayLock.try_lock()) {
            if (Cache::levelArray.size() > 0) {
                Cache::renderArray = Cache::levelArray;
                Cache::levelArray.clear();
            }

            Cache::levelArrayLock.unlock();
        }

        for (const auto& level : Cache::renderArray) {
            for (const auto& kvp : level.m_soldiers) {
                if (!kvp.first ) {
                    continue;
                }

                const auto location = GetActorLocation(kvp.first);
                const auto distance = (int)CalculateDistance(camera.Location, location.position) / 100;
                if (distance > 250.f) {
                    continue;
                }

                const auto screenPosition = WorldToScreen(location.position, matrix, camera);
                if (screenPosition.x < 0.0 || screenPosition.x > 1920.0 || screenPosition.y < 0.0 || screenPosition.y > 1080.0) {
                    continue;
                }

                MilkRenColor color = kvp.second.m_isAI ? MilkRenColor(216, 250, 8,255) : MilkRenColor(255, 0, 0, 255);
                const MilkRenLabel label{ kvp.second.m_name + std::to_wstring(distance) + L"M", (float)screenPosition.x, (float)screenPosition.y, 12.f, color};
                payload.labels.push_back(label);
            }
        }

        //printf_s("<%f, %f, %f> <%f, %f, %f> | %f\n", camera.Location.x, camera.Location.y, camera.Location.z, camera.Rotation.x, camera.Rotation.y, camera.Rotation.z, camera.FOV);
        const MilkRenLabel label2{ std::to_wstring(frames), 20.f, 20.f, 30.f, 0xFFFFFFFF };
        payload.labels.push_back(label2);
        frames++;
        size_t size = 0;
        const auto pBuffer = payload.Serialize(size);
        if (!MemWriteRaw((void*)(g_payloadBufferAddress + MILKREN_BUFFER_OFFSET), (void*)pBuffer, (DWORD)size, true)) {
            std::cerr << "FAILED TO WRITE TO OVERLAY" << std::endl;
        }

        payload.Reset();
    }

    shouldThreadBeRan = false;
    pThread->join();
    payload.Reset();
    size_t size = 0;
    const auto pBuffer = payload.Serialize(size);
    MemWriteRaw((void*)(g_payloadBufferAddress + MILKREN_BUFFER_OFFSET), (void*)pBuffer, (DWORD)size, true);
    MemUninitialize();
}