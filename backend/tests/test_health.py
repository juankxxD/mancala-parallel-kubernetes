"""Tests del backend con el motor mockeado vía httpx.MockTransport.

No requieren un motor real corriendo: el cliente AsyncClient se reemplaza
en startup por uno apuntando a un transport que responde con respuestas
fijas. Esto valida la capa de validación pydantic y el reenvío al motor.
"""

from __future__ import annotations

import json
from typing import Optional

import httpx
import pytest
from fastapi.testclient import TestClient

from app import main as backend_main


# --- Fake transport del motor ---

class FakeMotor:
    def __init__(self) -> None:
        self.healthy = True
        self.last_payload: Optional[dict] = None

    def handler(self, request: httpx.Request) -> httpx.Response:
        if request.url.path == "/healthz":
            if not self.healthy:
                return httpx.Response(503, json={"status": "down"})
            return httpx.Response(200, json={"status": "ok"})
        if request.url.path == "/move":
            self.last_payload = json.loads(request.content)
            body = {
                "move": 2,
                "evaluation": 7,
                "elapsed_ms": 12,
                "stats": {"nodes": 100, "prunes": 20},
                "threads_used": self.last_payload.get("threads", 1),
            }
            return httpx.Response(200, json=body)
        return httpx.Response(404, json={"error": "not found"})


@pytest.fixture
def motor() -> FakeMotor:
    return FakeMotor()


@pytest.fixture
def client(motor: FakeMotor):
    # Inyectamos un AsyncClient con MockTransport ANTES de que arranque el
    # lifespan; main.lifespan respeta el cliente preexistente.
    transport = httpx.MockTransport(motor.handler)
    backend_main._client = httpx.AsyncClient(
        base_url=backend_main.MOTOR_BASE,
        transport=transport,
        timeout=2.0,
    )
    # Reiniciar los contadores agregados para que cada test sea determinista.
    backend_main._total_moves = 0
    backend_main._total_nodes = 0
    backend_main._total_prunes = 0
    with TestClient(backend_main.app) as c:
        yield c
    # El lifespan cierra el cliente al salir del with.
    backend_main._client = None


# --- Tests ---

def test_healthz(client):
    r = client.get("/healthz")
    assert r.status_code == 200
    assert r.json() == {"status": "ok"}


def test_readyz_when_motor_up(client, motor):
    motor.healthy = True
    r = client.get("/readyz")
    assert r.status_code == 200
    assert r.json()["status"] == "ready"


def test_readyz_when_motor_down(client, motor):
    motor.healthy = False
    r = client.get("/readyz")
    assert r.status_code == 503


def test_metrics_returns_plain_text(client):
    r = client.get("/metrics")
    assert r.status_code == 200
    assert r.headers["content-type"].startswith("text/plain")
    assert "mancala_backend_info" in r.text
    # Métricas agregadas del motor exigidas por la spec (nodos y podas).
    assert "mancala_motor_nodes_total" in r.text
    assert "mancala_motor_prunes_total" in r.text


def test_move_forwards_to_motor(client, motor):
    payload = {
        "board": [4, 4, 4, 4, 4, 4, 0, 4, 4, 4, 4, 4, 4, 0],
        "side": 0,
        "depth": 8,
        "threads": 2,
    }
    r = client.post("/move", json=payload)
    assert r.status_code == 200
    body = r.json()
    assert body["stats"]["nodes"] == 100
    assert body["stats"]["prunes"] == 20
    assert body["threads_used"] == 2
    # El payload reenviado al motor debe coincidir con lo que envió el cliente.
    assert motor.last_payload["depth"] == 8
    assert motor.last_payload["side"] == 0
    assert "algo" not in motor.last_payload


def test_move_aggregates_metrics(client):
    payload = {
        "board": [4, 4, 4, 4, 4, 4, 0, 4, 4, 4, 4, 4, 4, 0],
        "side": 0,
        "depth": 8,
        "threads": 1,
    }
    client.post("/move", json=payload)
    client.post("/move", json=payload)
    r = client.get("/metrics")
    assert "mancala_motor_moves_total 2" in r.text
    assert "mancala_motor_nodes_total 200" in r.text
    assert "mancala_motor_prunes_total 40" in r.text


def test_move_invalid_board_length(client):
    payload = {
        "board": [4, 4, 4],
        "side": 0,
        "depth": 8,
        "threads": 1,
    }
    r = client.post("/move", json=payload)
    assert r.status_code == 422


def test_move_requires_depth(client):
    payload = {
        "board": [4, 4, 4, 4, 4, 4, 0, 4, 4, 4, 4, 4, 4, 0],
        "side": 0,
        "threads": 1,
    }
    r = client.post("/move", json=payload)
    assert r.status_code == 422


def test_move_negative_seeds_rejected(client):
    payload = {
        "board": [-1, 4, 4, 4, 4, 4, 0, 4, 4, 4, 4, 4, 4, 0],
        "side": 0,
        "depth": 4,
        "threads": 1,
    }
    r = client.post("/move", json=payload)
    assert r.status_code == 422
