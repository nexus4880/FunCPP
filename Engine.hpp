#pragma once

#include "Math.hpp"
#include "Memory.hpp"
#include <vector>
#include <mutex>
#include <optional>
#include <sstream>
#include "Payload.hpp"

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

struct Location {
    Vector3 position;
    Vector3 rotation;
};

Location GetActorLocation(uintptr_t actorPtr);

struct Camera {
    Vector3 Rotation;
    Vector3 Location;
    float FOV;
};

Vector2 WorldToScreen(Vector3 world, const D3DMATRIX& temp_matrix, const Camera& camera);

struct FBoxSphereBounds {
    struct Vector3                                Origin;                                            // 0x0000(0x0018)(Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
    struct Vector3                                BoxExtent;                                         // 0x0018(0x0018)(Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
};

template<typename T>
struct TArray
{
    T* Data;
    int NumElements;
    int MaxElements;

    std::vector<T> AsVector(bool ignoreCache = false) {
        if (NumElements > 0 && NumElements < 1000000) {
            std::vector<T> result(NumElements);
            if (MemReadRaw((void*)Data, (void*)result.data(), sizeof(T) * NumElements, false, ignoreCache)) {
                return result;
            }
        }

        return std::vector<T>{};
    }
};

struct DrawBoundingBoxData {
    bool invalidated;
    bool shouldDraw;
    Vector3 basePosition;
    Vector2 screenPoints[8];
};

DrawBoundingBoxData GetBoundingBoxData(const FBoxSphereBounds& boxSphereBounds, const D3DMATRIX& matrix, const Camera& camera) {
    DrawBoundingBoxData result{};
    const Vector3& origin = boxSphereBounds.Origin;
    const Vector3& extent = boxSphereBounds.BoxExtent;

    result.invalidated = false;
    result.basePosition = origin;

    Vector3 corners[8];
    corners[0] = Vector3Add(origin, Vector3{ -extent.x, -extent.y, -extent.z }); // Front bottom left
    corners[1] = Vector3Add(origin, Vector3{ extent.x, -extent.y, -extent.z }); // Front bottom right
    corners[2] = Vector3Add(origin, Vector3{ -extent.x, extent.y, -extent.z });  // Front top left
    corners[3] = Vector3Add(origin, Vector3{ extent.x, extent.y, -extent.z });  // Front top right
    corners[4] = Vector3Add(origin, Vector3{ -extent.x, -extent.y, extent.z });  // Back bottom left
    corners[5] = Vector3Add(origin, Vector3{ extent.x, -extent.y, extent.z });  // Back bottom right
    corners[6] = Vector3Add(origin, Vector3{ -extent.x, extent.y, extent.z });   // Back top left
    corners[7] = Vector3Add(origin, Vector3{ extent.x, extent.y, extent.z });   // Back top right

    // Convert corner points from world space to screen space
    for (int i = 0; i < 8; ++i) {
        result.screenPoints[i] = WorldToScreen(corners[i], matrix, camera);
        if (result.screenPoints[i].x <= 0.0 || result.screenPoints[i].x >= 1920.0 || result.screenPoints[i].y <= 0.0 || result.screenPoints[i].y >= 1080.0) {
            result.shouldDraw = false;

            return result;
        }
    }

    result.shouldDraw = true;

    return result;
}

class Soldier {
public:
    Soldier() = delete;

    Soldier(uintptr_t address) : m_address(address) {
        m_isAI = MemRead<bool>(m_address + 0x0CE5);
        m_teamSide = MemRead<uint8_t>(m_address + 0x778);
        std::wostringstream stream{};
        if (!IsAI()) {
            switch (GetTeamSide()) {
            case 0: {
                stream << L"[LRI] ";
                break;
            }
            case 1: {
                stream << L"[MSS] ";
                break;
            }
            case 2: {
                stream << L"[CSI] ";
                break;
            }
            default: {
                stream << L"[UNK] ";
                break;
            }
            }
        }

        stream << (IsAI() ? L"AI" : L"PLAYER");
        m_name = stream.str();
    }

