# Prueba de carga (análisis comparativo local vs. nube)

Scripts para generar las métricas de latencia (p50/p95) y throughput que pide
[docs/07-analisis-comparativo.md](../../../docs/07-analisis-comparativo.md).
Apuntan al endpoint `POST /move` del backend.

Usa la **misma posición, profundidad e hilos** en ambos entornos para que la
comparación sea honesta. Solo cambia la capa de orquestación (hilos en local,
réplicas en la nube).

## Con wrk

```bash
wrk -t4 -c50 -d30s -s post_move.lua http://<host>:8000/move
```

## Con k6 (reporta p50/p95 directamente)

`move.js` se parametriza por variables de entorno: `BASE`, `THREADS`, `DEPTH`,
`VUS`, `DURATION`.

```bash
BASE=http://<host>:8000 THREADS=4 DEPTH=10 k6 run move.js
```

Sin k6 instalado, vía su imagen Docker (lo usado para generar `docs/07`):

```bash
docker run --rm -i -v "$PWD/move.js:/move.js:ro" \
  -e BASE=http://host.docker.internal:8000 -e THREADS=4 -e DEPTH=10 -e DURATION=30s \
  grafana/k6 run /move.js
```

## Barrido sugerido

- **Local**: una sola instancia, variando los hilos del motor `THREADS ∈ {1, 2, 4, 8}`
  a `DEPTH` fija. (El motor toma el conteo de hilos del campo `threads` del
  payload, que sobrescribe `OMP_NUM_THREADS`; ver la nota en `docs/07`.)
- **Nube**: `THREADS=2` fijo, variando réplicas del backend `r ∈ {1, 3}`.

Pega los percentiles y el throughput en las tablas de `docs/07`.
