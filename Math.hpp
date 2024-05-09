#pragma once

#include <cmath>

constexpr double UCONST_Pi = 3.1415926;
constexpr double URotationToRadians = UCONST_Pi / 180.0;

struct Vector3 {
    double x, y, z;
};

double CalculateDistance(const Vector3& a, const Vector3& b) {
    double distX = b.x - a.x;
    double distY = b.y - a.y;
    double distZ = b.z - a.z;

    return std::sqrt(distX * distX + distY * distY + distZ * distZ);
}

struct Vector2 {
    double x, y;
};

struct Vector4 {
    double x, y, z, w;
};

struct FRotator {
    double pitch, yaw, roll;
};

Vector3 RotationToVector(FRotator R) {
    Vector3 Vec;
    double fYaw = R.yaw * URotationToRadians;
    double fPitch = R.pitch * URotationToRadians;
    double CosPitch = cos(fPitch);
    Vec.x = cos(fYaw) * CosPitch;
    Vec.y = sin(fYaw) * CosPitch;
    Vec.z = sin(fPitch);

    return Vec;
}

double Vector3Size(Vector3& v) {
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vector3 Vector3Scale(Vector3& v, double scalar) {
    return Vector3{ v.x * scalar, v.y * scalar, v.z * scalar };
}

void Vector3Normalize(Vector3& v) {
    double size = Vector3Size(v);

    if (!size)
        v.x = v.y = v.z = 1;
    else
    {
        v.x /= size;
        v.y /= size;
        v.z /= size;
    }
}

Vector3 Vector3Add(Vector3 va, Vector3 vb) {
    Vector3 out;

    out.x = va.x + vb.x;
    out.y = va.y + vb.y;
    out.z = va.z + vb.z;

    return out;
}

Vector3 Vector3Subtract(Vector3 va, Vector3 vb) {
    Vector3 out;

    out.x = va.x - vb.x;
    out.y = va.y - vb.y;
    out.z = va.z - vb.z;

    return out;
}

double Vector3Distance(const Vector3& va, const Vector3& vb) {
    double dx = va.x - vb.x;
    double dy = va.y - vb.y;
    double dz = va.z - vb.z;

    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

double inline Vector3Dot(const Vector3& V1, const Vector3& V2) {
    return (V1.x * V2.x + V1.y * V2.y + V1.z * V2.z);
}