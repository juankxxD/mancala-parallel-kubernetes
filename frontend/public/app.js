// Cliente jugable de Kalah(6,4).
//
// El humano es el jugador 0 (lado inferior, kalaha derecho).
// El bot es el jugador 1 (lado superior, kalaha izquierdo).
//
// Reglas Kalah replicadas en JS para aplicar el movimiento humano sin
// ida-y-vuelta al backend. El bot juega vía POST /move del backend, que
// resuelve la jugada con Alfa-Beta a la profundidad y nº de hilos del panel.

const API_BASE = window.MANCALA_API_BASE || '/api';

const HUMAN = 0;
const BOT = 1;
const KALAHA = [6, 13];
const PITS_OF = [[0,1,2,3,4,5], [7,8,9,10,11,12]];

/* ----------------- Reglas Kalah ----------------- */

function initialBoard() {
    const b = new Array(14).fill(0);
    for (let i = 0; i < 6; i++) { b[i] = 4; b[i + 7] = 4; }
    return b;
}

function legalMoves(board, side) {
    return PITS_OF[side].filter(i => board[i] > 0);
}

function isTerminal(board) {
    const s0 = PITS_OF[0].reduce((a, i) => a + board[i], 0);
    const s1 = PITS_OF[1].reduce((a, i) => a + board[i], 0);
    return s0 === 0 || s1 === 0;
}

function collectRemaining(board) {
    for (const i of PITS_OF[0]) { board[KALAHA[0]] += board[i]; board[i] = 0; }
    for (const i of PITS_OF[1]) { board[KALAHA[1]] += board[i]; board[i] = 0; }
}

/** Aplica un movimiento. Devuelve { extraTurn: bool, lastIdx: int, captured: bool }. */
function applyMove(board, side, pit) {
    const myKalaha = KALAHA[side];
    const oppKalaha = KALAHA[1 - side];

    let seeds = board[pit];
    board[pit] = 0;
    let idx = pit;
    while (seeds > 0) {
        idx = (idx + 1) % 14;
        if (idx === oppKalaha) continue;
        board[idx] += 1;
        seeds -= 1;
    }

    const extraTurn = (idx === myKalaha);
    let captured = false;

    if (!extraTurn) {
        const onOwnSide = PITS_OF[side].includes(idx);
        if (onOwnSide && board[idx] === 1) {
            const opp = 12 - idx;
            if (board[opp] > 0) {
                board[myKalaha] += board[opp] + 1;
                board[opp] = 0;
                board[idx] = 0;
                captured = true;
            }
        }
    }

    return { extraTurn, lastIdx: idx, captured };
}

/* ----------------- Estado del juego ----------------- */

const state = {
    board: initialBoard(),
    sideToMove: HUMAN,
    over: false,
    waitingBot: false,
};

/* ----------------- Render ----------------- */

const $ = id => document.getElementById(id);

function render() {
    for (let i = 0; i < 14; i++) {
        $(`pit-${i}`).textContent = String(state.board[i]);
    }
    $('score-p0').textContent = String(state.board[KALAHA[0]]);
    $('score-p1').textContent = String(state.board[KALAHA[1]]);

    // Pits clicables solo si es turno del humano y tienen semillas.
    for (const i of PITS_OF[HUMAN]) {
        const el = document.querySelector(`button.pit[data-idx="${i}"]`);
        const playable = (state.sideToMove === HUMAN && !state.over &&
                          !state.waitingBot && state.board[i] > 0);
        el.classList.toggle('playable', playable);
        el.disabled = !playable;
    }
    // Pits del rival siempre deshabilitados.
    for (const i of PITS_OF[BOT]) {
        const el = document.querySelector(`button.pit[data-idx="${i}"]`);
        el.disabled = true;
    }

    // Indicador de estado
    const status = $('status');
    status.classList.remove('thinking', 'over', 'error');
    if (state.over) {
        status.classList.add('over');
        const s0 = state.board[KALAHA[0]], s1 = state.board[KALAHA[1]];
        $('status-text').textContent =
            s0 > s1 ? '¡Ganaste!' : s1 > s0 ? 'Ganó el bot' : 'Empate';
    } else if (state.waitingBot) {
        status.classList.add('thinking');
        $('status-text').textContent = 'Bot pensando…';
    } else if (state.sideToMove === HUMAN) {
        $('status-text').textContent = 'Tu turno';
    } else {
        $('status-text').textContent = 'Turno del bot';
    }
}

function highlightLastMove(indices) {
    document.querySelectorAll('.last-move').forEach(el => el.classList.remove('last-move'));
    for (const i of indices) {
        const el = document.querySelector(`[data-idx="${i}"]`);
        if (el) {
            // forzar reflow para reiniciar animación
            el.classList.remove('last-move');
            void el.offsetWidth;
            el.classList.add('last-move');
        }
    }
}

function logLine(cls, text) {
    const li = document.createElement('li');
    li.className = cls;
    li.textContent = text;
    const log = $('log');
    log.insertBefore(li, log.firstChild);
    while (log.children.length > 30) log.removeChild(log.lastChild);
}

