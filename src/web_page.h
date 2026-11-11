#pragma once
#include <pgmspace.h>

static const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>DF3MT — CW Morse Live Terminal</title>
<style>
:root {
  --bg: #0b0f19;
  --card-bg: #111827;
  --card-border: #1f293d;
  --accent: #00e5ff;
  --accent-glow: rgba(0, 229, 255, 0.25);
  --green: #10b981;
  --green-glow: rgba(16, 185, 129, 0.3);
  --amber: #f59e0b;
  --text-main: #f3f4f6;
  --text-dim: #9ca3af;
  --font-mono: 'SF Mono', Consolas, 'Liberation Mono', Menlo, Courier, monospace;
}
* { box-sizing: border-box; margin: 0; padding: 0; }
body {
  background: var(--bg);
  color: var(--text-main);
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
  min-height: 100vh;
  display: flex;
  flex-direction: column;
}
header {
  background: #0d1322;
  border-bottom: 1px solid var(--card-border);
  padding: 12px 20px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-wrap: wrap;
  gap: 12px;
}
.brand {
  display: flex;
  align-items: center;
  gap: 12px;
}
.callsign {
  font-size: 24px;
  font-weight: 900;
  letter-spacing: 2px;
  background: linear-gradient(135deg, #ffffff 30%, var(--accent));
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
}
.tag {
  font-size: 11px;
  font-weight: 700;
  padding: 3px 8px;
  border-radius: 4px;
  background: rgba(0, 229, 255, 0.12);
  color: var(--accent);
  border: 1px solid rgba(0, 229, 255, 0.3);
  letter-spacing: 1px;
}
.status-bar {
  display: flex;
  align-items: center;
  gap: 14px;
  font-size: 13px;
}
.live-pill {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 4px 10px;
  background: rgba(16, 185, 129, 0.12);
  border: 1px solid rgba(16, 185, 129, 0.4);
  color: var(--green);
  border-radius: 20px;
  font-weight: 600;
}
.live-dot {
  width: 8px;
  height: 8px;
  background: var(--green);
  border-radius: 50%;
  box-shadow: 0 0 8px var(--green);
  animation: pulse 1.5s infinite;
}
@keyframes pulse {
  0%, 100% { opacity: 1; transform: scale(1); }
  50% { opacity: 0.4; transform: scale(0.85); }
}
.wpm-badge {
  background: #1e293b;
  border: 1px solid var(--card-border);
  padding: 4px 12px;
  border-radius: 6px;
  font-weight: 700;
  color: var(--amber);
  font-family: var(--font-mono);
}
main {
  flex: 1;
  max-width: 1200px;
  width: 100%;
  margin: 0 auto;
  padding: 20px;
  display: flex;
  flex-direction: column;
  gap: 16px;
}
.stats-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
  gap: 12px;
}
.stat-card {
  background: var(--card-bg);
  border: 1px solid var(--card-border);
  padding: 12px 16px;
  border-radius: 8px;
  display: flex;
  flex-direction: column;
}
.stat-title {
  font-size: 11px;
  color: var(--text-dim);
  text-transform: uppercase;
  font-weight: 600;
  letter-spacing: 0.5px;
}
.stat-val {
  font-size: 22px;
  font-weight: 800;
  font-family: var(--font-mono);
  color: #fff;
  margin-top: 4px;
}
.terminal-card {
  background: var(--card-bg);
  border: 1px solid var(--card-border);
  border-radius: 12px;
  box-shadow: 0 10px 30px rgba(0, 0, 0, 0.4);
  display: flex;
  flex-direction: column;
  flex: 1;
  min-height: 380px;
  position: relative;
  overflow: hidden;
}
.terminal-header {
  background: #0f1626;
  border-bottom: 1px solid var(--card-border);
  padding: 10px 16px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  font-size: 12px;
  color: var(--text-dim);
}
.terminal-controls {
  display: flex;
  align-items: center;
  gap: 8px;
}
button {
  background: #1f293d;
  color: var(--text-main);
  border: 1px solid #334155;
  padding: 6px 12px;
  border-radius: 6px;
  font-size: 12px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.15s ease;
  display: inline-flex;
  align-items: center;
  gap: 6px;
}
button:hover {
  background: #2d3c54;
  border-color: var(--accent);
  color: #fff;
}
button.active {
  background: rgba(0, 229, 255, 0.15);
  border-color: var(--accent);
  color: var(--accent);
}
.terminal-body {
  flex: 1;
  padding: 20px;
  font-family: var(--font-mono);
  font-size: 24px;
  line-height: 1.6;
  overflow-y: auto;
  white-space: pre-wrap;
  word-break: break-word;
  background: radial-gradient(circle at 50% 0%, #161e31 0%, var(--card-bg) 70%);
}
.cursor {
  display: inline-block;
  width: 12px;
  height: 24px;
  background: var(--accent);
  vertical-align: text-bottom;
  animation: blink 1s step-end infinite;
  box-shadow: 0 0 8px var(--accent);
}
@keyframes blink { 0%, 100% { opacity: 1; } 50% { opacity: 0; } }
.prosign {
  display: inline-block;
  background: rgba(0, 229, 255, 0.15);
  color: var(--accent);
  border: 1px solid rgba(0, 229, 255, 0.4);
  padding: 0 6px;
  border-radius: 4px;
  font-weight: 700;
  margin: 0 2px;
  font-size: 0.9em;
}
.raw-ticker {
  background: #0a0e17;
  border-top: 1px solid var(--card-border);
  padding: 8px 16px;
  font-family: var(--font-mono);
  font-size: 13px;
  color: var(--accent);
  display: flex;
  align-items: center;
  gap: 10px;
  overflow: hidden;
}
.raw-label {
  color: var(--text-dim);
  font-size: 11px;
  font-weight: 700;
  text-transform: uppercase;
}
.raw-stream {
  white-space: nowrap;
  letter-spacing: 2px;
}
footer {
  border-top: 1px solid var(--card-border);
  padding: 12px 20px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  font-size: 12px;
  color: var(--text-dim);
  background: #0d1322;
}
footer a {
  color: var(--accent);
  text-decoration: none;
}
footer a:hover { text-decoration: underline; }
</style>
</head>
<body>
<header>
  <div class="brand">
    <span class="callsign">DF3MT</span>
    <span class="tag">CW LIVE TERMINAL</span>
  </div>
  <div class="status-bar">
    <div id="connStatus" class="live-pill">
      <span class="live-dot"></span>
      <span id="connText" data-i18n="connected">VERBUNDEN</span>
    </div>
    <div class="wpm-badge" id="wpmBadge">-- WPM &middot; -- BpM</div>
  </div>
</header>
<main>
  <div class="stats-grid">
    <div class="stat-card">
      <span class="stat-title" data-i18n="chars">Zeichen</span>
      <span class="stat-val" id="statChars">0</span>
    </div>
    <div class="stat-card">
      <span class="stat-title" data-i18n="words">Wörter</span>
      <span class="stat-val" id="statWords">0</span>
    </div>
    <div class="stat-card">
      <span class="stat-title" data-i18n="speed">Tempo</span>
      <span class="stat-val" id="statWpm">-- WPM &middot; -- BpM</span>
    </div>
    <div class="stat-card">
      <span class="stat-title" data-i18n="session">Sitzungszeit</span>
      <span class="stat-val" id="statTime">00:00</span>
    </div>
  </div>
  <div class="terminal-card">
    <div class="terminal-header">
      <span data-i18n="termTitle">CW MITSCHRIFT &middot; ECHTZEIT-DECODER</span>
      <div class="terminal-controls">
        <button id="btnSound" data-i18n-title="soundTip">🔇 Ton: Aus</button>
        <button id="btnScroll" class="active" data-i18n="scroll" data-i18n-title="scrollTip">📜 Auto-Scroll</button>
        <button id="btnCopy" data-i18n="copy" data-i18n-title="copyTip">📋 Kopieren</button>
        <button id="btnDownload" data-i18n="save" data-i18n-title="saveTip">💾 Speichern</button>
        <button id="btnClear" data-i18n="clear" data-i18n-title="clearTip">🗑️ Leeren</button>
      </div>
    </div>
    <div class="terminal-body" id="termBody"><span id="textContent"></span><span class="cursor"></span></div>
    <div class="raw-ticker">
      <span class="raw-label" data-i18n="elemStream">Element-Stream:</span>
      <span class="raw-stream" id="rawStream">...</span>
    </div>
  </div>
</main>
<footer>
  <span>Morse Trainer v2.1 &middot; LilyGO T-Display ESP32</span>
  <span><a href="https://df3mt.de" target="_blank">df3mt.de</a> &middot; <a href="https://github.com/DF3MT/esp32-morse-trainer" target="_blank">GitHub</a></span>
</footer>
<script>
let ws = null;
let textBuffer = "";
let autoScroll = true;
let soundEnabled = false;
let audioCtx = null;
let startTime = Date.now();
let charCount = 0;
let wordCount = 0;
let lastWpm = 0;
let connected = false;

// ---- UI translations (keep keys in sync across languages) ----
const I18N = {
  de: {
    connected: 'VERBUNDEN', disconnected: 'GETRENNT (verbinde neu...)',
    chars: 'Zeichen', words: 'Wörter', speed: 'Tempo', session: 'Sitzungszeit',
    termTitle: 'CW MITSCHRIFT · ECHTZEIT-DECODER',
    soundOn: '🔊 Ton: Ein', soundOff: '🔇 Ton: Aus', soundTip: 'Web-Audio-Mithörton ein/aus',
    scroll: '📜 Auto-Scroll', scrollTip: 'Automatisch mitscrollen',
    copy: '📋 Kopieren', copied: '✅ Kopiert!', copyTip: 'Text in die Zwischenablage kopieren',
    save: '💾 Speichern', saveTip: 'Als TXT-Datei herunterladen',
    clear: '🗑️ Leeren', clearTip: 'Mitschrift leeren', clearConfirm: 'Möchtest du die gesamte Mitschrift leeren?',
    elemStream: 'Element-Stream:', cpm: 'BpM'
  },
  en: {
    connected: 'CONNECTED', disconnected: 'DISCONNECTED (reconnecting...)',
    chars: 'Characters', words: 'Words', speed: 'Speed', session: 'Session time',
    termTitle: 'CW TRANSCRIPT · REAL-TIME DECODER',
    soundOn: '🔊 Sound: On', soundOff: '🔇 Sound: Off', soundTip: 'Toggle Web Audio sidetone',
    scroll: '📜 Auto-scroll', scrollTip: 'Scroll automatically',
    copy: '📋 Copy', copied: '✅ Copied!', copyTip: 'Copy text to clipboard',
    save: '💾 Save', saveTip: 'Download as TXT file',
    clear: '🗑️ Clear', clearTip: 'Clear transcript', clearConfirm: 'Do you want to clear the whole transcript?',
    elemStream: 'Element stream:', cpm: 'CPM'
  }
};
let lang = 'de';
function t(key) { return (I18N[lang] && I18N[lang][key]) || I18N.de[key] || key; }

function applyLanguage(code) {
  lang = I18N[code] ? code : 'de';
  document.documentElement.lang = lang;
  document.querySelectorAll('[data-i18n]').forEach(el => { el.textContent = t(el.dataset.i18n); });
  document.querySelectorAll('[data-i18n-title]').forEach(el => { el.title = t(el.dataset.i18nTitle); });
  connText.textContent = connected ? t('connected') : t('disconnected');
  btnSound.textContent = soundEnabled ? t('soundOn') : t('soundOff');
  if (lastWpm) updateSpeedDisplay(lastWpm);
}

const textContent = document.getElementById('textContent');
const termBody = document.getElementById('termBody');
const rawStream = document.getElementById('rawStream');
const wpmBadge = document.getElementById('wpmBadge');
const statWpm = document.getElementById('statWpm');
const statChars = document.getElementById('statChars');
const statWords = document.getElementById('statWords');
const statTime = document.getElementById('statTime');
const connStatus = document.getElementById('connStatus');
const connText = document.getElementById('connText');
const btnSound = document.getElementById('btnSound');
const btnScroll = document.getElementById('btnScroll');
const btnCopy = document.getElementById('btnCopy');
const btnDownload = document.getElementById('btnDownload');
const btnClear = document.getElementById('btnClear');

setInterval(() => {
  const elapsed = Math.floor((Date.now() - startTime) / 1000);
  const m = String(Math.floor(elapsed / 60)).padStart(2, '0');
  const s = String(elapsed % 60).padStart(2, '0');
  statTime.textContent = `${m}:${s}`;
}, 1000);

function playSidetone(freq, durationMs) {
  if (!soundEnabled) return;
  try {
    if (!audioCtx) audioCtx = new (window.AudioContext || window.webkitAudioContext)();
    const osc = audioCtx.createOscillator();
    const gain = audioCtx.createGain();
    osc.type = 'sine';
    osc.frequency.setValueAtTime(freq, audioCtx.currentTime);
    gain.gain.setValueAtTime(0.12, audioCtx.currentTime);
    gain.gain.exponentialRampToValueAtTime(0.001, audioCtx.currentTime + durationMs / 1000);
    osc.connect(gain);
    gain.connect(audioCtx.destination);
    osc.start();
    osc.stop(audioCtx.currentTime + durationMs / 1000);
  } catch (e) {}
}

function updateStats() {
  statChars.textContent = charCount;
  statWords.textContent = wordCount;
}

function updateSpeedDisplay(wpm) {
  if (!wpm) return;
  lastWpm = wpm;
  const bpm = wpm * 5;
  const str = `${wpm} WPM · ${bpm} ${t('cpm')}`;
  wpmBadge.textContent = str;
  statWpm.textContent = str;
}

function renderText(txt) {
  const formatted = txt.replace(/<([A-Z]{2,3})>/g, '<span class="prosign">&lt;$1&gt;</span>');
  textContent.innerHTML = formatted;
  if (autoScroll) {
    termBody.scrollTop = termBody.scrollHeight;
  }
}

function connectWs() {
  const proto = location.protocol === 'https:' ? 'wss:' : 'ws:';
  const host = location.hostname || '192.168.4.1';
  const wsUrl = `${proto}//${host}:81/`;

  ws = new WebSocket(wsUrl);

  ws.onopen = () => {
    connStatus.style.background = 'rgba(16, 185, 129, 0.12)';
    connStatus.style.borderColor = 'rgba(16, 185, 129, 0.4)';
    connStatus.style.color = '#10b981';
    connected = true;
    connText.textContent = t('connected');
  };

  ws.onclose = () => {
    connStatus.style.background = 'rgba(239, 68, 68, 0.12)';
    connStatus.style.borderColor = 'rgba(239, 68, 68, 0.4)';
    connStatus.style.color = '#ef4444';
    connected = false;
    connText.textContent = t('disconnected');
    setTimeout(connectWs, 2000);
  };

  ws.onerror = () => {
    ws.close();
  };

  ws.onmessage = (event) => {
    try {
      const msg = JSON.parse(event.data);
      if (msg.t === 'token') {
        const token = msg.val;
        textBuffer += token;
        if (token === ' ') {
          wordCount++;
        } else {
          charCount += token.startsWith('<') ? 1 : token.length;
        }
        renderText(textBuffer);
        updateStats();
        if (msg.wpm) {
          updateSpeedDisplay(msg.wpm);
        }
        playSidetone(750, 60);
      } else if (msg.t === 'elem') {
        let cur = rawStream.textContent;
        if (cur.length > 40) cur = cur.substring(cur.length - 35);
        rawStream.textContent = cur + msg.val;
      } else if (msg.t === 'wpm') {
        updateSpeedDisplay(msg.val);
      } else if (msg.t === 'lang') {
        applyLanguage(msg.val);
      } else if (msg.t === 'init') {
        if (msg.lang) applyLanguage(msg.lang);
        textBuffer = msg.history || '';
        renderText(textBuffer);
        charCount = textBuffer.replace(/<[^>]+>/g, '_').replace(/ /g, '').length;
        wordCount = (textBuffer.match(/\s+/g) || []).length;
        updateStats();
        if (msg.wpm) {
          updateSpeedDisplay(msg.wpm);
        }
      }
    } catch (e) {
      console.error(e);
    }
  };
}

btnSound.onclick = () => {
  soundEnabled = !soundEnabled;
  btnSound.textContent = soundEnabled ? t('soundOn') : t('soundOff');
  btnSound.classList.toggle('active', soundEnabled);
  if (soundEnabled && !audioCtx) {
    audioCtx = new (window.AudioContext || window.webkitAudioContext)();
  }
};

btnScroll.onclick = () => {
  autoScroll = !autoScroll;
  btnScroll.classList.toggle('active', autoScroll);
};

btnCopy.onclick = () => {
  navigator.clipboard.writeText(textBuffer).then(() => {
    btnCopy.textContent = t('copied');
    setTimeout(() => { btnCopy.textContent = t('copy'); }, 1500);
  });
};

btnDownload.onclick = () => {
  const blob = new Blob([textBuffer], { type: 'text/plain;charset=utf-8' });
  const a = document.createElement('a');
  a.href = URL.createObjectURL(blob);
  a.download = `cw_session_${new Date().toISOString().slice(0,19).replace(/[:T]/g,'-')}.txt`;
  a.click();
};

btnClear.onclick = () => {
  if (confirm(t('clearConfirm'))) {
    textBuffer = '';
    charCount = 0;
    wordCount = 0;
    rawStream.textContent = '';
    renderText('');
    updateStats();
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify({ a: 'clear' }));
    }
  }
};

applyLanguage('de');
connectWs();
</script>
</body>
</html>
)rawliteral";
