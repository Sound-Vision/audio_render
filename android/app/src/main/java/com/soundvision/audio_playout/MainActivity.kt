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
        audioRender = SVNativeAudioRender.instance
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