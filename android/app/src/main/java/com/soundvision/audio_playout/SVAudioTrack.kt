package com.soundvision.audio_playout

import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack
import android.os.Process
import android.util.Log
import java.io.InputStream
import java.nio.ByteBuffer

class SVAudioTrack private constructor() : IAudioRender {

    private val tag: String = "SVAudioTrack"
    private var byteBuffer: ByteBuffer? = null
    private var audioTrack: AudioTrack? = null
    private var audioThread: AudioTrackThread? = null

    companion object {
        val instance: SVAudioTrack by lazy {
            SVAudioTrack()
        }
    }

    override fun initPlayout(sampleRate: Int, channels: Int, streamType: Int): Int {
        Log.i(this.tag, "initPlayout(sampleRate=$sampleRate, channels=$channels)")
        val bytesPerFrame = channels * BITS_PER_SAMPLE / 8
        byteBuffer = ByteBuffer.allocateDirect(bytesPerFrame * (sampleRate / BUFFERS_PER_SECOND))
        Log.i(this.tag, "byteBuffer.capacity: ${byteBuffer?.capacity()}")
        val channelConfig = channelCountToConfiguration(channels)
        var minBufferSizeInBytes = AudioTrack.getMinBufferSize(sampleRate, channelConfig, AudioFormat.ENCODING_PCM_16BIT)
        Log.i(this.tag, "AudioTrack.getMinBufferSize:$minBufferSizeInBytes")

        if (minBufferSizeInBytes < byteBuffer!!.capacity()) {
            Log.i(this.tag, "AudioTrack.getMinBufferSize returns an invalid value.")
            return ErrorCode.INIT_ERROR.ordinal
        }

        if (audioTrack != null) {
            Log.i(this.tag, "AudioTrack obj is exist, state: ${audioTrack?.state}")
            return ErrorCode.INIT_ERROR.ordinal
        }

        //Create AudioTrack.
        val nativeOutputSampleRate = AudioTrack.getNativeOutputSampleRate(streamType)
        Log.w(this.tag, "nativeOutputSampleRate:$nativeOutputSampleRate,You select sampleRate:$sampleRate")
        val usageAttribute = AudioAttributes.USAGE_MEDIA
        val contentType = AudioAttributes.CONTENT_TYPE_MUSIC
        audioTrack = AudioTrack(AudioAttributes.Builder().setUsage(usageAttribute).setContentType(contentType).build(),
            AudioFormat.Builder().setEncoding(AudioFormat.ENCODING_PCM_16BIT).setSampleRate(sampleRate).setChannelMask(channelConfig).build(),
            minBufferSizeInBytes, AudioTrack.MODE_STREAM, AudioManager.AUDIO_SESSION_ID_GENERATE)
        return ErrorCode.NO_ERROR.ordinal
    }

    private fun channelCountToConfiguration(channels: Int): Int {
        return (if (channels == 1) AudioFormat.CHANNEL_OUT_MONO else AudioFormat.CHANNEL_OUT_STEREO)
    }

    override fun startPlayout(): Int {
        Log.d(this.tag, "startPlayout")
        runCatching {
            if (audioTrack == null || audioThread != null) {
                Log.w(this.tag, "startPlayout, but state error.")
                return ErrorCode.START_ERROR.ordinal
            }
            audioTrack!!.play()
        }.let {
            if (it.isFailure) {
                Log.w(this.tag, "startPlayout failed. reason:${it.exceptionOrNull()?.printStackTrace()}")
                return ErrorCode.START_ERROR.ordinal
            }
        }

        audioThread = AudioTrackThread()
        audioThread!!.start()
        return ErrorCode.NO_ERROR.ordinal
    }

    override fun stopPlayout(): Int {
        Log.d(this.tag, "stopPlayout")
        if (audioThread == null) {
            Log.w(this.tag, "stopPlayout, but audioThread is null.")
            return ErrorCode.STOP_ERROR.ordinal
        }
        audioThread?.stopThread()
        audioThread = null
        releaseAudioRelease()
        return ErrorCode.NO_ERROR.ordinal
    }

    private fun releaseAudioRelease() {
        Log.d(this.tag, "releaseAudioRelease")
        audioTrack?.release()
        audioTrack = null
    }

    inner class AudioTrackThread : Thread("audio_render_thread") {

        private val tag: String = "AudioTrackThread"
        private var keepAlive = true
        private var inputStream: InputStream

        init {
            val filesDir = context?.filesDir
            assert(filesDir != null) { "Please set application." }
            inputStream = context!!.assets.open("haidao.pcm")
        }

        override fun run() {
            super.run()
            Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO)

            if (audioTrack == null || audioTrack!!.playState != AudioTrack.PLAYSTATE_PLAYING) {
                Log.e(this.tag, "AudioTrack state is wrong!")
                return
            }

            assert(byteBuffer != null) { "byteBuffer is null." }
            val sizeInBytes = byteBuffer!!.capacity()
            val byteArray = ByteArray(sizeInBytes)
            byteBuffer?.clear()
            while (keepAlive) {

                //read 10ms audio data from file.
                val readLen = inputStream.read(byteArray)
                if (readLen < sizeInBytes) {
                    Log.w(this.tag, "read pcm buffer not equals sizeInBytes.")
                    if (readLen < 0) { //-1 means read end.
                        keepAlive = false
                        return
                    }
                }

                byteBuffer?.put(byteArray)
                byteBuffer?.rewind()
                val len = audioTrack!!.write(byteBuffer!!, sizeInBytes, AudioTrack.WRITE_BLOCKING)
                if (len != sizeInBytes) {
                    Log.e(this.tag, "AudioTrack.write played invalid bytes.")
                }
                if (len < 0) {
                    keepAlive = false
                    return
                }
                byteBuffer?.rewind()
            }

            runCatching {
                audioTrack?.stop()
                Log.d(this.tag, "AudioTrack.stop is done.")
            }.let {
                if (it.isFailure) {
                    Log.e(this.tag, "AudioRecord.stop failed: ${it.exceptionOrNull()?.printStackTrace()}")
                }
            }
        }

        fun stopThread() {
            Log.d(this.tag, "Stop audio_render_thread!!!")
            keepAlive = false
        }
    }
}