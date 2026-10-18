/*
 * THEREMIN RECEPTOR OSC & SINTETIZADOR
 * Entorno: Processing 3 / 4
 * Librerías requeridas: oscP5, processing.sound
 */

import oscP5.*;
import netP5.*;
import processing.sound.*;

OscP5 oscP5;
SinOsc sineOsc;

// Control de tono y volumen con interpolación
float targetFreq = 440.0;
float currentFreq = 440.0;
float targetAmp = 0.0;
float currentAmp = 0.0;

// Variables de vibrato LFO
float vibratoDepth = 0.0; // Recibido por OSC (0.0 a 1.0)
float lfoPhase = 0.0;
float lfoSpeed = 5.5;     // Velocidad del vibrato en Hz

void setup() {
  size(700, 450);
  background(18);

  // Escuchar mensajes OSC entrantes en el puerto 12000
  oscP5 = new OscP5(this, 12000);

  // Crear e inicializar oscilador senoidal
  sineOsc = new SinOsc(this);
  sineOsc.play();
  sineOsc.amp(0.0);
}

void draw() {
  // Suavizado de cambios bruscos (lerp)
  currentFreq = lerp(currentFreq, targetFreq, 0.15);
  currentAmp  = lerp(currentAmp, targetAmp, 0.18);

  // Generador LFO senoidal para el vibrato
  lfoPhase += (lfoSpeed * TWO_PI) / frameRate;
  float lfoOffset = sin(lfoPhase) * (vibratoDepth * 35.0); // Oscilación de hasta ±35 Hz

  float finalFreq = max(50.0, currentFreq + lfoOffset);

  // Aplicar frecuencia y volumen al sintetizador
  sineOsc.freq(finalFreq);
  sineOsc.amp(currentAmp);

  // --- INTERFAZ GRÁFICA ---
  background(20, 22, 28);

  // Textos informativos
  fill(255);
  textSize(20);
  text("Theremin OSC -> Parlante Bluetooth", 35, 45);

  fill(100, 200, 255);
  textSize(15);
  text("Frecuencia: " + nf(finalFreq, 0, 1) + " Hz", 35, 90);
  text("Amplitud: " + nf(currentAmp, 0, 2), 35, 120);
  text("Profundidad Vibrato: " + int(vibratoDepth * 100) + " %", 35, 150);

  // Indicador visual reactivo en el centro
  noFill();
  stroke(80, 170, 255, map(currentAmp, 0, 0.5, 30, 255));
  strokeWeight(3);
  float diametro = map(finalFreq, 180, 1200, 80, 320);
  ellipse(width / 2, height / 2 + 30, diametro, diametro);
}

// Receptor de eventos OSC
void oscEvent(OscMessage theOscMessage) {
  if (theOscMessage.checkAddrPattern("/theremin/pitch")) {
    float dist = theOscMessage.get(0).floatValue();
    if (dist > 0) {
      // 4 cm = 1200 Hz (agudo), 38 cm = 180 Hz (grave)
      targetFreq = map(dist, 4.0, 38.0, 1200.0, 180.0);
      targetAmp = 0.5; // Nivel de volumen al tocar
    } else {
      targetAmp = 0.0; // Silencio al retirar la mano
    }
  } 
  else if (theOscMessage.checkAddrPattern("/theremin/vibrato")) {
    vibratoDepth = theOscMessage.get(0).floatValue();
  }
}
