// Script para k6: reporta percentiles p50/p95 y throughput sobre POST /move.
//
// Uso:
//   BASE=http://<host>:8000 THREADS=4 DEPTH=10 k6 run move.js
//
// Parámetros por variable de entorno (con valores por defecto):
//   BASE    URL base del backend            (default http://localhost:8000)
//   THREADS hilos OpenMP del motor por req   (default 2)
//   DEPTH   profundidad de búsqueda fija     (default 10)
//   VUS     usuarios virtuales concurrentes  (default 50)
//   DURATION duración de la prueba           (default 30s)
//
// El número de hilos del motor se controla con el campo `threads` del payload:
// en alphabeta.cpp el `#pragma omp parallel for num_threads(cfg.threads)`
// sobrescribe OMP_NUM_THREADS, así que el barrido local de hilos se hace
// variando THREADS aquí (misma posición y profundidad en todas las corridas).
//
// Reporta http_req_duration (med=p50, p(95)) y http_reqs (throughput).

import http from 'k6/http';
import { check } from 'k6';

const BASE = __ENV.BASE || 'http://localhost:8000';
const THREADS = Number(__ENV.THREADS || 2);
const DEPTH = Number(__ENV.DEPTH || 10);

export const options = {
  vus: Number(__ENV.VUS || 50),
  duration: __ENV.DURATION || '30s',
};

const payload = JSON.stringify({
  board: [4, 4, 4, 4, 4, 4, 0, 4, 4, 4, 4, 4, 4, 0],
  side: 0,
  depth: DEPTH,
  threads: THREADS,
});

const params = { headers: { 'Content-Type': 'application/json' } };

export default function () {
  const res = http.post(`${BASE}/move`, payload, params);
  check(res, { 'status 200': (r) => r.status === 200 });
}

// Resumen compacto en una línea para automatizar el barrido y llenar docs/07.
export function handleSummary(data) {
  const d = data.metrics.http_req_duration.values;
  const reqs = data.metrics.http_reqs.values;
  const line =
    `RESULT threads=${THREADS} depth=${DEPTH} ` +
    `p50=${d.med.toFixed(1)} p95=${d['p(95)'].toFixed(1)} ` +
    `rps=${reqs.rate.toFixed(1)}\n`;
  return { stdout: line };
}
