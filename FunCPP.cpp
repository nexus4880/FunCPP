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

std::chrono::time_point<std::chrono::high_resolution_clock> g_lastTick;
int g_lastFPS = 0;
int g_frames = 0;

void RefreshPlayerList(const uintptr_t gameWorld, bool* shouldThreadBeRan) {
    while (*shouldThreadBeRan) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto levelsTArray = MemRead<TArray<uintptr_t>>(gameWorld + 0x0170);
        auto levelsArray = levelsTArray.AsVector();
        //printf_s("Read %i levels at %p\n", levelsTArray.NumElements, (void*)levelsTArray.Data);
        for (auto level : levelsArray) {
            auto actorsArray = MemRead<TArray<uintptr_t>>(level + 0x0098);
            //printf_s("Level %p has %i actors at %p\n", (void*)level.m_address, level.actors.NumElements, (void*)level.actors.Data);
            auto actors = actorsArray.AsVector();
            for (const auto actor : actors) {
                if (!actor) {
                    continue;
                }

                const auto fname = MemRead<uint32_t>(actor + 0x0018);
                const auto actorName = GetFNameFromID(fname);
                if (actorName != "BP_MFG_Soldier_C") {
                    continue;
                }

                if (!Cache::g_Soldiers.contains(actor)) {
                    Cache::g_Soldiers[actor] = new Soldier(actor);
                }
            }
        }
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

    const auto localPlayerCharacter = MemRead<uintptr_t>(localPlayerController + 0x02F0);
    if (!localPlayerCharacter) {
        std::cerr << "No localPlayerCharacter" << std::endl;
        return 1;
    }

    printf_s("[LocalPlayerCharacter] %s: %p\n", GetFNameFromID(MemRead<int32_t>(localPlayerCharacter + 0x18)).c_str(), (void*)localPlayerCharacter);
    
    const auto characterMovement = MemRead<uintptr_t>(localPlayerCharacter + 0x0330);
    if (!characterMovement) {
        std::cerr << "No characterMovement" << std::endl;
        return 1;
    }

    printf_s("[CharacterMovement] %s: %p\n", GetFNameFromID(MemRead<int32_t>(characterMovement + 0x18)).c_str(), (void*)characterMovement);
    MemWrite<uint8_t>(characterMovement + 0x0213, 0);

    std::unordered_map<int, uintptr_t> readActors;
    bool shouldThreadBeRan = true;
    std::thread* pThread = new std::thread(RefreshPlayerList, gameWorld, &shouldThreadBeRan);
    while (GetAsyncKeyState(VK_ESCAPE) == 0) {
        g_frames++;

        static bool doSpeedHack = false;
        if ((GetAsyncKeyState(VK_NUMPAD3) & 1)) {
            doSpeedHack = !doSpeedHack;
            printf_s("Toggled %s speed hack\n", doSpeedHack ? "on" : "off");
        }

        if (doSpeedHack) {
            auto velocity = MemRead<Vector3>(characterMovement + 0xC0);
            const auto speed = Vector3Size(velocity);
            if (speed <= 3000) {
                velocity = Vector3Scale(velocity, 1.1);
                MemWrite<Vector3>(characterMovement + 0xC0, velocity);
            }
        }

        bool multiplyJumpHeight = (GetAsyncKeyState(VK_ADD) & 1);
        bool changeJumpHeight = multiplyJumpHeight || (GetAsyncKeyState(VK_SUBTRACT) & 1);
        if (changeJumpHeight) {
            auto jumpVelocity = MemRead<float>(characterMovement + 0x0180);
            if (multiplyJumpHeight) {
                jumpVelocity *= 2.f;
            }
            else {
                jumpVelocity /= 2.f;
            }

            printf_s("Jump velocity set to %f (default is 420)\n", jumpVelocity);
            MemWrite<float>(characterMovement + 0x0180, jumpVelocity);
        }

        auto pawnFlags = MemRead<uint8_t>(localPlayerPawn + 0x60);
        bool bReplication = pawnFlags & (1 << 4);
        if ((GetAsyncKeyState(VK_NUMPAD1) & 1)) {
            pawnFlags ^= (1 << 4);
            MemWrite<uint8_t>(localPlayerPawn + 0x60, pawnFlags);
            printf_s("Toggled %s replication\n", bReplication ? "off" : "on");
        }

        if (bReplication == false && (GetAsyncKeyState(VK_NUMPAD2) & 1)) {
            auto movementMode = MemRead<uint8_t>(characterMovement + 0x211);
            uint8_t targetMovementMode = movementMode == 1 ? 5 : 1;
            MemWrite<uint8_t>(characterMovement + 0x211, targetMovementMode);
            printf_s("Toggled %s fly mode\n", targetMovementMode == 5 ? "on" : "off");
        }

        const auto fov = MemRead<float>(localPlayerCameraManager + 0x1320 + 0x10 + 0x0030);
        const auto cameraLocation = MemRead<Location>(localPlayerRoot + 0x0130, false, true);
        const Camera camera{ cameraLocation.rotation, cameraLocation.position, fov };
        const auto matrix = CreateMatrix(cameraLocation.rotation);

        payload.initialized = true;
        auto soldiers = Cache::g_Soldiers;
        for (auto kvp : soldiers) {
            if (kvp.first == localPlayerPawn) {
                continue;
            }

            if (kvp.second->IsDead()) {
                continue;
            }

            kvp.second->InvalidatePosition();
            const auto location = kvp.second->GetPosition();
            const int distance = CalculateDistance(camera.Location, location) / 100;
            if (distance > 250) {
                continue;
            }

            const auto screenPosition = WorldToScreen(location, matrix, camera);
            if (screenPosition.x <= 0.0 || screenPosition.x >= 1920.0 || screenPosition.y <= 0.0 || screenPosition.y >= 1080.0) {
                continue;
            }

            kvp.second->DrawBoxESP(payload, matrix, camera);
            MilkRenColor color = kvp.second->IsAI() ? MilkRenColor(216, 250, 8, 255) : MilkRenColor(255, 0, 0, 255);
            const MilkRenLabel label{ kvp.second->GetName() + L" [" + std::to_wstring(distance) + L"M]", (float)screenPosition.x, (float)screenPosition.y, 12.f, color };
            payload.labels.push_back(label);
        }

        const auto now = std::chrono::high_resolution_clock::now();
        const auto diff = now - g_lastTick;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() >= 1000) {
            g_lastFPS = g_frames;
            g_frames = 0;
            g_lastTick = now;
        }

        //printf_s("<%f, %f, %f> <%f, %f, %f> | %f\n", camera.Location.x, camera.Location.y, camera.Location.z, camera.Rotation.x, camera.Rotation.y, camera.Rotation.z, camera.FOV);
        const MilkRenLabel fpsLabel{ std::to_wstring(g_lastFPS), 1920.f * 0.5f, 20.f, 30.f, 0xFFFFFFFF };
        payload.labels.push_back(fpsLabel);
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