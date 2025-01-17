#ifndef AUDIO_PLAYOUT_SV_AAUDIO_RENDER_H
#define AUDIO_PLAYOUT_SV_AAUDIO_RENDER_H

#include "sv_common.h"
#include <string>
#include <aaudio/AAudio.h>

namespace sv_render {

class SVAAudioRender  : public INativeAudioRender {

public:
  explicit SVAAudioRender(const std::string& file_path);
  ~SVAAudioRender();
  int InitAudioRender(int sample_rate, int channels) override;
  int StartPlayout() override;
  int StopPlayout() override;

private:
  static aaudio_data_callback_result_t DataCallback(AAudioStream* stream, void* user_data, void* audio_data, int32_t num_frames);
  static void ErrorCallback(AAudioStream* stream, void* user_data, aaudio_result_t error);
  bool ReadPlayoutData(int num_frames);

private:
  AAudioStreamBuilder *builder_;
  AAudioStream* stream_;
  FILE *file_;
  bool initialized_;
  std::unique_ptr<int16_t[]> audio_buffers_;
};

} // sv_render

#endif //AUDIO_PLAYOUT_SV_AAUDIO_RENDER_H
