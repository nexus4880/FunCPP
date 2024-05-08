#pragma once

#include "Math.hpp"
#include "Memory.hpp"
#include <vector>
#include <mutex>

struct Camera {
    Vector3 Rotation;
    Vector3 Location;
    float FOV;
};

template<typename T>
struct TArray
{
    T* Data;
    int NumElements;
    int MaxElements;

    std::vector<T> AsVector(bool ignoreCache = false) const {
        if (NumElements <= 0 || NumElements >= 1000000) {
            return {};
        }

        std::vector<T> result(NumElements);
        if (!MemReadRaw((void*)Data, (void*)result.data(), sizeof(T) * NumElements, false, ignoreCache)) {
            return {};
        }

        return result;
    }
};

class Soldier {
public:
    uintptr_t m_address;
    uintptr_t m_rootComponent;
    uintptr_t m_playerState;
    bool m_isAI;
    bool m_isDead;
    std::wstring m_name = L"";
    int m_teamSide;

    bool operator==(uintptr_t actorAddress) {
        return this->m_address == actorAddress;
    }
};

class Level {
public:
    uintptr_t m_address;
    std::unordered_map<uintptr_t, Soldier> m_soldiers{};
    std::vector<uintptr_t> m_actorsRead{};
    TArray<uintptr_t> actors{};

    Level(uintptr_t address) {
        m_address = address;
    }

    bool operator==(uintptr_t levelAddress) {
        return this->m_address == levelAddress;
    }
};

namespace Cache {
    std::vector<Level> renderArray; //Its not the cache of levelsArray its the cache of each level. Cached after reading it.
    std::mutex levelArrayLock{};
    std::vector<Level> levelArray; //Its not the cache of levelsArray its the cache of each level. Cached after reading it.
};

struct Location {
    Vector3 position;
    Vector3 rotation;
};

typedef struct _D3DMATRIX {
    union {
        struct {
            double        _11, _12, _13, _14;
            double        _21, _22, _23, _24;
            double        _31, _32, _33, _34;
            double        _41, _42, _43, _44;

        };
        double m[4][4];
    };
} D3DMATRIX;

D3DMATRIX CreateMatrix(Vector3 rot, Vector3 origin = { 0.0, 0.0, 0.0 }) {
    double radPitch = (rot.x * UCONST_Pi / 180.0);
    double radYaw = (rot.y * UCONST_Pi / 180.0);
    double radRoll = (rot.z * UCONST_Pi / 180.0);

    double SP = sin(radPitch);
    double CP = cos(radPitch);
    double SY = sin(radYaw);
    double CY = cos(radYaw);
    double SR = sin(radRoll);
    double CR = cos(radRoll);

    D3DMATRIX matrix;
    matrix.m[0][0] = CP * CY;
    matrix.m[0][1] = CP * SY;
    matrix.m[0][2] = SP;
    matrix.m[0][3] = 0.0;

    matrix.m[1][0] = SR * SP * CY - CR * SY;
    matrix.m[1][1] = SR * SP * SY + CR * CY;
    matrix.m[1][2] = -SR * CP;
    matrix.m[1][3] = 0.0;

    matrix.m[2][0] = -(CR * SP * CY + SR * SY);
    matrix.m[2][1] = CY * SR - CR * SP * SY;
    matrix.m[2][2] = CR * CP;
    matrix.m[2][3] = 0.0;

    matrix.m[3][0] = origin.x;
    matrix.m[3][1] = origin.y;
    matrix.m[3][2] = origin.z;
    matrix.m[3][3] = 1.0;

    return matrix;
}

Vector2 WorldToScreen(Vector3 world, const D3DMATRIX& temp_matrix, const Camera& camera)
{
    Vector3 vAxisX = Vector3{ temp_matrix.m[0][0], temp_matrix.m[0][1], temp_matrix.m[0][2] };
    Vector3 vAxisY = Vector3{ temp_matrix.m[1][0], temp_matrix.m[1][1], temp_matrix.m[1][2] };
    Vector3 vAxisZ = Vector3{ temp_matrix.m[2][0], temp_matrix.m[2][1], temp_matrix.m[2][2] };

    Vector3 vDelta = { world.x - camera.Location.x, world.y - camera.Location.y, world.z - camera.Location.z };
    Vector3 vTransformed = Vector3{ Dot(vDelta, vAxisY), Dot(vDelta, vAxisZ), Dot(vDelta,vAxisX) };

    if (vTransformed.z < 1.f)
        vTransformed.z = 1.f;

    double x = 1920.0 * 0.5 + vTransformed.x * (((1920.0 * 0.5) / tan(camera.FOV * UCONST_Pi / 360.0))) / vTransformed.z;
    double y = 1080.0 * 0.5 - vTransformed.y * (((1920.0 * 0.5) / tan(camera.FOV * UCONST_Pi / 360.0))) / vTransformed.z;

    return Vector2{ x, y };
}

Location GetActorLocation(uintptr_t actorPtr) {
    const auto playerRootComponent = MemRead<uintptr_t>(actorPtr + 0x01A8);
    if (!playerRootComponent) {
        return { 0.0, 0.0, 0.0 };
    }

    return MemRead<Location>(playerRootComponent + 0x0130, false, true);
}