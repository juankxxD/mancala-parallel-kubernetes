# Misma base en build y runtime para evitar mismatch de libstdc++.
FROM debian:bookworm-slim AS builder

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        g++ cmake make ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY src/ ./src/
COPY tests/ ./tests/

RUN cmake -S src -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --parallel \
    && ctest --test-dir build --output-on-failure

# --- Runtime ---
FROM debian:bookworm-slim AS runtime

RUN apt-get update \
    && apt-get install -y --no-install-recommends libgomp1 libstdc++6 \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --create-home --shell /bin/bash motor

WORKDIR /app
COPY --from=builder /build/build/mancala_motor /app/mancala_motor
COPY --from=builder /build/build/mancala_bench /app/mancala_bench
COPY --from=builder /build/tests/suite.txt /app/tests/suite.txt

USER motor

ENV OMP_NUM_THREADS=4 \
    MOTOR_PORT=9000 \
    MOTOR_WORKERS=8

EXPOSE 9000

CMD ["/bin/sh", "-c", "exec /app/mancala_motor --serve --port ${MOTOR_PORT} --workers ${MOTOR_WORKERS}"]
