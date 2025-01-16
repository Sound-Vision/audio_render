#include "sv_aaudio_render.h"
#include "log.h"
#include <cassert>

namespace sv_render {

SVAAudioRender::SVAAudioRender(const std::string& file_path)
: file_(nullptr), initialized_(false), playing_(false) {
  AV_LOGI("SVAAudioRender Construct");
  file_ = fopen(file_path.c_str(), "rb");
  assert(file_);
  AV_LOGI("open file address: %p", file_);
}

SVAAudioRender::~SVAAudioRender() {
  AV_LOGI("SVAAudioRender Destruct");
}

int SVAAudioRender::InitAudioRender(int sample_rate, int channels) {
  return 0;
}

int SVAAudioRender::StartPlayout() {
  return 0;
}

int SVAAudioRender::StopPlayout() {
  return 0;
}

} // sv_render