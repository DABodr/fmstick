/*
 * FmStick web UI. Copyright (C) 2026 Aurel. GPL v2 or later.
 *
 * Polls /api/state every second and sends changes to /api/set. The state
 * texts (status, banner, modulation...) come already translated from the
 * application; only the fixed labels of the page are translated here.
 */
'use strict';

const TEXTS = {
  fr: {
    frequency: 'Fréquence', freqLabel: 'Fréquence en MHz',
    freqDown: 'Baisser la fréquence de 0,1 MHz', freqUp: 'Monter la fréquence de 0,1 MHz',
    freqTip: 'Choisissez une fréquence où votre radio ne capte que du souffle, loin des stations locales.',
    power: "Puissance d'émission", powerNote: 'À forte puissance, vérifiez ce que la réglementation de votre pays autorise.',
    sound: 'Son', inputLevel: "Niveau d'entrée", modulation: 'Modulation', dsp: 'Traitement du son',
    rds: 'Informations RDS', ps: 'Nom de la station', rt: 'Texte défilant', pty: 'Type de programme',
    psHint: 'Plus de 8 caractères : les récepteurs affichent le nom par morceaux de 8.',
    save: 'Garder au démarrage', classic: 'Interface avancée',
    dirty: 'Modifications appliquées en direct, pas encore gardées pour le prochain démarrage',
    clean: "Aucune modification depuis la lecture de l'émetteur",
    confirmSave: "Enregistrer ces réglages dans la mémoire de l'émetteur ?\nIl démarrera avec eux la prochaine fois.",
    offline: 'Application injoignable', offlineDevice: "L'application FmStick ne répond pas",
  },
  en: {
    frequency: 'Frequency', freqLabel: 'Frequency in MHz',
    freqDown: 'Lower the frequency by 0.1 MHz', freqUp: 'Raise the frequency by 0.1 MHz',
    freqTip: 'Pick a frequency where your radio only hears static, away from local stations.',
    power: 'Transmit power', powerNote: 'At high power, check what the regulations of your country allow.',
    sound: 'Sound', inputLevel: 'Input level', modulation: 'Modulation', dsp: 'Sound processing',
    rds: 'RDS information', ps: 'Station name', rt: 'Radio text', pty: 'Programme type',
    psHint: 'More than 8 characters: receivers show the name in parts of 8.',
    save: 'Keep at startup', classic: 'Advanced interface',
    dirty: 'Changes applied live, not yet kept for the next startup',
    clean: 'No change since the transmitter was read',
    confirmSave: "Write these settings to the transmitter's memory?\nIt will start with them next time.",
    offline: 'App unreachable', offlineDevice: 'The FmStick application does not answer',
  },
};
const lang = (navigator.language || 'en').toLowerCase().startsWith('fr') ? 'fr' : 'en';
const t = (key) => TEXTS[lang][key];
const $ = (id) => document.getElementById(id);

function translatePage() {
  document.documentElement.lang = lang;
  document.querySelectorAll('[data-i18n]').forEach((el) => { el.textContent = t(el.dataset.i18n); });
  document.querySelectorAll('[data-i18n-label]').forEach((el) => {
    el.setAttribute('aria-label', t(el.dataset.i18nLabel));
    el.title = t(el.dataset.i18nLabel);
  });
}

/* ------------------------------------------------------------ server */

let state = null;
let pollTimer = null;

async function send(field, value = '') {
  const body = 'field=' + encodeURIComponent(field) + '&value=' + encodeURIComponent(value);
  try {
    await fetch('/api/set', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded', 'X-FmStick': '1' },
      body,
    });
  } catch (e) { /* the next poll shows the app as unreachable */ }
  setTimeout(poll, 300);   // the app applies the change in its own thread
}

async function poll() {
  clearTimeout(pollTimer);
  try {
    const r = await fetch('/api/state', { cache: 'no-store' });
    if (!r.ok) throw new Error(r.status);
    state = await r.json();
    render();
  } catch (e) {
    renderOffline();
  }
  pollTimer = setTimeout(poll, 1000);
}

/* ------------------------------------------------------------ render */

/* never overwrite what the user is typing */
function setValue(input, value) {
  if (document.activeElement !== input && input.value !== String(value)) input.value = value;
}

/* the frequency field as wide as its value ("101.1", "101.05"): a fixed
   width cut the last digit on phones */
function fitFreq() {
  const f = $('freq');
  f.style.width = (Math.max(f.value.length, 3) + 0.4) + 'ch';
}

function fillSelect(select, list) {
  const items = list.items || [];
  const same = select.options.length === items.length && items.every((txt, i) => select.options[i].text === txt);
  if (!same) {
    select.replaceChildren(...items.map((txt, i) => new Option(txt, i)));
  }
  if (document.activeElement !== select) select.value = list.index;
}

