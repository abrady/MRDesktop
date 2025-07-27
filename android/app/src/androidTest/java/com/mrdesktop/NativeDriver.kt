package com.mrdesktop

import android.os.Build
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Test
import org.junit.runner.RunWith
import kotlin.test.assertEquals

@RunWith(AndroidJUnit4::class)
class NativeDriver {
    @Test fun runCppTests() {
        val abi = Build.SUPPORTED_ABIS.first()
        val exe = "/data/local/tmp/${BuildConfig.APPLICATION_ID}.test/lib/$abi/hevc_gtest"
        val exit = Runtime.getRuntime().exec(arrayOf("sh", "-c", exe)).waitFor()
        assertEquals(0, exit)
    }
}
