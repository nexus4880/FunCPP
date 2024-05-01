#pragma once

#include "Math.hpp"

struct Camera {
    Vector3 Rotation;
    Vector3 Location;
    float FOV;
};

template<typename ArrayElementType>
struct TArray
{
    ArrayElementType* Data;
    int NumElements;
    int MaxElements;
};

struct Location {
    Vector3 position;
    FRotator rotation;
};

typedef struct _D3DMATRIX {
    union {
        struct {
            float        _11, _12, _13, _14;
            float        _21, _22, _23, _24;
            float        _31, _32, _33, _34;
            float        _41, _42, _43, _44;

        };
        float m[4][4];
    };
} D3DMATRIX;

D3DMATRIX CreateMatrix(Vector3 rot, Vector3 origin = { 0, 0, 0 }) {
    float radPitch = (rot.x * float(UCONST_Pi) / 180.f);
    float radYaw = (rot.y * float(UCONST_Pi) / 180.f);
    float radRoll = (rot.z * float(UCONST_Pi) / 180.f);

    float SP = sinf(radPitch);
    float CP = cosf(radPitch);
    float SY = sinf(radYaw);
    float CY = cosf(radYaw);
    float SR = sinf(radRoll);
    float CR = cosf(radRoll);

    D3DMATRIX matrix;
    matrix.m[0][0] = CP * CY;
    matrix.m[0][1] = CP * SY;
    matrix.m[0][2] = SP;
    matrix.m[0][3] = 0.f;

    matrix.m[1][0] = SR * SP * CY - CR * SY;
    matrix.m[1][1] = SR * SP * SY + CR * CY;
    matrix.m[1][2] = -SR * CP;
    matrix.m[1][3] = 0.f;

    matrix.m[2][0] = -(CR * SP * CY + SR * SY);
    matrix.m[2][1] = CY * SR - CR * SP * SY;
    matrix.m[2][2] = CR * CP;
    matrix.m[2][3] = 0.f;

    matrix.m[3][0] = origin.x;
    matrix.m[3][1] = origin.y;
    matrix.m[3][2] = origin.z;
    matrix.m[3][3] = 1.f;

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

    float x = 1920.f / 2.f + vTransformed.x * (((1920.f / 2.0f) / tanf(camera.FOV * (float)UCONST_Pi / 360.f))) / vTransformed.z;
    float y = 1080.f / 2.f - vTransformed.y * (((1920.f / 2.0f) / tanf(camera.FOV * (float)UCONST_Pi / 360.f))) / vTransformed.z;

    return Vector2{ x, y };
}