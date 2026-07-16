"""Backend FastAPI — wrapper HTTP del motor C++/OpenMP.

Delega el cálculo al contenedor del motor (`http://<MOTOR_HOST>:<MOTOR_PORT>/move`).
La separación de contenedores es a nivel de red, no de proceso.
"""

from __future__ import annotations

import os
import threading
from contextlib import asynccontextmanager
from typing import AsyncIterator, Literal, Optional

import httpx
from fastapi import FastAPI, HTTPException, Response
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel, Field, field_validator

MOTOR_HOST = os.environ.get("MOTOR_HOST", "motor")
MOTOR_PORT = int(os.environ.get("MOTOR_PORT", "9000"))
MOTOR_BASE = f"http://{MOTOR_HOST}:{MOTOR_PORT}"
MOTOR_TIMEOUT_S = float(os.environ.get("MOTOR_TIMEOUT_S", "30"))


@asynccontextmanager
async def lifespan(app: FastAPI) -> AsyncIterator[None]:
    global _client
    # Si los tests ya inyectaron un cliente con MockTransport, lo respetamos.
    if _client is None:
        _client = httpx.AsyncClient(base_url=MOTOR_BASE, timeout=MOTOR_TIMEOUT_S)
    try:
        yield
    finally:
        if _client is not None:
            await _client.aclose()
            _client = None


app = FastAPI(
    title="Mancala/Kalah API",
    version="0.2.0",
    description="Wrapper HTTP del motor de Kalah(6,4).",
    lifespan=lifespan,
)

# --- CORS: orígenes explícitos, sin comodín "*" ---
ALLOWED_ORIGINS = [
    o.strip()
    for o in os.environ.get(
        "ALLOWED_ORIGINS",
        "http://localhost:8080,http://127.0.0.1:8080",
    ).split(",")
    if o.strip()
]

app.add_middleware(
    CORSMiddleware,
    allow_origins=ALLOWED_ORIGINS,
    allow_methods=["GET", "POST", "OPTIONS"],
    allow_headers=["Content-Type"],
)


# --- Schemas ---
# Contrato de la especificación (sección 2.3):
#   POST /move recibe { board, side, depth, threads }
#   responde       { move, evaluation, elapsed_ms, stats:{nodes,prunes}, threads_used }


class MoveRequest(BaseModel):
    board: list[int] = Field(..., min_length=14, max_length=14,
                             description="14 enteros: 12 hoyos + 2 kalahas en orden canónico.")
    side: Literal[0, 1] = Field(..., description="Jugador al que le toca mover.")
    depth: int = Field(..., ge=1, le=64,
                       description="Profundidad de búsqueda de Minimax + Alfa-Beta.")
    threads: int = Field(default=1, ge=1, le=64,
                         description="Número de hilos OpenMP del motor.")

    @field_validator("board")
    @classmethod
    def _no_negatives(cls, v: list[int]) -> list[int]:
        if any(x < 0 for x in v):
            raise ValueError("board no puede contener enteros negativos")
        return v


class MoveStats(BaseModel):
    nodes: int
    prunes: int


class MoveResponse(BaseModel):
    move: int
    evaluation: float
    elapsed_ms: int
    stats: MoveStats
    threads_used: int


# --- Cliente HTTP al motor ---

_client: Optional[httpx.AsyncClient] = None

# --- Métricas agregadas del motor (nodos visitados y podas) ---
# Se acumulan a partir del bloque `stats` que devuelve el motor en cada /move.
_metrics_lock = threading.Lock()
_total_moves = 0
_total_nodes = 0
_total_prunes = 0


def _record_motor_stats(stats: dict) -> None:
    global _total_moves, _total_nodes, _total_prunes
    with _metrics_lock:
        _total_moves += 1
        _total_nodes += int(stats.get("nodes", 0))
        _total_prunes += int(stats.get("prunes", 0))


async def _motor_healthy() -> bool:
    if _client is None:
        return False
    try:
        r = await _client.get("/healthz", timeout=2.0)
        return r.status_code == 200
    except (httpx.HTTPError, OSError):
        return False


# --- Endpoints ---

@app.get("/healthz")
def healthz() -> dict:
    """Liveness probe: el propio proceso del backend está vivo."""
    return {"status": "ok"}


@app.get("/readyz")
async def readyz() -> dict:
    """Readiness probe: 200 solo si el motor está accesible por la red interna."""
    if not await _motor_healthy():
        raise HTTPException(status_code=503, detail="motor no disponible")
    return {"status": "ready", "motor": MOTOR_BASE}


@app.get("/metrics")
def metrics() -> Response:
    """Métricas agregadas del motor en formato Prometheus (texto plano).

    Acumula los nodos explorados y las podas Alfa-Beta reportadas por el motor
    en cada `POST /move` desde el arranque del proceso.
    """
    with _metrics_lock:
        moves, nodes, prunes = _total_moves, _total_nodes, _total_prunes
    body = (
        "# HELP mancala_backend_info Información del backend.\n"
        "# TYPE mancala_backend_info gauge\n"
        f'mancala_backend_info{{version="0.2.0",motor="{MOTOR_BASE}"}} 1\n'
        "# HELP mancala_motor_moves_total Jugadas calculadas por el motor.\n"
        "# TYPE mancala_motor_moves_total counter\n"
        f"mancala_motor_moves_total {moves}\n"
        "# HELP mancala_motor_nodes_total Nodos del árbol explorados por el motor.\n"
        "# TYPE mancala_motor_nodes_total counter\n"
        f"mancala_motor_nodes_total {nodes}\n"
        "# HELP mancala_motor_prunes_total Podas Alfa-Beta efectuadas por el motor.\n"
        "# TYPE mancala_motor_prunes_total counter\n"
        f"mancala_motor_prunes_total {prunes}\n"
    )
    return Response(content=body, media_type="text/plain; charset=utf-8")


@app.post("/move", response_model=MoveResponse)
async def move(req: MoveRequest) -> MoveResponse:
    if _client is None:
        raise HTTPException(status_code=503, detail="cliente HTTP del motor no inicializado")

    # Reenviar tal cual el JSON al motor.
    payload: dict = req.model_dump()
    try:
        r = await _client.post("/move", json=payload)
    except httpx.HTTPError as e:
        raise HTTPException(status_code=503, detail=f"error contactando motor: {e}") from e

    if r.status_code >= 500:
        raise HTTPException(status_code=503,
                            detail=f"motor devolvió {r.status_code}: {r.text}")
    if r.status_code >= 400:
        # Propagar el error del motor con el mismo código si aplica.
        raise HTTPException(status_code=r.status_code, detail=r.text)

    data = r.json()
    _record_motor_stats(data.get("stats", {}))
    return MoveResponse(**data)
