const $ = (id) => document.getElementById(id);
let authMode = "login";
let token = localStorage.getItem("tm_token") || "";
let username = localStorage.getItem("tm_username") || "";
let passage = "";
let timerId = null;
let timeLeft = 30;
let totalDuration = 30;
let startedAt = null;
let running = false;
let finished = false;

const passages = {
  easy: "Practice every day to improve your typing speed. Small steps lead to great results.",
  medium: "Consistent practice improves accuracy, concentration, and confidence. Focus on rhythm rather than rushing through every sentence.",
  hard: "Reliable software requires analytical thinking, disciplined experimentation, and careful evaluation of unexpected edge cases."
};

function headers() {
  return token ? {"X-Session-Token": token} : {};
}
async function api(path, options = {}) {
  const response = await fetch(path, {...options, headers: {...headers(), ...(options.headers || {})}});
  const data = await response.json();
  if (!response.ok) throw new Error(data.error || data.message || "Request failed.");
  return data;
}
function showAuth() {
  $("authView").classList.remove("hidden");
  $("dashboard").classList.add("hidden");
  $("logoutBtn").classList.add("hidden");
  $("welcome").textContent = "";
}
function showDashboard() {
  $("authView").classList.add("hidden");
  $("dashboard").classList.remove("hidden");
  $("logoutBtn").classList.remove("hidden");
  $("welcome").textContent = username;
  $("welcomeName").textContent = username;
  loadDashboard();
}
document.querySelectorAll(".tab").forEach(tab => tab.addEventListener("click", () => {
  authMode = tab.dataset.mode;
  document.querySelectorAll(".tab").forEach(t => t.classList.toggle("active", t === tab));
  $("authSubmit").innerHTML = authMode === "login" ? 'Log in <span>→</span>' : 'Create account <span>→</span>';
  $("password").autocomplete = authMode === "login" ? "current-password" : "new-password";
  $("authMessage").textContent = "";
}));
$("authForm").addEventListener("submit", async (event) => {
  event.preventDefault();
  const body = new URLSearchParams({username: $("username").value.trim(), password: $("password").value});
  try {
    const data = await api(authMode === "login" ? "/api/login" : "/api/register", {
      method: "POST", body
    });
    $("authMessage").classList.remove("error");
    $("authMessage").textContent = data.message;
    if (authMode === "register") {
      authMode = "login";
      document.querySelector('[data-mode="login"]').click();
      $("username").value = body.get("username");
    } else {
      token = data.token; username = data.username;
      localStorage.setItem("tm_token", token);
      localStorage.setItem("tm_username", username);
      showDashboard();
    }
  } catch (error) {
    $("authMessage").classList.add("error");
    $("authMessage").textContent = error.message;
  }
});
$("logoutBtn").addEventListener("click", async () => {
  try { await api("/api/logout", {method:"POST"}); } catch (_) {}
  token = ""; username = "";
  localStorage.removeItem("tm_token"); localStorage.removeItem("tm_username");
  stopTimer(); showAuth();
});

