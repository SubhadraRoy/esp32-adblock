const { spawnSync } = require('child_process');
const path = require('path');
const fs = require('fs');

const workDir = path.join(__dirname);
const audioOut = path.join(workDir, 'audio_master.wav');

console.log('Synthesizing 20-second cyber-electronic soundtrack via FFmpeg...');

// 20 seconds @ 44100Hz
// We construct an intricate multi-track synth using FFmpeg lavfi filters:
// 1. Kick Drum: 120 BPM = 0.5s per beat.
// 2. Hi-Hat / Shaker: 16th note noise burst pattern.
// 3. Cyber Synth Arpeggio: Pulsing square/sawtooth synth frequencies.
// 4. Sub-Bassline: Deep 55Hz bass drone.
// 5. Transition Whooshes at 4.0s, 8.0s, 12.0s, 16.0s.

const filterComplex = `
  /* Track 1: Sub Bass Drone (55Hz / A1 -> 43.65Hz / F1) */
  sine=f=55:d=20[sub];

  /* Track 2: 808 Style Pitch-Dropped Kick Drum (at t=0, 0.5, 1.0, 1.5, ... 40 beats) */
  sine=f=120:d=20,
  volume=eval=frame:volume='if(lt(mod(t,0.5),0.12), (1 - mod(t,0.5)/0.12)*1.8, 0)'[kick_raw];
  [kick_raw]lowpass=f=180[kick];

  /* Track 3: Cyber Hi-Hat / Tick on 16th notes (every 0.125s) */
  anoise=d=20:c=white,
  highpass=f=7500,
  volume=eval=frame:volume='if(lt(mod(t,0.125),0.025), 0.35, 0)'[hats];

  /* Track 4: Rhythmic Cyber Synth Arpeggios (Arp pattern cycling through frequencies) */
  sine=f=220:d=20[s1];
  sine=f=277:d=20[s2];
  sine=f=330:d=20[s3];
  sine=f=440:d=20[s4];
  [s1][s2][s3][s4]amix=inputs=4:duration=first:dropout_transition=0,
  volume=eval=frame:volume='0.25 * (0.5 + 0.5*sin(2*PI*8*t))'[arp_raw];
  [arp_raw]bandpass=f=1200:width_type=h:w=800,flanger=delay=3:depth=2:regen=50[arp];

  /* Track 5: Scene Transition Risers / Whooshes at 3.5-4.0s, 7.5-8.0s, 11.5-12.0s, 15.5-16.0s */
  anoise=d=20:c=white,
  bandpass=f=2400:width_type=h:w=1200,
  volume=eval=frame:volume='if(between(mod(t,4.0),3.4,4.0), (mod(t,4.0)-3.4)/0.6 * 0.7, if(lt(mod(t,4.0),0.3), (0.3-mod(t,4.0))/0.3 * 0.5, 0))'[risers];

  /* Track 6: Tech Telemetry Beeps */
  sine=f=1760:d=20,
  volume=eval=frame:volume='if(between(t,1.2,1.3)+between(t,1.4,1.5)+between(t,9.0,9.1)+between(t,9.2,9.3)+between(t,9.4,9.5)+between(t,13.5,13.6), 0.2, 0)'[beeps];

  /* Mix all 6 tracks and apply master compression & limiter */
  [sub][kick][hats][arp][risers][beeps]amix=inputs=6:duration=first:weights='0.4 1.2 0.5 0.7 0.8 0.4',
  alimiter=limit=0.92:attack=5:release=50[out]
`;

const res = spawnSync('ffmpeg', [
  '-f', 'lavfi',
  '-i', 'anullsrc=channel_layout=stereo:sample_rate=44100',
  '-filter_complex', filterComplex.replace(/\s+/g, ' '),
  '-map', '[out]',
  '-t', '20',
  '-y',
  audioOut
], { stdio: 'inherit' });

if (res.status === 0 && fs.existsSync(audioOut)) {
  console.log('Audio master generated successfully:', audioOut);
} else {
  console.error('Audio generation failed with exit code:', res.status);
  process.exit(1);
}
