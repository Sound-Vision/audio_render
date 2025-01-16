package com.soundvision.audio_playout

import android.app.Application

const val BITS_PER_SAMPLE = 16
const val CALLBACK_BUFFER_SIZE_MS = 10
const val BUFFERS_PER_SECOND = 1000 / CALLBACK_BUFFER_SIZE_MS
const val SAMPLE_RATE = 44100
const val CHANNELS = 2

enum class ErrorCode {
    NO_ERROR,
    INIT_ERROR,
    START_ERROR,
    STOP_ERROR,
}

var context: Application ?= null