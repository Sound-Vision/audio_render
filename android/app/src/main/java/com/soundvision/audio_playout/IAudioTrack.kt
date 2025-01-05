package com.soundvision.audio_playout

interface IAudioRender {

    fun initPlayout(sampleRate: Int, channels: Int, streamType: Int): Int

    fun startPlayout(): Int

    fun stopPlayout(): Int
}