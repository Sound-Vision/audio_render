#ifndef AUDIO_PLAYOUT_SV_AAUDIO_RENDER_H
#define AUDIO_PLAYOUT_SV_AAUDIO_RENDER_H

#include "sv_common.h"
#include <string>

namespace sv_render {

class SVAAudioRender  : public INativeAudioRender {

public:
  explicit SVAAudioRender(const std::string& file_path);
  ~SVAAudioRender();
  int InitAudioRender(int sample_rate, int channels) override;
  int StartPlayout() override;
  int StopPlayout() override;

private:
    FILE *file_;
    bool initialized_;
    bool playing_;
};

} // sv_render

#endif //AUDIO_PLAYOUT_SV_AAUDIO_RENDER_H
