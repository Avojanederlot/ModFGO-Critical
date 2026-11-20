const $ = id => document.getElementById(id);
const script = '/data/adb/modules/fgomod_zygisk/ctl.sh';
let serial = 0, busy = false, dirty = false, loaded = false;
const preview = new URLSearchParams(location.search).get('preview') === '1' && !window.ksu;

// ── KernelSU exec bridge ────────────────────────────────────
async function exec(command) {
  if (!window.ksu) throw new Error('请在 KernelSU 的模块 WebUI 中打开');
  return new Promise((resolve, reject) => {
    const name = `critical_exec_${Date.now()}_${serial++}`;
    const timer = setTimeout(() => { delete window[name]; reject(new Error('读取超时，请重试')); }, 10000);
    window[name] = (code, stdout, stderr) => {
      clearTimeout(timer); delete window[name];
      if (Number(code)) reject(new Error(stderr || `命令失败 (${code})`)); else resolve(String(stdout).trim());
    };
    try { window.ksu.exec(command, '{}', name); }
    catch (e) { clearTimeout(timer); delete window[name]; reject(e); }
  });
}

async function ctl(action) {
  if (preview) {
    if (action === 'get') return '1';
    if (action === 'pid') return '12345';
    if (action === 'status') return JSON.stringify({pid:12345, stage:'ready', installed:true, enabled:true, calls:128, forced:96, enemySkipped:32, criticalChecks:644});
    if (action === 'speed_get') return '1';
    if (action === 'modcfg_get') return JSON.stringify({EnableOneHitKill:true,EnableDematerializer:true,EnableReplaceSvt:false,EnableAutoSelectCard:true,EnablePrintAssetName:false,CardColor:2,SetFPS:120,SvtId:"[]"});
    return 'saved';
  }
  return exec(`sh ${script} ${action}`);
}

// ── UI helpers ──────────────────────────────────────────────
function message(text, error = false) {
  $('message').textContent = text;
  $('message').classList.toggle('error', error);
}

const toggleIds = ['EnableOneHitKill','EnableDematerializer','EnableReplaceSvt','EnableAutoSelectCard','EnablePrintAssetName'];
const paramIds = ['CardColor','SetFPS','SvtId','UnitySpeed'];
const allInputIds = ['critical_enabled', ...toggleIds, ...paramIds];

function setControlsEnabled(on) {
  for (const id of allInputIds) {
    const el = $(id);
    if (el) el.disabled = !on;
  }
  $('save').disabled = !on || !dirty || preview;
  $('reset').disabled = !on || preview;
}

function markDirty() {
  if (!dirty) { dirty = true; message('有未保存的修改'); }
  $('save').disabled = !loaded || busy || preview;
}

// ── Refresh: read all configs ───────────────────────────────
async function refresh() {
  if (busy) return;
  busy = true; setControlsEnabled(false);
  try {
    const [critVal, raw, pid, modRaw] = await Promise.all([
      ctl('get'), ctl('status'), ctl('pid'), ctl('modcfg_get')
    ]);

    if (critVal !== '0' && critVal !== '1') throw new Error('暴击配置格式不正确');
    const data = JSON.parse(raw);
    const modcfg = JSON.parse(modRaw);
    const running = pid.split(/\s+/).includes(String(data.pid));

    // Critical toggle
    if (!dirty) $('critical_enabled').checked = critVal === '1';

    // ModConfig toggles
    if (!dirty) {
      for (const id of toggleIds) {
        $(id).checked = modcfg[id] === true;
      }
      $('CardColor').value = String(modcfg.CardColor ?? 2);
      $('UnitySpeed').value = await ctl('speed_get');
      $('SetFPS').value = String(modcfg.SetFPS ?? 120);
      $('SvtId').value = String(modcfg.SvtId ?? '[]');
    }

    loaded = true;

    // Status display
    const ready = running && data.installed === true && data.stage === 'ready';
    $('connection').textContent = preview ? '浏览器预览 · 模拟数据'
      : ready ? '适配已就绪'
      : !pid ? '游戏未运行 · 设置已保留'
      : running ? '游戏启动中 / 等待适配'
      : '等待当前游戏进程更新状态';
    $('dot').className = `dot ${ready ? 'ready' : ''}`;

    // Metrics
    $('checks').textContent = running && Number.isSafeInteger(data.criticalChecks) ? String(data.criticalChecks) : '—';
    $('pid').textContent = pid || '未运行';
    $('stage').textContent = running ? String(data.stage) : '等待游戏启动';
    for (const [id, key] of [['calls','calls'],['forced','forced'],['enemy','enemySkipped']]) {
      $(id).textContent = running && Number.isSafeInteger(data[key]) && data[key] >= 0 ? data[key].toLocaleString() : '—';
    }

    if (!dirty) message(preview ? '预览模式不可保存到设备' : ready && data.enabled !== (critVal === '1') ? '配置已保存，等待游戏读取…' : '配置已读取');
  } catch (e) {
    $('connection').textContent = '暂时无法读取';
    $('dot').className = 'dot error';
    message(e.message, true);
  } finally {
    busy = false;
    setControlsEnabled(loaded);
  }
}

// ── Save: write all configs ─────────────────────────────────
async function save() {
  if (busy || !loaded || preview) return;
  busy = true; setControlsEnabled(false); message('正在保存…');
  try {
    const speed = $('UnitySpeed').valueAsNumber;
    if (!Number.isInteger(speed) || speed < 1 || speed > 30) throw new Error('倍速请输入 1–30 的整数');
    // Save critical config
    await ctl(`set ${$('critical_enabled').checked ? '1' : '0'}`);

    // Save ModConfig toggles
    for (const id of toggleIds) {
      await ctl(`modcfg_set ${id} ${$(id).checked ? 'true' : 'false'}`);
    }

    // Save params
    await ctl(`modcfg_set CardColor ${$('CardColor').value}`);

    const fps = parseInt($('SetFPS').value, 10);
    if (isNaN(fps) || fps < 30 || fps > 240) throw new Error('帧率范围 30–240');
    await ctl(`modcfg_set SetFPS ${fps}`);

    const svt = $('SvtId').value.trim() || '[]';
    await ctl(`modcfg_set SvtId ${svt}`);

    await ctl(`speed_set ${speed}`);
    dirty = false;
    message('已保存 · 暴击和倍速由游戏进程读取 · 其余设置重启游戏后生效');
  } catch (e) {
    message(`保存失败：${e.message}`, true);
  } finally {
    busy = false;
    setControlsEnabled(loaded);
  }
}

// ── Reset ───────────────────────────────────────────────────
async function resetAll() {
  if (busy || !loaded || preview) return;
  if (!confirm('确定恢复所有设置为默认值？')) return;
  busy = true; setControlsEnabled(false); message('正在恢复默认…');
  try {
    await ctl('set 1');
    await ctl('modcfg_reset');
    await ctl('speed_set 1');
    dirty = false;
    busy = false;
    await refresh();
  } catch (e) {
    message(`恢复失败：${e.message}`, true);
  } finally {
    busy = false;
    setControlsEnabled(loaded);
  }
}

// ── Event listeners ─────────────────────────────────────────
$('critical_enabled').addEventListener('change', markDirty);
for (const id of toggleIds) $(id).addEventListener('change', markDirty);
for (const id of paramIds) $(id).addEventListener('input', markDirty);
$('save').addEventListener('click', save);
$('reset').addEventListener('click', resetAll);
$('refresh').addEventListener('click', refresh);

// ── Init ────────────────────────────────────────────────────
refresh();
setInterval(() => { if (!document.hidden && !dirty) refresh(); }, 4000);