    bool IsAI() const {
        return m_isAI;
    }

    bool IsDead() {
        if (m_isDead == false) {
            m_isDead = MemRead<bool>(m_address + 0x0CF0);
        }

        return m_isDead;
    }

    void DrawBoxESP(struct Payload& payload, const D3DMATRIX& matrix, const Camera& camera) {
        static auto boxOrigin = Vector3{ 0.000010, 7.792915, 78.891420 };
        static auto boxExtent = Vector3{ 63.953703, 19.529924, 77.974425 };
        auto thisBoxSphereBounds = FBoxSphereBounds{ boxOrigin, boxExtent };
        thisBoxSphereBounds.Origin = Vector3Add(thisBoxSphereBounds.Origin, GetPosition());
        thisBoxSphereBounds.Origin.z -= thisBoxSphereBounds.BoxExtent.z;
        if (m_boundingBoxData.invalidated || !Vector3ApproxEquals(m_boundingBoxData.basePosition, thisBoxSphereBounds.Origin)) {
            thisBoxSphereBounds.BoxExtent.y *= 1.2;
            thisBoxSphereBounds.BoxExtent.x *= 0.6;
            m_boundingBoxData = GetBoundingBoxData(thisBoxSphereBounds, matrix, camera);
        }

        if (!m_boundingBoxData.shouldDraw) {
            return;
        }

        // Define lines to connect the corners of the box
        // Front face
        payload.lines.push_back({ (float)m_boundingBoxData.screenPoints[0].x, (float)m_boundingBoxData.screenPoints[0].y, (float)m_boundingBoxData.screenPoints[1].x, (float)m_boundingBoxData.screenPoints[1].y, 0xFFFFFFFF });
        payload.lines.push_back({ (float)m_boundingBoxData.screenPoints[1].x, (float)m_boundingBoxData.screenPoints[1].y, (float)m_boundingBoxData.screenPoints[3].x, (float)m_boundingBoxData.screenPoints[3].y, 0xFFFFFFFF });
        payload.lines.push_back({ (float)m_boundingBoxData.screenPoints[3].x, (float)m_boundingBoxData.screenPoints[3].y, (float)m_boundingBoxData.screenPoints[2].x, (float)m_boundingBoxData.screenPoints[2].y, 0xFFFFFFFF });
        payload.lines.push_back({ (float)m_boundingBoxData.screenPoints[2].x, (float)m_boundingBoxData.screenPoints[2].y, (float)m_boundingBoxData.screenPoints[0].x, (float)m_boundingBoxData.screenPoints[0].y, 0xFFFFFFFF });

        // Back face
        payload.lines.push_back({ (float)m_boundingBoxData.screenPoints[4].x, (float)m_boundingBoxData.screenPoints[4].y, (float)m_boundingBoxData.screenPoints[5].x, (float)m_boundingBoxData.screenPoints[5].y, 0xFFFFFFFF });
        payload.lines.push_back({ (float)m_boundingBoxData.screenPoints[5].x, (float)m_boundingBoxData.screenPoints[5].y, (float)m_boundingBoxData.screenPoints[7].x, (float)m_boundingBoxData.screenPoints[7].y, 0xFFFFFFFF });
        payload.lines.push_back({ (float)m_boundingBoxData.screenPoints[7].x, (float)m_boundingBoxData.screenPoints[7].y, (float)m_boundingBoxData.screenPoints[6].x, (float)m_boundingBoxData.screenPoints[6].y, 0xFFFFFFFF });
        payload.lines.push_back({ (float)m_boundingBoxData.screenPoints[6].x, (float)m_boundingBoxData.screenPoints[6].y, (float)m_boundingBoxData.screenPoints[4].x, (float)m_boundingBoxData.screenPoints[4].y, 0xFFFFFFFF });

        // Connect front and back faces
        payload.lines.push_back({ (float)m_boundingBoxData.screenPoints[0].x, (float)m_boundingBoxData.screenPoints[0].y, (float)m_boundingBoxData.screenPoints[4].x, (float)m_boundingBoxData.screenPoints[4].y, 0xFFFFFFFF });
        payload.lines.push_back({ (float)m_boundingBoxData.screenPoints[1].x, (float)m_boundingBoxData.screenPoints[1].y, (float)m_boundingBoxData.screenPoints[5].x, (float)m_boundingBoxData.screenPoints[5].y, 0xFFFFFFFF });
        payload.lines.push_back({ (float)m_boundingBoxData.screenPoints[2].x, (float)m_boundingBoxData.screenPoints[2].y, (float)m_boundingBoxData.screenPoints[6].x, (float)m_boundingBoxData.screenPoints[6].y, 0xFFFFFFFF });
        payload.lines.push_back({ (float)m_boundingBoxData.screenPoints[3].x, (float)m_boundingBoxData.screenPoints[3].y, (float)m_boundingBoxData.screenPoints[7].x, (float)m_boundingBoxData.screenPoints[7].y, 0xFFFFFFFF });
    }

