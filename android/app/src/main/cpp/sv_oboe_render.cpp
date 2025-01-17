#include "sv_oboe_render.h"
#include "log.h"
#include <cassert>

namespace sv_render {

SVOboeRender::SVOboeRender(const std::string& file_path)
: file_(nullptr), initialized_(false), audio_buffers_(nullptr) {
  AV_LOGI("SVOboeRender Construct.");
  file_ = fopen(file_path.c_str(), "rb");
  assert(file_);
}

SVOboeRender::~SVOboeRender() {
  if (file_) {
    fclose(file_);
    file_ = nullptr;
  }
  Result result = stream_->close();
  if (result != Result::OK) {
    AV_LOGW("Oboe stream close failed, reason:%s", convertToText(result));
  }
  stream_ = nullptr;
  initialized_ = false;
  audio_buffers_ = nullptr;
}

bool SVOboeRender::ReadPlayoutData(int num_frames) {
  const size_t per_size = sizeof(int16_t);
  int channels = stream_->getChannelCount();
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

DataCallbackResult SVOboeRender::onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames) {
  AV_LOGI("===== onAudioReady =====");
  int channels = stream_->getChannelCount();
  size_t num_bytes = sizeof(int16_t) * channels * numFrames;
  memset(audioData, 0, num_bytes);
  if (!ReadPlayoutData(numFrames)) {
    AV_LOGW("Read playout data failed.");
    return DataCallbackResult::Stop;
  }
  memcpy(audioData, audio_buffers_.get(), num_bytes);
  return DataCallbackResult::Continue;
}

bool SVOboeRender::onError(AudioStream* audio_stream, Result error) {
  AV_LOGE("Oboe onError: %s", convertToText(error));
  return true; /** true if the stream has been stopped and closed, false if not **/
}

int SVOboeRender::InitAudioRender(int sample_rate, int channels) {
  AV_LOGI("InitAudioRender start.");
  if (initialized_) {
    AV_LOGW("Oboe render has initialized.");
    return SV_NO_ERROR;
  }

  builder_.setDeviceId(kUnspecified);
  builder_.setDirection(Direction::Output);
  builder_.setPerformanceMode(PerformanceMode::LowLatency);
  builder_.setSharingMode(SharingMode::Shared);
  builder_.setFormat(AudioFormat::I16);
  builder_.setChannelCount(channels);
  builder_.setSampleRate(sample_rate);
  builder_.setFormatConversionAllowed(false);
  builder_.setDataCallback(this);
  builder_.setErrorCallback(this);

  Result result = builder_.openStream(stream_);
  if (result != Result::OK) {
    AV_LOGE("Oboe open stream failed, reason: %s", convertToText(result));
    return SV_PLAY_INIT_ERROR;
  }

  const size_t buffer_size_in_samples = sample_rate / 100 * channels;
  audio_buffers_.reset(new int16_t[buffer_size_in_samples]);

  initialized_ = true;
  AV_LOGI("InitAudioRender done.");
  return SV_NO_ERROR;
}

int SVOboeRender::StartPlayout() {
  AV_LOGI("StartPlayout.");
  if (!initialized_) {
    AV_LOGE("Start Playout failed, state is invalid.");
    return SV_PLAY_STATE_ERROR;
  }
  auto state = stream_->getState();
  if (state != StreamState::Open) {
    AV_LOGE("Start Playout failed, not open state.");
    return SV_PLAY_STATE_ERROR;
  }

  auto result = stream_->requestStart();
  if (result != Result::OK) {
    AV_LOGE("Oboe request start failed, reason: %s", convertToText(result));
    return SV_START_PLAY_ERROR;
  }
  AV_LOGI("Start playout end.");
  return SV_NO_ERROR;
}

int SVOboeRender::StopPlayout() {
  AV_LOGI("Stop playout start.");
  if (!initialized_) {
    AV_LOGW("Stop playout failed, invalid state.");
    return SV_PLAY_STATE_ERROR;
  }
  auto state = stream_->getState();
  if (state != StreamState::Started) {
    AV_LOGW("Stop playout failed, not requestStart.");
    return SV_PLAY_STATE_ERROR;
  }
  auto result = stream_->requestStop();
  if (result != Result::OK) {
    AV_LOGE("Oboe request stop failed, reason: %s", convertToText(result));
    return SV_STOP_PLAYER_ERROR;
  }
  AV_LOGI("Stop playout end.");
  return SV_NO_ERROR;
}

} // sv_render