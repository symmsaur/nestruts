#pragma once

#include <span>
#include <cstdint>
#include <stdexcept>

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_error.h>

#include "log.h"

// Audio interface stuff

class audio_backend {
    SDL_AudioDeviceID device;

  public:
    audio_backend() {
        SDL_InitSubSystem(SDL_INIT_AUDIO);
        SDL_AudioSpec spec{};
        spec.freq = 44100;
        spec.format = AUDIO_S16SYS;
        spec.channels = 1;
        spec.samples = 1024;
        spec.callback = nullptr;
        device = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);
        if (device == 0) {
            log(log_level::error, "{}\n", SDL_GetError());
            throw new std::runtime_error("Failed to open audio device");
        }
        SDL_PauseAudioDevice(device, 0);
    }

    ~audio_backend() {
        SDL_CloseAudioDevice(device);
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }

    int required_samples() {
        // Aim for 1 / 30 s (two frames) of buffered data.
        return (1470 * 2 - SDL_GetQueuedAudioSize(device)) / 2;
    }

    int sample_rate_hz() { return 44100; }

    void queue(std::span<std::int16_t> samples) {
        SDL_QueueAudio(device, samples.data(), samples.size_bytes());
    }
};
