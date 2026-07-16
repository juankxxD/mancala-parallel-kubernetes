-- Script para wrk: envía POST /move con un cuerpo JSON fijo.
--
-- Uso:
--   wrk -t4 -c50 -d30s -s post_move.lua http://<host>:8000/move
--
-- Mantén la misma posición y profundidad en local y en la nube para que la
-- comparación sea honesta.

wrk.method = "POST"
wrk.headers["Content-Type"] = "application/json"
wrk.body = [[
{
  "board": [4,4,4,4,4,4,0,4,4,4,4,4,4,0],
  "side": 0,
  "depth": 8,
  "threads": 2
}
]]
