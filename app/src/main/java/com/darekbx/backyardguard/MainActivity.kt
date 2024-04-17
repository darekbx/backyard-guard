package com.darekbx.backyardguard

import android.os.Bundle
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.mutableStateOf
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import com.darekbx.backyardguard.ui.theme.BackyardGuardTheme
import com.google.firebase.auth.ktx.auth
import com.google.firebase.database.DataSnapshot
import com.google.firebase.database.DatabaseError
import com.google.firebase.database.ValueEventListener
import com.google.firebase.database.getValue
import com.google.firebase.database.ktx.database
import com.google.firebase.ktx.Firebase
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

data class Temperature(val temperature: Float = 0F, val timestamp: Long = 0L) {

    fun formattedTemp() = String.format("%.1f", temperature)

    fun getDateTime() :String {
        val sdf = SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault())
        val date = Date(timestamp * 1000)
        return sdf.format(date)
    }

    override fun toString(): String {
        return "$temperature, $timestamp"
    }
}

class MainActivity : ComponentActivity() {

    private var currentTemperature = mutableStateOf<Temperature?>(null)
    private var lastPirEvent = mutableStateOf<Long?>(null)
    private var error = mutableStateOf<String?>(null)

    fun Long.getDateTime() :String {
        val sdf = SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault())
        val date = Date(this)
        return sdf.format(date)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        Firebase.auth.signInWithEmailAndPassword(BuildConfig.CLOUD_EMAIL, BuildConfig.CLOUD_PASSWORD)
            .addOnCompleteListener(this) { task ->
                if (task.isSuccessful) {
                    Log.v("sigma", "Logged in")
                    startTemp()
                    startPir()
                } else {
                    Log.e("sigma", "Failed to login")
                }
            }


        setContent {
            BackyardGuardTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    Box(modifier = Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                        Column {
                            currentTemperature?.value?.let {
                                Text(
                                    text = "Temperature: ${it.formattedTemp()}°C (${it.getDateTime()})",
                                    modifier = Modifier
                                )
                            }
                            Spacer(modifier = Modifier.height(16.dp))
                            lastPirEvent?.value?.let {
                                Text(
                                    text = "Last PIR event: ${it.getDateTime()})",
                                    modifier = Modifier
                                )
                            }
                            Spacer(modifier = Modifier.height(16.dp))
                            Text(text = error.value ?: "", color = Color.Red)
                        }
                    }
                }
            }
        }
    }

    private fun startTemp() {
        val database = Firebase.database.reference

        val dataListener = object : ValueEventListener {
            override fun onDataChange(dataSnapshot: DataSnapshot) {
                val temp = dataSnapshot.children.last().getValue<Temperature>()
                currentTemperature.value = temp
                Log.v("sigma", "onDataChange, $temp")
            }

            override fun onCancelled(databaseError: DatabaseError) {
                Log.e("sigma", "onCancelled")
                error.value = databaseError.message
            }
        }
        database.child("/data/temperature").addValueEventListener(dataListener)
    }

    private fun startPir() {
        val database = Firebase.database.reference

        val dataListener = object : ValueEventListener {
            override fun onDataChange(dataSnapshot: DataSnapshot) {
                val lastPirevent = dataSnapshot.children.last().getValue<Long>()
                lastPirEvent.value = lastPirevent
                Log.v("sigma", "onDataChange, $lastPirevent")
            }

            override fun onCancelled(databaseError: DatabaseError) {
                Log.e("sigma", "onCancelled")
                error.value = databaseError.message
            }
        }
        database.child("/data/pir-event").addValueEventListener(dataListener)
    }
}
