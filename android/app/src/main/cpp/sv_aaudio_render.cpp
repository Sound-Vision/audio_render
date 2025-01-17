#include "sv_aaudio_render.h"
#include "log.h"
#include <cassert>

namespace sv_render {

SVAAudioRender::SVAAudioRender(const std::string& file_path)
  : file_(nullptr),
  initialized_(false),
  builder_(nullptr),
  stream_(nullptr) {
  AV_LOGI("SVAAudioRender Construct");
  file_ = fopen(file_path.c_str(), "rb");
  assert(file_);
  AV_LOGI("open file address: %p", file_);
  auto result = AAudio_createStreamBuilder(&builder_);
  if (result != AAUDIO_OK) {
    AV_LOGE("createStreamBuilder failed, reason:%s", AAudio_convertResultToText(result));
  }
  assert(builder_);
}

SVAAudioRender::~SVAAudioRender() {
  AV_LOGI("SVAAudioRender Destruct");
  if (file_) {
    fclose(file_);
    file_ = nullptr;
  }
  AAudioStream_close(stream_);
  stream_ = nullptr;
  builder_ = nullptr;
  audio_buffers_ = nullptr;
  initialized_ = false;
}

bool SVAAudioRender::ReadPlayoutData(int num_frames) {
  const size_t per_size = sizeof(int16_t);
  int channels = AAudioStream_getChannelCount(stream_);
  const size_t buf_size = num_frames * channels;
  auto len = fread(audio_buffers_.get(), per_size, buf_size, file_);
  if (len < buf_size) {
    if (ferror(file_)) {
      AV_LOGW("read file error.");
    }
    if (feof(file_)) {
      AV_LOGW("read file end.");
    }
    fclose(file_);
    return false;
  }
  return true;
}

aaudio_data_callback_result_t
SVAAudioRender::DataCallback(AAudioStream* stream, void* user_data, void* audio_data, int32_t num_frames) {

  auto* render = reinterpret_cast<SVAAudioRender*>(user_data);
  if (render == nullptr) {
    AV_LOGW("Get SVAAudioRender failed, so stop audio data render.");
    return AAUDIO_CALLBACK_RESULT_STOP;
  }

  // memset audio_data.
  int channels = AAudioStream_getChannelCount(stream);
  size_t num_bytes = sizeof(int16_t) * channels * num_frames;
  memset(audio_data, 0, num_bytes);

  if (!render->ReadPlayoutData(num_frames)) {
    AV_LOGW("Read playout data failed.");
    return AAUDIO_CALLBACK_RESULT_STOP;
  }

  memcpy(audio_data, render->audio_buffers_.get(), num_bytes);
  return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void SVAAudioRender::ErrorCallback(AAudioStream* stream, void* user_data, aaudio_result_t error) {
  // handle error.
  AV_LOGE("ErrorCallback error: %s", AAudio_convertResultToText(error));
}

int SVAAudioRender::InitAudioRender(int sample_rate, int channels) {
  AV_LOGI("InitAudioRender");
  // step1: AAudio configuration.
  AAudioStreamBuilder_setDeviceId(builder_, AAUDIO_UNSPECIFIED);
  AAudioStreamBuilder_setSampleRate(builder_, sample_rate);
  AAudioStreamBuilder_setChannelCount(builder_, channels);
  AAudioStreamBuilder_setFormat(builder_, AAUDIO_FORMAT_PCM_I16);
  AAudioStreamBuilder_setSharingMode(builder_, AAUDIO_SHARING_MODE_SHARED);
  AAudioStreamBuilder_setDirection(builder_, AAUDIO_DIRECTION_OUTPUT);
  AAudioStreamBuilder_setPerformanceMode(builder_, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
  AAudioStreamBuilder_setDataCallback(builder_, DataCallback, this);
  AAudioStreamBuilder_setErrorCallback(builder_, ErrorCallback, this);

  // step2: open stream.
  auto result = AAudioStreamBuilder_openStream(builder_, &stream_);
  if (result != AAUDIO_OK) {
    AV_LOGE("AAudio open stream failed, reason:%s", AAudio_convertResultToText(result));
    return SV_PLAY_INIT_ERROR;
  }

  // step3: set buffer.
  int32_t capacity = AAudioStream_getBufferCapacityInFrames(stream_);
  int32_t frames_per_burst = AAudioStream_getFramesPerBurst(stream_);
  AV_LOGI("max buffer capacity: %d, framesPerBurst:%d", capacity, frames_per_burst);

  int32_t buffer_size = AAudioStream_getBufferSizeInFrames(stream_);
  AV_LOGI("buffer_size: %d", buffer_size);

  //根据自己需求，来决定是否要设置缓冲区大小。如果不设置 buffer size 等于 capacity.
//  AAudioStream_setBufferSizeInFrames()

  //step4: allocate buffer.
  const size_t buffer_size_in_samples = sample_rate / 100 * channels;
  audio_buffers_.reset(new int16_t[buffer_size_in_samples]);

  initialized_ = true;
  AV_LOGI("AAudio init done.");
  return SV_NO_ERROR;
}

int SVAAudioRender::StartPlayout() {
  AV_LOGI("AAudio start playout.");
  if (!initialized_) {
    AV_LOGE("AAudio state error, not init.");
    return SV_PLAY_STATE_ERROR;
  }

  auto state = AAudioStream_getState(stream_);
  if (state != AAUDIO_STREAM_STATE_OPEN) {
    AV_LOGE("Invalid state, please open stream first.");
    return SV_START_PLAY_ERROR;
  }

  auto result = AAudioStream_requestStart(stream_);
  if (result != AAUDIO_OK) {
    AV_LOGE("AAudio request start error, reason:%s", AAudio_convertResultToText(result));
    return SV_START_PLAY_ERROR;
  }
  AV_LOGI("AAudio start playout end.");
  return SV_NO_ERROR;
}

int SVAAudioRender::StopPlayout() {
  AV_LOGI("AAudio stop playout start.");
  if (!initialized_) {
    AV_LOGE("AAudio state error, not init.");
    return SV_PLAY_STATE_ERROR;
  }

  auto result = AAudioStream_requestStop(stream_);
  if (result != AAUDIO_OK) {
    AV_LOGE("AAudio request stop failed, reason: %s", AAudio_convertResultToText(result));
    return SV_STOP_PLAYER_ERROR;
  }
  AV_LOGI("AAudio stop playout end.");
  initialized_ = false;
  return SV_NO_ERROR;
}

} // sv_render