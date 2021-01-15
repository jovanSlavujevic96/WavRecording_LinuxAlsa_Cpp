#pragma once

#include <cstdint>

class WaveHeader
{
public:
    WaveHeader() = delete;
    WaveHeader(uint32_t _sample_rate, uint16_t _bit_depth, uint16_t _channels);
    ~WaveHeader() = default;

    static int writeWAVHeader(int fd, WaveHeader *header);
    static int writeWAVHeader(void* file, WaveHeader *header);
    static int recordWAV(const char *fileName, WaveHeader *header, uint32_t duration);

    uint16_t getBitsPerSample() const;
    uint16_t getNumberOfChannels() const;

private:
    uint32_t file_size;
    uint32_t data_header_length;
    uint16_t format_type;
    uint16_t number_of_channels;
    uint32_t sample_rate;
    uint32_t bytes_per_second;
    uint16_t bytes_per_frame;
    uint16_t bits_per_sample;
};