$("duration").addEventListener("change", () => {
  $("customWrap").classList.toggle("hidden", $("duration").value !== "custom");
});
function selectedDuration() {
  const value = $("duration").value === "custom" ? Number($("customDuration").value) : Number($("duration").value);
  if (!Number.isInteger(value) || value < 5 || value > 600) throw new Error("Choose a duration between 5 and 600 seconds.");
  return value;
}
function resetTest() {
  stopTimer(); running = false; finished = false; startedAt = null;
  try { totalDuration = selectedDuration(); } catch (_) { totalDuration = 30; }
  timeLeft = totalDuration;
  $("timer").textContent = formatTime(timeLeft);
  $("typingInput").value = "";
  $("typingInput").disabled = true;
  $("startBtn").disabled = false;
  $("startBtn").textContent = "Start test →";
  $("resetBtn").disabled = true;
  $("liveWpm").textContent = "0";
  $("liveAccuracy").textContent = "100";
  $("consistency").textContent = "100";
  $("testStatus").textContent = "Ready when you are.";
  $("resultBanner").classList.add("hidden");
  $("passage").textContent = "Choose your settings and press Start test to load a passage.";
}
function formatTime(seconds) {
  return `${String(Math.floor(seconds / 60)).padStart(2,"0")}:${String(seconds % 60).padStart(2,"0")}`;
}
function renderPassage() {
  const typed = $("typingInput").value;
  $("passage").innerHTML = [...passage].map((ch, i) => {
    let cls = "";
    if (i < typed.length) cls = typed[i] === ch ? "correct" : "incorrect";
    else if (i === typed.length && running) cls = "current";
    return `<span class="${cls}">${escapeHtml(ch)}</span>`;
  }).join("");
}
function escapeHtml(value) {
  return value.replaceAll("&","&amp;").replaceAll("<","&lt;").replaceAll(">","&gt;").replaceAll('"',"&quot;").replaceAll("'","&#039;");
}
function metrics() {
  const typed = $("typingInput").value;
  let correct = 0;
  for (let i = 0; i < typed.length; i++) if (typed[i] === passage[i]) correct++;
  const errors = typed.length - correct;
  const elapsed = Math.max(1, (Date.now() - startedAt) / 1000);
  const wpm = Math.round((correct / 5) / (elapsed / 60));
  const accuracy = typed.length ? (correct / typed.length) * 100 : 100;
  $("liveWpm").textContent = String(wpm);
  $("liveAccuracy").textContent = accuracy.toFixed(0);
  $("consistency").textContent = accuracy.toFixed(0);
  return {wpm, accuracy, errors, correct, typed: typed.length};
}
function startClock() {
  if (running || finished) return;
  running = true;
  startedAt = Date.now();
  $("testStatus").textContent = "Test in progress — keep going!";
  timerId = setInterval(() => {
    const elapsed = Math.floor((Date.now() - startedAt) / 1000);
    timeLeft = Math.max(0, totalDuration - elapsed);
    $("timer").textContent = formatTime(timeLeft);
    metrics(); renderPassage();
    if (timeLeft <= 0) finishTest();
  }, 200);
}
function stopTimer() {
  if (timerId) clearInterval(timerId);
  timerId = null;
}
async function finishTest() {
  if (finished) return;
  finished = true; running = false; stopTimer();
  const result = metrics();
  const actualDuration = Math.max(1, Math.min(totalDuration, Math.round((Date.now() - startedAt) / 1000)));
  $("typingInput").disabled = true;
  $("startBtn").disabled = true;
  $("resetBtn").disabled = false;
  $("testStatus").textContent = "Test completed.";
  $("resultBanner").classList.remove("hidden");
  $("resultBanner").innerHTML = `<strong>Test complete!</strong> &nbsp; ${result.wpm} WPM · ${result.accuracy.toFixed(1)}% accuracy · ${result.errors} errors`;
  try {
    await api("/api/results", {method:"POST", body:new URLSearchParams({
      wpm: result.wpm, accuracy: result.accuracy.toFixed(2), errors: result.errors, duration: actualDuration
    })});
    await loadDashboard();
  } catch (error) {
    $("resultBanner").innerHTML += `<br>Could not save result: ${escapeHtml(error.message)}`;
  }
}
$("startBtn").addEventListener("click", async () => {
  try {
    totalDuration = selectedDuration();
    timeLeft = totalDuration;
    $("startBtn").disabled = true;
    $("testStatus").textContent = "Loading typing passage...";
    const selected = $("difficulty").value;
    // The C++ backend creates EasyTest / MediumTest / HardTest objects.
    const data = await api(`/api/passage?difficulty=${encodeURIComponent(selected)}`);
    passage = data.passage;
    $("passage").textContent = passage;
    $("timer").textContent = formatTime(timeLeft);
    $("typingInput").value = "";
    $("typingInput").disabled = false;
    $("typingInput").focus();
    $("resetBtn").disabled = false;
    $("testStatus").textContent = "Timer starts with your first keystroke.";
    $("resultBanner").classList.add("hidden");
    finished = false; running = false; startedAt = null;
  } catch (error) {
    $("startBtn").disabled = false;
    $("testStatus").textContent = error.message;
  }
});
$("resetBtn").addEventListener("click", resetTest);
$("typingInput").addEventListener("input", () => {
  if (finished) return;
  if (!running && $("typingInput").value.length > 0) startClock();
  renderPassage();
  if (running) metrics();
  if ($("typingInput").value.length >= passage.length) finishTest();
});
$("typingInput").addEventListener("paste", e => e.preventDefault());