function render() {
  const s = state;
  const connected = s.state !== 'unplugged';

  $('device').textContent = s.device;
  $('pill').dataset.state = s.state;
  $('pill-text').textContent = s.status;

  const b = s.banner || {};
  $('banner').hidden = !b.text;
  $('banner').dataset.error = b.error ? 'true' : 'false';
  $('banner-text').textContent = b.text || '';
  $('banner-action').hidden = !b.action;
  $('banner-action').textContent = b.action || '';

  $('main').classList.toggle('off', !connected);

  const freq = $('freq');
  freq.min = s.freqMin; freq.max = s.freqMax;
  setValue(freq, (+s.freq).toFixed(Math.abs(s.freq * 10 - Math.round(s.freq * 10)) < 1e-6 ? 1 : 2));
  fitFreq();

  const power = $('power');
  if (power.children.length !== s.powerPresets.length) {
    power.replaceChildren(...s.powerPresets.map((v) => {
      const btn = document.createElement('button');
      btn.type = 'button';
      btn.setAttribute('role', 'radio');
      btn.addEventListener('click', () => send('power', v));
      return btn;
    }));
  }
  s.powerPresets.forEach((v, i) => {
    const btn = power.children[i];
    btn.textContent = s.powerLabels[i];
    btn.setAttribute('aria-checked', v === s.power ? 'true' : 'false');
  });
  $('power-detail').textContent = s.powerDetail;
  $('power-detail').title = s.powerHint || '';

  $('tx').textContent = s.txLabel;
  $('tx').dataset.on = s.onAir ? 'true' : 'false';

  const lvl = s.inputLevel;
  $('level-text').textContent = lvl === null || lvl === undefined ? '—' : lvl + ' dB';
  const lb = $('level-bar');
  lb.style.width = (lvl === null || lvl === undefined ? 0 : Math.max(0, Math.min(70, lvl + 70)) / 70 * 100) + '%';
  lb.style.background = lvl > -3 ? '#DC2626' : lvl > -10 ? '#EAB308' : '';
  $('mod-text').textContent = s.modulation.text;
  $('mod-text').style.color = s.modulation.textColor;
  $('mod-bar').style.width = s.modulation.percent + '%';
  $('mod-bar').style.background = s.modulation.color;
  $('level-warning').hidden = !s.levelWarning;
  $('level-warning').textContent = s.levelWarning || '';
  fillSelect($('dsp'), s.dsp);

  setValue($('ps'), s.ps);
  setValue($('rt'), s.rt);
  /* RadioText from a media player: the field then holds the text sent
     when nothing is playing */
  const a = s.rtAuto || {};
  $('rt-label').textContent = s.rtLabel || t('rt');
  if (document.activeElement !== $('rt-auto')) $('rt-auto').checked = !!a.enabled;
  $('rt-auto-label').textContent = a.label || '';
  $('rt-player').hidden = !a.enabled;
  $('rt-auto-info').hidden = !a.enabled;
  $('rt-auto-info').textContent = a.info || '';
  const players = a.players || [];
  fillSelect($('rt-player'), { items: [a.anyLabel || ''].concat(players.map((p) => p.name)),
                               index: a.player ? players.findIndex((p) => p.id === a.player) + 1 : 0 });
  $('rt-player').dataset.ids = JSON.stringify([''].concat(players.map((p) => p.id)));
  updateCounts();
  fillSelect($('pty'), s.pty);
  const ck = s.clock || {};
  if (document.activeElement !== $('clock')) $('clock').checked = !!ck.enabled;
  $('clock-label').textContent = ck.label || '';
  $('clock').closest('label').title = ck.hint || '';

  $('save-dot').style.background = s.dirty ? '#B45309' : '#16A34A';
  $('save-text').textContent = t(s.dirty ? 'dirty' : 'clean');
  $('save').disabled = !connected || !s.dirty;
}

function renderOffline() {
  $('pill').dataset.state = 'offline';
  $('pill-text').textContent = t('offline');
  $('device').textContent = t('offlineDevice');
  $('main').classList.add('off');
  $('save').disabled = true;
}

function updateCounts() {
  const ps = $('ps').value;
  $('ps-count').textContent = ps.length + ' / 8';
  $('ps-hint').hidden = ps.length <= 8;
  $('rt-count').textContent = $('rt').value.length + ' / 64';
}

/* ------------------------------------------------------------ events */

function onCommit(input, field) {
  /* change = Enter or leaving the field, like the application */
  input.addEventListener('change', () => send(field, input.value));
  input.addEventListener('keydown', (e) => { if (e.key === 'Enter') input.blur(); });
}

document.addEventListener('DOMContentLoaded', () => {
  translatePage();
  $('freq-down').addEventListener('click', () => send('freqStep', -1));
  $('freq-up').addEventListener('click', () => send('freqStep', 1));
  onCommit($('freq'), 'freq');
  $('freq').addEventListener('input', fitFreq);
  $('tx').addEventListener('click', () => send('tx', state && state.onAir ? 'off' : 'on'));
  $('banner-action').addEventListener('click', () => send('fix'));
  $('dsp').addEventListener('change', (e) => send('dsp', e.target.value));
  $('pty').addEventListener('change', (e) => send('pty', e.target.value));
  $('clock').addEventListener('change', (e) => send('clock', e.target.checked ? 'on' : 'off'));
  onCommit($('ps'), 'ps');
  onCommit($('rt'), 'rt');
  $('rt-auto').addEventListener('change', (e) => send('rtAuto', e.target.checked ? 'on' : 'off'));
  $('rt-player').addEventListener('change', (e) => {
    send('rtPlayer', JSON.parse(e.target.dataset.ids || '[""]')[+e.target.value] || '');
  });
  $('ps').addEventListener('input', updateCounts);
  $('rt').addEventListener('input', updateCounts);
  $('save').addEventListener('click', () => { if (confirm(t('confirmSave'))) send('save'); });
  poll();
});
