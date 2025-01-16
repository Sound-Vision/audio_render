#ifndef AUDIO_PLAYOUT_SV_OPENSL_RENDER_H
#define AUDIO_PLAYOUT_SV_OPENSL_RENDER_H

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <string>
#include "sv_common.h"
#include "log.h"

namespace sv_render {

inline const char* GetSLErrorString(size_t code) {
  static const char* sl_error_strings[] = {
          "SL_RESULT_SUCCESS",                 // 0
          "SL_RESULT_PRECONDITIONS_VIOLATED",  // 1
          "SL_RESULT_PARAMETER_INVALID",       // 2
          "SL_RESULT_MEMORY_FAILURE",          // 3
          "SL_RESULT_RESOURCE_ERROR",          // 4
          "SL_RESULT_RESOURCE_LOST",           // 5
          "SL_RESULT_IO_ERROR",                // 6
          "SL_RESULT_BUFFER_INSUFFICIENT",     // 7
          "SL_RESULT_CONTENT_CORRUPTED",       // 8
          "SL_RESULT_CONTENT_UNSUPPORTED",     // 9
          "SL_RESULT_CONTENT_NOT_FOUND",       // 10
          "SL_RESULT_PERMISSION_DENIED",       // 11
          "SL_RESULT_FEATURE_UNSUPPORTED",     // 12
          "SL_RESULT_INTERNAL_ERROR",          // 13
          "SL_RESULT_UNKNOWN_ERROR",           // 14
          "SL_RESULT_OPERATION_ABORTED",       // 15
          "SL_RESULT_CONTROL_LOST",            // 16
  };

  if (code >= arraysize(sl_error_strings)) {
    return "SL_RESULT_UNKNOWN_ERROR";
  }
  return sl_error_strings[code];
}

class SVOpenslRender : public INativeAudioRender {

public:
    explicit SVOpenslRender(const std::string &file_path);
    ~SVOpenslRender() override;
    int InitAudioRender(int sample_rate, int channels) override;
    int StartPlayout() override;
    int StopPlayout() override;

private:
    SV_RESULT CreatePlayerEngine();
    SV_RESULT CreateAudioPlayer();
    SLDataFormat_PCM CreatePCMConfiguration() const;
    static void SimpleBufferQueueCallback(SLAndroidSimpleBufferQueueItf caller, void* context);
    bool ReadPlayoutData();
    bool FillBufferQueue(bool check_state = true);

private:
    bool initialized_ = false;
    bool playing_ = false;
    int sample_rate_ = 0;
    int channels_ = 0;
    int num_of_opensles_buffers_ = 2;
    FILE* file_;

private:
    SLObjectItf sl_object_ { nullptr };
    SLEngineItf sl_engine_ { nullptr };
    SLObjectItf sl_player_object_ { nullptr };
    SLPlayItf sl_player_ { nullptr };
    SLObjectItf  sl_output_mix_ { nullptr };
    SLAndroidSimpleBufferQueueItf  simple_buffer_queue_ { nullptr };
    std::unique_ptr<SLint16[]> audio_buffers_;
};

} // sv_render

#endif //AUDIO_PLAYOUT_SV_OPENSL_RENDER_H
