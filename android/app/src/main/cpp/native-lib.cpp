#include <jni.h>
#include <string>
#include "sv_common.h"
#include "log.h"
#include "sv_opensl_render.h"

using namespace sv_render;

SV_RENDER_TYPE g_render_type = UNDEFINED;
INativeAudioRender::Ptr g_audio_render = nullptr;

void NativeSetRecordType(JNIEnv *env, jobject obj, jint type, jstring file_path) {
  if (g_render_type != UNDEFINED && g_audio_render) {
    AV_LOGW("please release previous g_audio_render.");
    return;
  }
  const char* c_path = env->GetStringUTFChars(file_path, nullptr);
  std::string path(c_path);
  if (type == OPENSL) {
    g_audio_render = std::make_shared<SVOpenslRender>(path);
  }
  g_render_type = static_cast<SV_RENDER_TYPE>(type);
  AV_LOGI("Reset render type %d", g_render_type);
  env->ReleaseStringUTFChars(file_path, c_path);
}

jint NativeInitRecording(JNIEnv *env, jobject obj, jint sample_rate, jint channels) {
  if (g_audio_render) {
    auto result = g_audio_render->InitAudioRender(sample_rate, channels);
    if (result != SV_NO_ERROR)
      return JNI_ERR;
  }
  return JNI_OK;
}

jint NativeStartRecording(JNIEnv *env, jobject obj) {
  if (g_audio_render) {
    auto result = g_audio_render->StartPlayout();
    if (result != SV_NO_ERROR)
      return JNI_ERR;
  }
  return JNI_OK;
}

jint NativeStopRecording(JNIEnv *env, jobject obj) {
  if (g_audio_render) {
    auto result = g_audio_render->StopPlayout();
    if (result != SV_NO_ERROR)
      return JNI_ERR;
  }
  g_audio_render = nullptr;
  g_render_type = UNDEFINED;
  return JNI_OK;
}

static JNINativeMethod gMethods[] = {
        {"nativeSetRenderType", "(ILjava/lang/String;)V", (void*) NativeSetRecordType},
        {"nativeInitRender", "(II)I", (void*) NativeInitRecording},
        {"nativeStartPlayout", "()I", (void*) NativeStartRecording},
        {"nativeStopPlayout", "()I", (void*) NativeStopRecording},
};

static const char* className = "com/soundvision/audio_playout/SVNativeAudioRender";

static int registerNativeMethods(JNIEnv* env) {
  jclass clazz;
  clazz = env->FindClass(className);

  if(!clazz) return JNI_FALSE;
  if(env->RegisterNatives(clazz, gMethods, sizeof(gMethods)/sizeof(gMethods[0])) < 0) {
    return JNI_FALSE;
  }
  return JNI_TRUE;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  JNIEnv* env = nullptr;
  if(vm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK) {
    return JNI_ERR;
  }
  if(registerNativeMethods(env) != JNI_TRUE) {
    return JNI_ERR;
  }
  return JNI_VERSION_1_4;
}