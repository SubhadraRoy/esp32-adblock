const fs = require('fs');
const path = require('path');

const sampleRate = 44100;
const duration = 20.0;
const totalSamples = Math.floor(sampleRate * duration);
const numChannels = 2;

console.log(`Synthesizing procedural cyberpunk audio track (${duration}s, 44.1kHz stereo)...`);

// Buffer for 16-bit PCM stereo
const byteLength = totalSamples * numChannels * 2;
const pcmBuffer = Buffer.alloc(byteLength);

// Synthesis state
const bpm = 124;
const beatSec = 60 / bpm; // ~0.4838s per beat

// Simple resonant lowpass filter state for arpeggio
let fltL = 0, fltR = 0, fltVelL = 0, fltVelR = 0;

for (let i = 0; i < totalSamples; i++) {
  const t = i / sampleRate;

  // --- 1. Kick Drum (at every beat) ---
  const beatTime = t % beatSec;
  let kick = 0;
  if (beatTime < 0.22) {
    const kickEnv = Math.exp(-beatTime * 28);
    const kickPitch = 48 + 140 * Math.exp(-beatTime * 45); // Pitch drop 188Hz -> 48Hz
    kick = Math.sin(2 * Math.PI * kickPitch * beatTime) * kickEnv * 0.9;
    // Add sub-click for punch
    if (beatTime < 0.008) {
      kick += (Math.random() * 2 - 1) * 0.3 * (1 - beatTime / 0.008);
    }
  }

  // --- 2. Hi-Hat / Shakers (every 16th note = beatSec / 4) ---
  const sixteenthTime = t % (beatSec / 4);
  const sixteenthIdx = Math.floor(t / (beatSec / 4));
  let hat = 0;
  // Accent on off-beats
  const isOffbeat = (sixteenthIdx % 2 === 1);
  const hatVol = isOffbeat ? 0.35 : 0.18;
  if (sixteenthTime < 0.045) {
    const hatEnv = Math.exp(-sixteenthTime * 85);
    const noise = (Math.random() * 2 - 1);
    hat = noise * hatEnv * hatVol;
  }

  // --- 3. Sub-Bass Drone & Pumping Sidechain ---
  // Notes: F1 (43.65Hz), Ab1 (51.9Hz), Bb1 (58.27Hz), C2 (65.4Hz)
  let bassFreq = 43.65;
  if (t >= 4.0 && t < 8.0) bassFreq = 51.9;
  if (t >= 8.0 && t < 12.0) bassFreq = 58.27;
  if (t >= 12.0 && t < 16.0) bassFreq = 65.4;
  if (t >= 16.0) bassFreq = 43.65;

  const sidechain = Math.min(1.0, (beatTime / 0.25)); // Dip volume during kick
  const subBass = Math.sin(2 * Math.PI * bassFreq * t) * 0.45 * (0.3 + 0.7 * sidechain);

  // --- 4. Melodic Cyber Arpeggio (16th notes synth wave) ---
  const arpNotesFm = [174.61, 207.65, 261.63, 311.13, 349.23, 415.30, 523.25, 622.25]; // Fm pentatonic
  const noteIdx = Math.floor((t / (beatSec / 4)) % arpNotesFm.length);
  const noteFreq = arpNotesFm[noteIdx];
  const arpTime = sixteenthTime;
  const arpEnv = Math.exp(-arpTime * 18);
  // Sawtooth approximation (sum of 3 sines)
  let rawArp = (Math.sin(2 * Math.PI * noteFreq * t) +
                0.5 * Math.sin(2 * Math.PI * noteFreq * 2 * t) +
                0.25 * Math.sin(2 * Math.PI * noteFreq * 3 * t)) * arpEnv * 0.28;

  // Filter sweep on arpeggio
  const cutoff = 0.05 + 0.15 * (0.5 + 0.5 * Math.sin(2 * Math.PI * 0.2 * t));
  fltL += (rawArp - fltL) * cutoff;
  const arpOut = fltL * (0.4 + 0.6 * sidechain);

  // --- 5. Risers / Whooshes before scene transitions (at 3.2s-4.0s, 7.2s-8.0s, 11.2s-12.0s, 15.2s-16.0s) ---
  let riser = 0;
  const sceneT = t % 4.0;
  if (sceneT >= 3.2) {
    const rProgress = (sceneT - 3.2) / 0.8; // 0 to 1
    const rNoise = (Math.random() * 2 - 1);
    riser = rNoise * Math.pow(rProgress, 2.5) * 0.35;
  }

  // --- 6. High-Tech UI Clicks & Telemetry Blips ---
  let blip = 0;
  if ((t > 1.2 && t < 1.26) || (t > 1.45 && t < 1.51) || (t > 8.5 && t < 8.56) || (t > 13.0 && t < 13.06)) {
    blip = Math.sin(2 * Math.PI * 2400 * t) * 0.25;
  }

  // Stereo panning: Arp alternates slightly left/right
  const panOffset = Math.sin(2 * Math.PI * (bpm / 60) * t) * 0.25;
  let left = kick * 0.95 + subBass + hat * 0.8 + arpOut * (0.5 - panOffset) + riser * 0.7 + blip;
  let right = kick * 0.95 + subBass + hat * 0.9 + arpOut * (0.5 + panOffset) + riser * 0.7 + blip;

  // Master Soft Limiter / Saturation
  left = Math.tanh(left * 1.3) * 0.95;
  right = Math.tanh(right * 1.3) * 0.95;

  // Convert to 16-bit signed PCM (-32768 to 32767)
  const sampleL = Math.max(-32768, Math.min(32767, Math.floor(left * 32767)));
  const sampleR = Math.max(-32768, Math.min(32767, Math.floor(right * 32767)));

  const offset = i * 4;
  pcmBuffer.writeInt16LE(sampleL, offset);
  pcmBuffer.writeInt16LE(sampleR, offset + 2);
}

// Write standard 44-byte WAV header
const header = Buffer.alloc(44);
header.write('RIFF', 0);
header.writeUInt32LE(36 + byteLength, 4);
header.write('WAVE', 8);
header.write('fmt ', 12);
header.writeUInt32LE(16, 16); // Subchunk1Size
header.writeUInt16LE(1, 20);  // AudioFormat = PCM
header.writeUInt16LE(numChannels, 22);
header.writeUInt32LE(sampleRate, 24);
header.writeUInt32LE(sampleRate * numChannels * 2, 28); // ByteRate
header.writeUInt16LE(numChannels * 2, 32);              // BlockAlign
header.writeUInt16LE(16, 34);                            // BitsPerSample
header.write('data', 36);
header.writeUInt32LE(byteLength, 40);

const finalWav = Buffer.concat([header, pcmBuffer]);
const audioOut = path.join(__dirname, 'audio_master.wav');
fs.writeFileSync(audioOut, finalWav);
console.log(`Audio master written successfully: ${audioOut} (${(finalWav.length / 1024 / 1024).toFixed(2)} MB)`);