async function loadDashboard() {
  try {
    const [history, board] = await Promise.all([api("/api/history"), api("/api/leaderboard")]);
    renderHistory(history); renderLeaderboard(board); renderStats(history);
  } catch (error) {
    if (error.message.includes("log in")) { token = ""; localStorage.removeItem("tm_token"); showAuth(); }
  }
}
function renderStats(history) {
  $("testCount").textContent = history.length;
  $("bestWpm").textContent = history.length ? `${Math.max(...history.map(x=>x.wpm)).toFixed(0)}` : "—";
  $("bestAccuracy").textContent = history.length ? `${Math.max(...history.map(x=>x.accuracy)).toFixed(0)}` : "—";
  drawWpmChart(history);
}
function drawWpmChart(history) {
  const canvas = $("wpmChart");
  if (!canvas) return;
  const context = canvas.getContext("2d");
  const width = canvas.width;
  const height = canvas.height;
  context.clearRect(0, 0, width, height);
  const values = history.slice(0, 8).reverse().map(item => item.wpm);
  if (!values.length) {
    context.fillStyle = "#65728a";
    context.font = "10px DM Sans";
    context.fillText("Complete a test to reveal your trend", 38, height / 2);
    return;
  }
  const max = Math.max(...values, 1);
  const padding = 8;
  context.strokeStyle = "rgba(142,154,178,.12)";
  context.lineWidth = 1;
  for (let i = 1; i < 4; i++) {
    const y = i * height / 4;
    context.beginPath(); context.moveTo(0, y); context.lineTo(width, y); context.stroke();
  }
  const points = values.map((value, index) => ({
    x: padding + index * (width - padding * 2) / Math.max(1, values.length - 1),
    y: height - padding - value / max * (height - padding * 2)
  }));
  const gradient = context.createLinearGradient(0, 0, 0, height);
  gradient.addColorStop(0, "rgba(98,211,204,.22)");
  gradient.addColorStop(1, "rgba(98,211,204,0)");
  context.beginPath();
  context.moveTo(points[0].x, height);
  points.forEach(point => context.lineTo(point.x, point.y));
  context.lineTo(points.at(-1).x, height);
  context.closePath();
  context.fillStyle = gradient;
  context.fill();
  context.beginPath();
  points.forEach((point, index) => index ? context.lineTo(point.x, point.y) : context.moveTo(point.x, point.y));
  context.strokeStyle = "#62d3cc";
  context.lineWidth = 2;
  context.stroke();
  points.forEach(point => {
    context.beginPath(); context.arc(point.x, point.y, 2.5, 0, Math.PI * 2);
    context.fillStyle = "#62d3cc"; context.fill();
  });
}
function renderHistory(items) {
  $("historyList").innerHTML = items.length ? items.slice(0,5).map(item =>
    `<div class="history-row"><div>${escapeHtml(item.date)}<small>${item.duration}s · ${item.errors} errors</small></div><strong>${item.wpm.toFixed(0)} WPM</strong></div>`
  ).join("") : '<p class="empty">No tests yet. Your first result will appear here.</p>';
}
function renderLeaderboard(items) {
  $("leaderboard").innerHTML = items.length ? items.slice(0,8).map((item,i) =>
    `<div class="leader-row"><span class="rank">${["🥇","🥈","🥉"][i] || (i+1)}</span><span class="leader-name">${escapeHtml(item.username)}</span><strong>${item.wpm.toFixed(0)}</strong></div>`
  ).join("") : '<p class="empty">Be the first on the leaderboard!</p>';
}
resetTest();
if (token && username) {
  api("/api/me").then(() => showDashboard()).catch(() => {token=""; username=""; localStorage.removeItem("tm_token"); localStorage.removeItem("tm_username"); showAuth();});
} else showAuth();
