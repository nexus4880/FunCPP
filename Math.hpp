#pragma once

#include <cmath>

constexpr float UCONST_Pi = 3.1415926f;
constexpr float URotationToRadians = UCONST_Pi / 180.f;

struct Vector3 {
    float x, y, z;
};

struct Vector2 {
    float x, y;
};

struct Vector4 {
    float x, y, z, w;
};

struct FRotator {
    float pitch, yaw, roll;
};

Vector3 RotationToVector(FRotator R) {
    Vector3 Vec;
    float fYaw = R.yaw * URotationToRadians;
    float fPitch = R.pitch * URotationToRadians;
    float CosPitch = cos(fPitch);
    Vec.x = cos(fYaw) * CosPitch;
    Vec.y = sin(fYaw) * CosPitch;
    Vec.z = sin(fPitch);

    return Vec;
}

float Size(Vector3& v) {
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

void Normalize(Vector3& v) {
    float size = Size(v);

    if (!size)
        v.x = v.y = v.z = 1;
    else
    {
        v.x /= size;
        v.y /= size;
        v.z /= size;
    }
}

Vector3 VectorSubtract(Vector3 va, Vector3 vb) {
    Vector3 out;

    out.x = va.x - vb.x;
    out.y = va.y - vb.y;
    out.z = va.z - vb.z;

    return out;
}

float Vector3Distance(const Vector3& va, const Vector3& vb) {
    float dx = va.x - vb.x;
    float dy = va.y - vb.y;
    float dz = va.z - vb.z;

    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

float inline Dot(const Vector3& V1, const Vector3& V2) {
    return (V1.x * V2.x + V1.y * V2.y + V1.z * V2.z);
}