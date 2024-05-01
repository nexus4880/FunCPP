#pragma once

#include <cstdint>
#include <string>

struct MilkRenColor {
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;

    MilkRenColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) :
        r(r), g(g), b(b), a(a)
    {
    }

    MilkRenColor(uint32_t value) {
        r = static_cast<uint8_t>((value >> 16) & 0xFF);
        g = static_cast<uint8_t>((value >> 8) & 0xFF);
        b = static_cast<uint8_t>(value & 0xFF);
        a = static_cast<uint8_t>((value >> 24) & 0xFF);
    }
};

struct MilkRenRectangle {
    float x, y, width, height;
    MilkRenColor color;
};

struct MilkRenCircle {
    float x, y, radius;
    MilkRenColor color;
};

struct MilkRenLabel {
    std::wstring text;
    float x, y, size;
    MilkRenColor color;
};

struct MilkRenLine {
    float x1, y1, x2, y2;
    MilkRenColor color;
};