#include "WaveHeader.h"

int main()
{
    WaveHeader header(44100/*Hz*/, 16/*bit*/, 1/*MONO*/);
    WaveHeader::recordWAV("test.wav", &header, 10000/*ms*/);
    return 0;
}