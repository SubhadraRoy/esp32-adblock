const http = require('http');
const fs = require('fs');
const path = require('path');
const { spawn, spawnSync } = require('child_process');

const workDir = path.join(__dirname);
const framesDir = path.join(workDir, 'frames');
if (!fs.existsSync(framesDir)) fs.mkdirSync(framesDir, { recursive: true });

const totalFrames = 600; // 20s @ 30fps
let receivedFrames = 0;
const startTime = Date.now();

console.log(`Starting launch video render pipeline (${totalFrames} frames @ 1080p 30fps)...`);

const server = http.createServer((req, res) => {
  if (req.url === '/render.html') {
    res.writeHead(200, { 'Content-Type': 'text/html' });
    fs.createReadStream(path.join(workDir, 'render_page.html')).pipe(res);
  } else if (req.url.startsWith('/frame')) {
    const chunks = [];
    req.on('data', c => chunks.push(c));
    req.on('end', () => {
      const buf = Buffer.concat(chunks);
      const urlParams = new URLSearchParams(req.url.split('?')[1]);
      const fNum = urlParams.get('f') || receivedFrames;
      const fName = `frame_${String(fNum).padStart(4, '0')}.jpg`;
      fs.writeFileSync(path.join(framesDir, fName), buf);
      receivedFrames++;

      if (receivedFrames % 60 === 0 || receivedFrames === totalFrames) {
        const pct = ((receivedFrames / totalFrames) * 100).toFixed(0);
        const elapsed = (Date.now() - startTime) / 1000;
        const fps = (receivedFrames / elapsed).toFixed(1);
        console.log(`Progress: ${receivedFrames}/${totalFrames} frames (${pct}%) at ${fps} fps`);
      }
      res.end('ok');
    });
  } else if (req.url === '/done') {
    res.end('done');
    const elapsed = (Date.now() - startTime) / 1000;
    console.log(`\nAll ${receivedFrames} frames rendered in ${elapsed.toFixed(1)}s!`);
    server.close();
    if (chromeProcess) chromeProcess.kill();

    // Now run FFmpeg muxing
    encodeVideo();
  }
}).listen(8767, () => {
  console.log('Frame capture server listening on port 8767');
  launchChrome();
});

let chromeProcess = null;
function launchChrome() {
  const chromePath = 'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe';
  chromeProcess = spawn(chromePath, [
    '--headless=new',
    '--disable-gpu',
    '--window-size=1920,1080',
    '--no-sandbox',
    'http://127.0.0.1:8767/render.html'
  ]);
}

function encodeVideo() {
  console.log('\nMuxing frames and audio into final MP4 via FFmpeg...');
  const audioPath = path.join(workDir, 'audio_master.wav');
  const videoOut = path.join(__dirname, '..', 'launch_video.mp4');

  // FFmpeg command:
  // -framerate 30 -i frames/frame_%04d.jpg
  // -i audio_master.wav
  // -c:v libx264 -preset medium -crf 18 -pix_fmt yuv420p
  // -c:a aac -b:a 256k
  // -shortest -movflags +faststart
  const args = [
    '-framerate', '30',
    '-i', path.join(framesDir, 'frame_%04d.jpg'),
    '-i', audioPath,
    '-c:v', 'libx264',
    '-preset', 'fast',
    '-crf', '17',
    '-pix_fmt', 'yuv420p',
    '-c:a', 'aac',
    '-b:a', '256k',
    '-shortest',
    '-movflags', '+faststart',
    '-y',
    videoOut
  ];

  const ffmpeg = spawnSync('ffmpeg', args, { stdio: 'inherit' });
  if (ffmpeg.status === 0 && fs.existsSync(videoOut)) {
    const stats = fs.statSync(videoOut);
    console.log(`\n======================================================`);
    console.log(`SUCCESS! Promotional video generated:`);
    console.log(`Path: ${videoOut}`);
    console.log(`Size: ${(stats.size / 1024 / 1024).toFixed(2)} MB`);
    console.log(`Duration: 20.0s @ 1080p 30fps`);
    console.log(`======================================================\n`);
  } else {
    console.error('FFmpeg encoding failed with code:', ffmpeg.status);
    process.exit(1);
  }
}
