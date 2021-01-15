#include <alsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <iostream>

#include "WaveHeader.h"

#define RIFF_MARKER "RIFF"
#define FILETYPE_HEADER "WAVE"
#define FORMAT_MARKER "fmt "
#define DATA_MARKER "data"

WaveHeader::WaveHeader(uint32_t _sample_rate, uint16_t _bit_depth, uint16_t _channels) :
    sample_rate(_sample_rate),
    bits_per_sample(_bit_depth),
    number_of_channels(_channels)
{
    data_header_length = 16;
    format_type = 1;
    bytes_per_second = sample_rate * _channels * _bit_depth / 8;
    bytes_per_frame = _channels * _bit_depth / 8;
}

int WaveHeader::writeWAVHeader(int fd, WaveHeader* header)
{
    if (!header)
    {
        return -1;
    }

    int ret;
    ret = write(fd, RIFF_MARKER, 4);
    ret = write(fd, &header->file_size, 4);
    ret = write(fd, FILETYPE_HEADER, 4);
    ret = write(fd, FORMAT_MARKER, 4);
    ret = write(fd, &header->data_header_length, 4);
    ret = write(fd, &header->format_type, 2);
    ret = write(fd, &header->number_of_channels, 2);
    ret = write(fd, &header->sample_rate, 4);
    ret = write(fd, &header->bytes_per_second, 4);
    ret = write(fd, &header->bytes_per_frame, 2);
    ret = write(fd, &header->bits_per_sample, 2);
    ret = write(fd, DATA_MARKER, 4);

    uint32_t data_size = header->file_size - 36;
    ret = write(fd, &data_size, 4);
    return (ret != -1) ? 0 : ret;
}

int WaveHeader::writeWAVHeader(void* file, WaveHeader* header)
{
    if (!header || !file)
    {
        return -1;
    }

    FILE* tmp = (FILE*)file;
    fwrite(RIFF_MARKER, 4, 1, tmp);
    fwrite(&header->file_size, 4, 1, tmp);
    fwrite(FILETYPE_HEADER, 4, 1, tmp);
    fwrite(FORMAT_MARKER, 4, 1, tmp);
    fwrite(&header->data_header_length, 4, 1, tmp);
    fwrite(&header->format_type, 2, 1, tmp);
    fwrite(&header->number_of_channels, 2, 1, tmp);
    fwrite(&header->sample_rate, 4, 1, tmp);
    fwrite(&header->bytes_per_second, 4, 1, tmp);
    fwrite(&header->bytes_per_frame, 2, 1, tmp);
    fwrite(&header->bits_per_sample, 2, 1, tmp);
    fwrite(DATA_MARKER, 4, 1, tmp);

    uint32_t data_size = header->file_size - 36;
    fwrite(&data_size, 4, 1, tmp);

    return 0;
}

