#pragma once

#include "MilkRen.hpp"
#include <vector>

class Payload {
public:
    Payload(size_t bufferSize) : initialized(false), rectangles{}, circles{},
        labels{}, lines{}, m_buffer(new char[bufferSize]), m_bufferSize(bufferSize)
    {
        memset(m_buffer, 0, bufferSize);
    }

    ~Payload() {
        delete[] m_buffer;
    }

    bool initialized;
    std::vector<MilkRenRectangle> rectangles;
    std::vector<MilkRenCircle> circles;
    std::vector<MilkRenLabel> labels;
    std::vector<MilkRenLine> lines;

    void Reset() {
        initialized = false;
        rectangles.clear();
        circles.clear();
        labels.clear();
        lines.clear();
    }

    void ClearVectors() {
        rectangles.clear();
        circles.clear();
        labels.clear();
        lines.clear();
    }

    const char* Serialize(size_t& position) {
        position = 0;

        Write<uint8_t>(position, initialized);

        if (initialized) {
            auto rectanglesCount = static_cast<uint16_t>(rectangles.size());
            auto circlesCount = static_cast<uint16_t>(circles.size());
            auto labelsCount = static_cast<uint16_t>(labels.size());
            auto linesCount = static_cast<uint16_t>(lines.size());

            Write<uint16_t>(position, rectanglesCount);
            Write<uint16_t>(position, circlesCount);
            Write<uint16_t>(position, labelsCount);
            Write<uint16_t>(position, linesCount);

            for (uint16_t i = 0; i < rectanglesCount; i++) {
                Write<MilkRenRectangle>(position, rectangles[i]);
            }

            for (uint16_t i = 0; i < circlesCount; i++) {
                Write<MilkRenCircle>(position, circles[i]);
            }

            for (uint16_t i = 0; i < labelsCount; i++) {
                auto& label = labels[i];
                auto textLength = static_cast<uint8_t>(label.text.length());
                Write<uint8_t>(position, textLength);
                WriteBlock(position, label.text.data(), textLength * sizeof(wchar_t));
                Write<float>(position, label.x);
                Write<float>(position, label.y);
                Write<float>(position, label.size);
                Write<MilkRenColor>(position, label.color);
            }

            for (uint16_t i = 0; i < linesCount; i++) {
                Write<MilkRenLine>(position, lines[i]);
            }
        }

        return m_buffer;
    }

private:
    char* m_buffer;
    size_t m_bufferSize;

    template <typename T>
    void Write(size_t& position, const T& value) {
        *(T*)(m_buffer + position) = value;
        position += sizeof(T);
    }

    void WriteBlock(size_t& position, const void* buffer, size_t size) {
        memcpy(m_buffer + position, buffer, size);
        position += size;
    }
};