function showEndModal() {
    const s0 = state.board[KALAHA[0]], s1 = state.board[KALAHA[1]];
    const title = s0 > s1 ? '¡Ganaste!' : s1 > s0 ? 'Ganó el bot' : 'Empate';
    $('modal-title').textContent = title;
    $('modal-text').textContent =
        `Tú ${s0} — ${s1} Bot. Total de semillas en kalahas: ${s0 + s1}.`;
    $('overlay').classList.remove('hidden');
}

/* ----------------- Turno del humano ----------------- */

function setupBoardClicks() {
    document.querySelectorAll('button.pit-own').forEach(el => {
        el.addEventListener('click', () => {
            if (state.over || state.waitingBot || state.sideToMove !== HUMAN) return;
            const idx = Number(el.dataset.idx);
            if (state.board[idx] === 0) return;
            humanMove(idx);
        });
    });
}

function humanMove(pit) {
    const { extraTurn, lastIdx, captured } = applyMove(state.board, HUMAN, pit);
    highlightLastMove(captured ? [lastIdx, 12 - lastIdx, KALAHA[HUMAN]] : [lastIdx]);
    logLine('you', `Tú: hoyo ${pit}` +
                   (extraTurn ? ' (turno extra)' : '') +
                   (captured ? ' (captura)' : ''));

    if (isTerminal(state.board)) {
        collectRemaining(state.board);
        state.over = true;
        render();
        showEndModal();
        return;
    }

    if (!extraTurn) state.sideToMove = BOT;
    render();
    if (state.sideToMove === BOT) {
        scheduleBotTurn();
    }
}

/* ----------------- Turno del bot (vía backend) ----------------- */

async function scheduleBotTurn() {
    state.waitingBot = true;
    render();
    try {
        // El bot puede encadenar varios movimientos si gana turno extra.
        // Cada llamada al backend pide UNA jugada; aplicamos y repetimos.
        while (state.sideToMove === BOT && !state.over) {
            const move = await askBackendForMove();
            if (move === null) break;
            const { extraTurn, lastIdx, captured } = applyMove(state.board, BOT, move.move);
            highlightLastMove(captured ? [lastIdx, 12 - lastIdx, KALAHA[BOT]] : [lastIdx]);
            logBotMove(move, extraTurn, captured);
            updateStatsPanel(move);

            if (isTerminal(state.board)) {
                collectRemaining(state.board);
                state.over = true;
                break;
            }
            if (!extraTurn) state.sideToMove = HUMAN;
            // Mostrar el movimiento un instante antes del siguiente.
            await sleep(extraTurn ? 350 : 0);
        }
    } catch (err) {
        $('status').classList.add('error');
        $('status-text').textContent = `Error: ${err.message || err}`;
        logLine('system', `error contactando backend: ${err.message || err}`);
        state.waitingBot = false;
        render();
        return;
    }
    state.waitingBot = false;
    render();
    if (state.over) showEndModal();
}

function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }

async function askBackendForMove() {
    const payload = {
        board: [...state.board],
        side: BOT,
        depth: Number($('depth').value),
        threads: Number($('threads').value),
    };

    const resp = await fetch(`${API_BASE}/move`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload),
    });
    if (!resp.ok) {
        const body = await resp.text();
        throw new Error(`HTTP ${resp.status} ${body}`);
    }
    return await resp.json();
}

function logBotMove(move, extraTurn, captured) {
    const flags = [extraTurn && 'turno extra', captured && 'captura'].filter(Boolean).join(', ');
    const tail = flags ? ` (${flags})` : '';
    logLine('bot', `Bot: hoyo ${move.move} — ${move.elapsed_ms}ms${tail}`);
}

function updateStatsPanel(move) {
    $('stat-move').textContent = String(move.move);
    $('stat-eval').textContent = (typeof move.evaluation === 'number')
        ? move.evaluation.toFixed(3) : '—';
    $('stat-time').textContent = `${move.elapsed_ms} ms (${move.threads_used} hilos)`;
    const s = move.stats || {};
    if (typeof s.nodes === 'number' && typeof s.prunes === 'number') {
        $('stat-extra').textContent = `${s.nodes.toLocaleString()} nodos · ${s.prunes.toLocaleString()} podas`;
    } else {
        $('stat-extra').textContent = '—';
    }
}

/* ----------------- Control de UI ----------------- */

function reset() {
    state.board = initialBoard();
    state.sideToMove = HUMAN;
    state.over = false;
    state.waitingBot = false;
    document.querySelectorAll('.last-move').forEach(el => el.classList.remove('last-move'));
    $('log').innerHTML = '';
    $('stat-move').textContent = '—';
    $('stat-eval').textContent = '—';
    $('stat-time').textContent = '—';
    $('stat-extra').textContent = '—';
    $('overlay').classList.add('hidden');
    logLine('system', `Nueva partida — API: ${API_BASE}`);
    render();
}

/* ----------------- Bootstrap ----------------- */

setupBoardClicks();
$('btn-reset').addEventListener('click', reset);
$('modal-replay').addEventListener('click', reset);
reset();