static inline int initPCM(snd_pcm_t*& handle, const WaveHeader* header, uint32_t& sample_rate, snd_pcm_uframes_t& frames)
{
    int err;
    snd_pcm_hw_params_t *params;
    const char *device = "plughw:2,0"; // USB microphone
    // const char *device = "default"; // Integrated system microphone

    /* Open PCM device for recording (capture). */
    err = snd_pcm_open(&handle, device, SND_PCM_STREAM_CAPTURE, 0);
    if (err)
    {
        printf("Unable to open PCM device: %s\n", snd_strerror(err));
        return err;
    }

    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(&params);

    /* Fill it in with default values. */
    snd_pcm_hw_params_any(handle, params);

    /* ### Set the desired hardware parameters. ### */

    /* Interleaved mode */
    err = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err)
    {
        fprintf(stderr, "Error setting interleaved mode: %s\n", snd_strerror(err));
        snd_pcm_close(handle);
        return err;
    }
    /* Signed 16-bit little-endian format */
    err = snd_pcm_hw_params_set_format(handle, params, (header->getBitsPerSample() == 16) ? SND_PCM_FORMAT_S16_LE : SND_PCM_FORMAT_U8);
    if (err)
    {
        fprintf(stderr, "Error setting format: %s\n", snd_strerror(err));
        snd_pcm_close(handle);
        return err;
    }
    /* two channels (stereo) or one (mono) */
    err = snd_pcm_hw_params_set_channels(handle, params, header->getNumberOfChannels() );
    if (err)
    {
        fprintf(stderr, "Error setting channels: %s\n", snd_strerror(err));
        snd_pcm_close(handle);
        return err;
    }
    /* 44100 bits/second sampling rate (CD quality) */
    err = snd_pcm_hw_params_set_rate_near(handle, params, &sample_rate, NULL);
    if (err)
    {
        fprintf(stderr, "Error setting sampling rate (%d): %s\n", sample_rate, snd_strerror(err));
        snd_pcm_close(handle);
        return err;
    }
    /* Set period size*/
    err = snd_pcm_hw_params_set_period_size_near(handle, params, &frames, NULL);
    if (err)
    {
        fprintf(stderr, "Error setting period size: %s\n", snd_strerror(err));
        snd_pcm_close(handle);
        return err;
    }
    /* Write the parameters to the driver */
    err = snd_pcm_hw_params(handle, params);
    if (err < 0)
    {
        fprintf(stderr, "Unable to set HW parameters: %s\n", snd_strerror(err));
        snd_pcm_close(handle);
        return err;
    }
}

int WaveHeader::recordWAV(const char* fileName, WaveHeader* header, uint32_t duration)
{
    if(!header)
    {
        return -1;
    }

    int err;
    size_t size;
    snd_pcm_t *handle;
    snd_pcm_uframes_t frames = 32UL;
    char *buffer;

    err = initPCM(handle, header, header->sample_rate, frames);
    if(err)
    {
        return err;
    }

    size = frames * header->bits_per_sample / 8 * header->number_of_channels; /* 2 bytes/sample, 2 channels */
    buffer = (char*) malloc(size);
    if (!buffer)
    {
        fprintf(stderr, "Buffer error.\n");
        snd_pcm_close(handle);
        return -1;
    }

    uint32_t pcm_data_size = header->sample_rate * header->bytes_per_frame * (duration / 1000);
    header->file_size = pcm_data_size + 36;

    // filedesc = open(fileName, O_WRONLY | O_CREAT, 0644);
    // err = writeWAVHeader(filedesc, hdr);

    FILE* wavFile = fopen(fileName, "w");
    if(NULL == wavFile)
    {
        printf("fopen failed | error %s", strerror(errno));
        return errno;
    }
    err = writeWAVHeader(wavFile, header);
    if (err)
    {
        printf("Error writing .wav header.");
        snd_pcm_close(handle);
        free(buffer);
        // close(filedesc);
        fclose(wavFile);
        return err;
    }

    int totalFrames = 0;
    int timeToExceed = ((duration * 1000) / (header->sample_rate / frames)); 
    std::cout << "time to exceed: " << timeToExceed << std::endl;
    for(int i = timeToExceed; i > 0; i--)
    {
        err = snd_pcm_readi(handle, buffer, frames);
        totalFrames += err;
        if (err < 0)
        {   
            if (err == -EPIPE) 
            {
                printf("Overrun occurred: %d\n", err);
            }
            err = snd_pcm_recover(handle, err, 0);
            // Still an error, need to exit.
            if (err < 0)
            {
                printf("Error occured while recording: %s\n", snd_strerror(err));
                snd_pcm_close(handle);
                free(buffer);
                // close(filedesc);
                fclose(wavFile);
                return err;
            }
        }
        // write(filedesc, buffer, size);
        fwrite(buffer, size, 1, wavFile);
    }

    std::cout << "total frames: " << totalFrames << std::endl;

    // close(filedesc);
    fclose(wavFile);
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(buffer);
    return 0;
}

uint16_t WaveHeader::getBitsPerSample() const
{
    return bits_per_sample;
}

uint16_t WaveHeader::getNumberOfChannels() const
{
    return number_of_channels;
}