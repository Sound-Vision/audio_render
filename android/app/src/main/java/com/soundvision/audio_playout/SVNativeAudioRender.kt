package com.soundvision.audio_playout

import android.util.Log
import java.io.File
import java.io.FileOutputStream
import java.io.InputStream
import java.io.OutputStream

class SVNativeAudioRender private constructor(): IAudioRender{

    private val tag: String = "SVNativeAudioRender"

    companion object {
        val instance: SVNativeAudioRender by lazy {
            SVNativeAudioRender()
        }
    }

    init {
        System.loadLibrary("audio_playout")
    }

    override fun initPlayout(sampleRate: Int, channels: Int, streamType: Int): Int {
        val dir = context?.filesDir
        assert(dir != null) { "Please set context." }
        val file = File(dir, "haidao.pcm")
        Log.i(this.tag, "file: ${file.absolutePath}")
        if (!file.exists()) file.createNewFile()

        var outputStream: OutputStream? = null
        var inputStream: InputStream? = null

        val result = runCatching {
            outputStream = FileOutputStream(file, false)
            inputStream = context?.assets?.open("haidao.pcm")
            assert(inputStream != null) { "inputStream is null." }
            val len = inputStream!!.available()
            val buffer: ByteArray = ByteArray(len)
            val readLen = inputStream!!.read(buffer)
            assert(readLen == len) { "read file error!" }
            outputStream!!.write(buffer)
        }
        println("copy assert resource result: ${result.isSuccess}")

        inputStream?.close()
        outputStream?.close()

        nativeSetRenderType(2,  file.absolutePath)
        return nativeInitRender(sampleRate, channels)
    }

    override fun startPlayout(): Int {
        return nativeStartPlayout()
    }

    override fun stopPlayout(): Int {
        return nativeStopPlayout()
    }

    private external fun nativeSetRenderType(type: Int, filePath: String)
    private external fun nativeInitRender(sampleRate: Int, channels: Int): Int
    private external fun nativeStartPlayout(): Int
    private external fun nativeStopPlayout(): Int

}