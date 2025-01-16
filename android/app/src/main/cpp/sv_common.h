#ifndef AUDIO_PLAYOUT_SV_COMMON_H
#define AUDIO_PLAYOUT_SV_COMMON_H

#include <cstdint>
#include <memory>
namespace sv_render {

enum SV_RENDER_TYPE: int16_t {
    UNDEFINED,
    OPENSL,
    AAUDIO,
    OBOE
};

enum SV_RESULT: int16_t {
    SV_NO_ERROR,
    SV_CRATE_ENGINE_ERROR,
    SV_PLAY_STATE_ERROR,
    SV_PLAY_INIT_ERROR,
    SV_START_PLAY_ERROR,
    SV_FILL_BUFFER_ERROR,
    SV_STOP_PLAYER_ERROR
};

#define arraysize(array) (sizeof(ArraySizeHelper(array)))
template <typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];

class INativeAudioRender {
public:
    using Ptr = std::shared_ptr<INativeAudioRender>;
    virtual ~INativeAudioRender() = default;
    virtual int InitAudioRender(int sample_rate, int channels) = 0;
    virtual int StartPlayout() = 0;
    virtual int StopPlayout() = 0;
};

}

#endif //AUDIO_PLAYOUT_SV_COMMON_H
