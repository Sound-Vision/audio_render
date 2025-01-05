package com.soundvision.audio_playout

import android.media.AudioManager
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.view.View
import com.soundvision.audio_playout.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var audioRender: IAudioRender

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        context = application
        audioRender = SVAudioTrack.instance
    }

    /**
     * A native method that is implemented by the 'audio_playout' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    companion object {
        // Used to load the 'audio_playout' library on application startup.
        init {
            System.loadLibrary("audio_playout")
        }
    }

    fun initPlayout(view: View) {
        audioRender.initPlayout(SAMPLE_RATE, CHANNELS, AudioManager.STREAM_MUSIC)
    }

    fun startPlayout(view: View) {
        audioRender.startPlayout()
    }

    fun stopPlayout(view: View) {
        audioRender.stopPlayout()
    }
}