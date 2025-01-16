#include "sv_opensl_render.h"
#include <memory>

namespace sv_render {

SVOpenslRender::SVOpenslRender(const std::string &file_path): file_(nullptr) {
  AV_LOGI("SVOpenslRender Constructor.");
  file_ = fopen(file_path.c_str(), "rb");
  AV_LOGI("open file address: %p", file_);
  CreatePlayerEngine();
}

SVOpenslRender::~SVOpenslRender() {
  AV_LOGI("SVOpenslRender Deconstruct");
}

int SVOpenslRender::InitAudioRender(int sample_rate, int channels) {

  AV_LOGI("SVOpenslRender init.");
  if (initialized_) {
    AV_LOGW("SVOpenslRender not initialized.");
    return SV_PLAY_STATE_ERROR;
  }

  if (!sl_engine_) {
    AV_LOGW("sl_engine_ is nullptr.");
    return SV_PLAY_INIT_ERROR;
  }

  // Allocate audio buffer.
  const size_t buffer_size_in_samples = sample_rate / 100 * channels;
  audio_buffers_.reset(new SLint16[buffer_size_in_samples]);

  SLresult  result = (*sl_engine_)->CreateOutputMix(sl_engine_, &sl_output_mix_, 0, nullptr, nullptr);
  if (result != SL_RESULT_SUCCESS) {
    AV_LOGW("CreateOutputMix failed, reason: %s", GetSLErrorString(result));
    return SV_PLAY_INIT_ERROR;
  }

  result = (*sl_output_mix_)->Realize(sl_output_mix_, SL_BOOLEAN_FALSE);
  if (result != SL_RESULT_SUCCESS) {
    AV_LOGW("sl_output_mix Realize failed, reason:%s", GetSLErrorString(result));
    return SV_PLAY_INIT_ERROR;
  }

  sample_rate_ = sample_rate;
  channels_ = channels;
  initialized_ = true;
  AV_LOGI("InitAudioRender done.");
  return SV_NO_ERROR;
}

int SVOpenslRender::StartPlayout() {

  if (!initialized_ || playing_) {
    AV_LOGW("SVOpenslRender StartPlayout state error.");
    return SV_PLAY_STATE_ERROR;
  }

  if (CreateAudioPlayer() != SL_RESULT_SUCCESS) {
    AV_LOGW("Create Audio player error.");
    return SV_START_PLAY_ERROR;
  }

  if (!FillBufferQueue(false)) {
    return SV_FILL_BUFFER_ERROR;
  }

  auto result = (*sl_player_)->SetPlayState(sl_player_, SL_PLAYSTATE_PLAYING);
  if (result != SL_RESULT_SUCCESS) {
    AV_LOGW("Set playing state failed.");  // Maybe permission problem.
    return SV_START_PLAY_ERROR;
  }

  SLuint32 play_state = SL_PLAYSTATE_STOPPED;
  (*sl_player_)->GetPlayState(sl_player_, &play_state);

  playing_ = (play_state == SL_PLAYSTATE_PLAYING);
  return SV_NO_ERROR;
}

int SVOpenslRender::StopPlayout() {
  AV_LOGI("StopPlayout start.");
  if (!initialized_ || !playing_) {
    return SV_PLAY_STATE_ERROR;
  }
  if ((*sl_player_)->SetPlayState(sl_player_, SL_PLAYSTATE_STOPPED) != SL_RESULT_SUCCESS) {
    AV_LOGW("Stop Playout setPlayState error.");
    return SV_STOP_PLAYER_ERROR;
  }
  if ((*simple_buffer_queue_)->Clear(simple_buffer_queue_) != SL_RESULT_SUCCESS) {
    AV_LOGW("Stop Playout clear bufferQueue error. ");
    return SV_STOP_PLAYER_ERROR;
  }

  (*simple_buffer_queue_)->RegisterCallback(simple_buffer_queue_, nullptr, nullptr);
  (*sl_player_object_)->Destroy(sl_player_object_);
  (*sl_object_)->Destroy(sl_object_);
  sl_player_ = nullptr;
  sl_player_object_ = nullptr;
  sl_engine_ = nullptr;
  sl_object_ = nullptr;
  playing_ = false;
  initialized_ = false;
  AV_LOGI("StopPlayout end.");
  return SV_NO_ERROR;
}

SV_RESULT SVOpenslRender::CreatePlayerEngine() {

   const SLEngineOption option[] = {
           {SL_ENGINEOPTION_THREADSAFE, static_cast<SLuint32>(SL_BOOLEAN_TRUE)}};
   SLresult result = slCreateEngine(&sl_object_, 1, option, 0, nullptr, nullptr);
   if (result != SL_RESULT_SUCCESS) {
     AV_LOGE("slCreateEngine failed, reason:%s", GetSLErrorString(result));
     return SV_CRATE_ENGINE_ERROR;
   }

  result = (*sl_object_)->Realize(sl_object_, SL_BOOLEAN_FALSE);
  if (result != SL_RESULT_SUCCESS) {
    AV_LOGE("sl_object realize failed, reason:%s", GetSLErrorString(result));
    return SV_CRATE_ENGINE_ERROR;
  }

  result = (*sl_object_)->GetInterface(sl_object_, SL_IID_ENGINE, &sl_engine_);
  if (result != SL_RESULT_SUCCESS) {
    AV_LOGE("sl_object GetInterface failed, reason:%s", GetSLErrorString(result));
    return SV_CRATE_ENGINE_ERROR;
  }

  return SV_NO_ERROR;
 }

void SVOpenslRender::SimpleBufferQueueCallback(SLAndroidSimpleBufferQueueItf caller, void* context) {
  auto* stream = reinterpret_cast<SVOpenslRender*>(context);
  if (stream) stream->FillBufferQueue();
}

bool SVOpenslRender::FillBufferQueue(bool check_state) {
  if (check_state) {
    SLuint32  state = SL_PLAYSTATE_STOPPED;
    (*sl_player_)->GetPlayState(sl_player_, &state);
    if (state != SL_PLAYSTATE_PLAYING) {
      AV_LOGW("FillBufferQueue failed, state is error.");
      return false;
    }
  }
  if (!ReadPlayoutData()) {
    AV_LOGW("FillBufferQueue failed, read playout data error.");
    return false;
  }
  auto * binary_data = reinterpret_cast<SLint8 *>(audio_buffers_.get());
  size_t size =  sample_rate_ / 100 * channels_ * 2;
  auto result = (*simple_buffer_queue_)->Enqueue(simple_buffer_queue_, binary_data, size);
  if (result != SL_RESULT_SUCCESS) {
    AV_LOGE("Enqueue failed: %s", GetSLErrorString(result));
    return false;
  }
  return true;
}

bool SVOpenslRender::ReadPlayoutData() {
  const size_t per_size = sizeof(SLint16);
  const size_t buf_size = sample_rate_ / 100 * channels_;
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

SLDataFormat_PCM SVOpenslRender::CreatePCMConfiguration() const {

  SLDataFormat_PCM format;
  format.formatType = SL_DATAFORMAT_PCM;
  format.numChannels = static_cast<SLuint32>(channels_);
  switch (sample_rate_) {
    case 8000:
      format.samplesPerSec = SL_SAMPLINGRATE_8;
      break;
    case 16000:
      format.samplesPerSec = SL_SAMPLINGRATE_16;
      break;
    case 22050:
      format.samplesPerSec = SL_SAMPLINGRATE_22_05;
      break;
    case 24000:
      format.samplesPerSec = SL_SAMPLINGRATE_24;
      break;
    case 32000:
      format.samplesPerSec = SL_SAMPLINGRATE_32;
      break;
    case 44100:
      format.samplesPerSec = SL_SAMPLINGRATE_44_1;
      break;
    case 48000:
      format.samplesPerSec = SL_SAMPLINGRATE_48;
      break;
    case 64000:
      format.samplesPerSec = SL_SAMPLINGRATE_64;
      break;
    case 88200:
      format.samplesPerSec = SL_SAMPLINGRATE_88_2;
      break;
    case 96000:
      format.samplesPerSec = SL_SAMPLINGRATE_96;
      break;
    default:
      AV_LOGW("Unsupported sample rate: %d ", sample_rate_);
      break;
  }
  format.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
  format.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
  format.endianness = SL_BYTEORDER_LITTLEENDIAN;
  if (format.numChannels == 1) {
    format.channelMask = SL_SPEAKER_FRONT_CENTER;
  } else if (format.numChannels == 2) {
    format.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
  } else {
    AV_LOGI("Unsupported number of channels: %d", channels_);
  }
  return format;
}

 SV_RESULT SVOpenslRender::CreateAudioPlayer() {
   AV_LOGI("CreateAudioPlayer channels:%d, sample_rate:%d", channels_, sample_rate_);
   if (!sl_output_mix_) {
     AV_LOGW("sl_output_mix_ is nullptr.");
     return SV_PLAY_INIT_ERROR;
   }

   if (sl_player_object_) {
     return SV_NO_ERROR;
   }

   auto pcm_format = CreatePCMConfiguration();
   //source.
   SLDataLocator_AndroidSimpleBufferQueue simple_buffer_queue = {
           SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
           static_cast<SLuint32>(num_of_opensles_buffers_)};
   SLDataSource audio_source = {&simple_buffer_queue, &pcm_format};

   // sink.
   SLDataLocator_OutputMix locator_output_mix = {SL_DATALOCATOR_OUTPUTMIX,
                                                 sl_output_mix_};
   SLDataSink audio_sink = {&locator_output_mix, nullptr};

   const SLInterfaceID interface_ids[] = {SL_IID_ANDROIDCONFIGURATION,
                                          SL_IID_BUFFERQUEUE, SL_IID_VOLUME};
   const SLboolean interface_required[] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
                                           SL_BOOLEAN_TRUE};

   SLresult result = (*sl_engine_)->CreateAudioPlayer(sl_engine_, &sl_player_object_, &audio_source, &audio_sink,
                                    arraysize(interface_ids), interface_ids, interface_required);
   if (result != SL_RESULT_SUCCESS) {
     AV_LOGE("CreateAudioPlayer failed, reason: %s", GetSLErrorString(result));
     return SV_PLAY_INIT_ERROR;
   }

   // ===  Android platform configuration. ====
   SLAndroidConfigurationItf player_config;
   result = (*sl_player_object_)->GetInterface(sl_player_object_, SL_IID_ANDROIDCONFIGURATION, &player_config);
   if (result != SL_RESULT_SUCCESS) {
     AV_LOGE("sl_player_object_ GetInterface failed, reason:%s", GetSLErrorString(result));
     return SV_PLAY_INIT_ERROR;
   }
   SLint32  stream_type = SL_ANDROID_STREAM_MEDIA;
   result = (*player_config)->SetConfiguration(player_config, SL_ANDROID_KEY_STREAM_TYPE, &stream_type, sizeof(SLint32));
   if (result != SL_RESULT_SUCCESS) {
     AV_LOGE("Set android stream_type failed, reason:%s", GetSLErrorString(result));
     return SV_PLAY_INIT_ERROR;
   }

   SLint32 performance_mode = SL_ANDROID_PERFORMANCE_LATENCY;
   result = (*player_config)->SetConfiguration(player_config, SL_ANDROID_KEY_PERFORMANCE_MODE, &performance_mode, sizeof(SLuint32));
   if (result != SL_RESULT_SUCCESS) {
     AV_LOGE("set performance mode failed, reason:%s", GetSLErrorString(result));
     return SV_PLAY_INIT_ERROR;
   }

   result = (*sl_player_object_)->Realize(sl_player_object_, SL_BOOLEAN_FALSE);
   if (result != SL_RESULT_SUCCESS) {
     AV_LOGE("sl_player_object Realize failed, reason: %s", GetSLErrorString(result));
     return SV_PLAY_INIT_ERROR;
   }

   // ==== Get player. ====
   result = (*sl_player_object_)->GetInterface(sl_player_object_, SL_IID_PLAY, &sl_player_);
   if (result != SL_RESULT_SUCCESS) {
     AV_LOGE("sl_player_object GetPlayer failed, reason: %s", GetSLErrorString(result));
     return SV_PLAY_INIT_ERROR;
   }

   // === Get BufferQueue ====
   result = (*sl_player_object_)->GetInterface(sl_player_object_, SL_IID_BUFFERQUEUE, &simple_buffer_queue_);
   if (result != SL_RESULT_SUCCESS) {
     AV_LOGE("sl_player_object GetBufferQueue failed, reason: %s", GetSLErrorString(result));
     return SV_PLAY_INIT_ERROR;
   }

   result = (*simple_buffer_queue_)->RegisterCallback(simple_buffer_queue_, SimpleBufferQueueCallback, this);
   if (result != SL_RESULT_SUCCESS) {
     AV_LOGE("SimpleBufferQueue RegisterCallback failed, reason:%s", GetSLErrorString(result));
     return SV_PLAY_INIT_ERROR;
   }
   AV_LOGI("CreateAudioPlayer done.");
   return SV_NO_ERROR;
}


} // sv_render