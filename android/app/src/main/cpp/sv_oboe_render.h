#ifndef AUDIO_PLAYOUT_SV_OBOE_RENDER_H
#define AUDIO_PLAYOUT_SV_OBOE_RENDER_H

#include "sv_common.h"
#include <string>
#include <oboe/Oboe.h>
using namespace oboe;

namespace sv_render {

class SVOboeRender :
        public INativeAudioRender, AudioStreamDataCallback, AudioStreamErrorCallback {

public:
    explicit SVOboeRender(const std::string& file_path);
    ~SVOboeRender() override;
    int InitAudioRender(int sample_rate, int channels) override;
    int StartPlayout() override;
    int StopPlayout() override;

private:
    DataCallbackResult onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames) override;
    bool onError(AudioStream*, Result) override;
    bool ReadPlayoutData(int num_frames);
private:
    FILE* file_;
    AudioStreamBuilder builder_;
    std::shared_ptr<AudioStream> stream_;
    bool initialized_;
    std::unique_ptr<int16_t[]> audio_buffers_;
};

} // sv_render

#endif //AUDIO_PLAYOUT_SV_OBOE_RENDER_H