    std::wstring& GetName() {
        return m_name;
    }

    uint8_t GetTeamSide() const {
        return m_teamSide;
    }

    Vector3 GetPosition() {
        if (!m_cachedPosition.has_value()) {
            const auto newPosition = GetActorLocation(m_address).position;
            if (m_lastPosition.has_value()) {
                if (!Vector3ApproxEquals(newPosition, m_lastPosition.value())) {
                    m_boundingBoxData.invalidated = true;
                }
            }

            m_cachedPosition = newPosition;
        }

        return m_cachedPosition.value();
    }

    void InvalidatePosition() {
        if (m_cachedPosition.has_value()) {
            m_lastPosition = m_cachedPosition.value();
        }

        m_cachedPosition = std::nullopt;
    }

    bool operator==(uintptr_t actorAddress) {
        return m_address == actorAddress;
    }

private:
    uintptr_t m_address;
    bool m_isAI;
    bool m_isDead = false;
    std::wstring m_name = L"?";
    uint8_t m_teamSide;
    DrawBoundingBoxData m_boundingBoxData = {};
    std::optional<Vector3> m_cachedPosition = std::nullopt;
    std::optional<Vector3> m_lastPosition = std::nullopt;
};

struct Level {
    uintptr_t m_address;
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
    std::unordered_map<uintptr_t, Soldier*> g_Soldiers;
};

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
    Vector3 vTransformed = Vector3{ Vector3Dot(vDelta, vAxisY), Vector3Dot(vDelta, vAxisZ), Vector3Dot(vDelta,vAxisX) };

    if (vTransformed.z < 1.0)
        vTransformed.z = 1.0;

    double x = 1920.0 * 0.5 + vTransformed.x * (((1920.0 * 0.5) / tan(camera.FOV * UCONST_Pi / 360.0))) / vTransformed.z;
    double y = 1080.0 * 0.5 - vTransformed.y * (((1920.0 * 0.5) / tan(camera.FOV * UCONST_Pi / 360.0))) / vTransformed.z;

    return Vector2{ x, y };
}

Location GetActorLocation(uintptr_t actorPtr) {
    static std::unordered_map<uintptr_t, uintptr_t> s_rootComponents{};
    uintptr_t rootComponent = 0;
    if (!s_rootComponents.contains(actorPtr)) {
        rootComponent = MemRead<uintptr_t>(actorPtr + 0x01A8);
        if (!rootComponent) {
            return { };
        }

        s_rootComponents[actorPtr] = rootComponent;
    }
    else {
        rootComponent = s_rootComponents[actorPtr];
    }

    return MemRead<Location>(rootComponent + 0x0130, false, true);
}