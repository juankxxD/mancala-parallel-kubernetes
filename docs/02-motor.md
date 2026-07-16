# 02 — Motor de Juego

## Reglas de Kalah(6,4)

Variante estándar de Mancala implementada en `motor/src/board.cpp`:

- Tablero con **2 filas de 6 hoyos**, **4 semillas** inicialmente en cada hoyo.
- Cada jugador tiene un **kalaha** (almacén) a su derecha.
- En su turno, el jugador toma todas las semillas de uno de sus hoyos y las
  reparte una a una en **sentido antihorario**, incluyendo su propio kalaha pero
  **saltando** el del oponente.
- Si la última semilla cae en el kalaha propio → **turno extra**.
- Si la última semilla cae en un hoyo propio que estaba vacío y el hoyo opuesto
  del rival tiene semillas → **captura**: ambas (la que llegó más las del
  opuesto) van al kalaha propio.
- El juego termina cuando un lado queda sin semillas; las semillas que queden en
  el otro lado se recogen a su kalaha y gana quien tenga más.

Estas reglas se implementan en `Board::apply_move`, `Board::terminal`,
`Board::collect_remaining` y `Board::winner`. La representación es un arreglo de
14 enteros con el orden canónico documentado en [01-arquitectura.md](01-arquitectura.md).

## Algoritmo — Minimax con poda Alfa-Beta

Es el algoritmo exigido por el proyecto. Hace una búsqueda en profundidad fija
(`depth`) sobre el árbol de juego (`motor/src/alphabeta.cpp`).

### Función de evaluación heurística

$$
h(\text{estado}) = (\text{kalaha propio} - \text{kalaha rival}) + \alpha \cdot (\text{semillas lado propio} - \text{semillas lado rival})
$$

con $\alpha \in [0, 1]$ definido por el grupo. En esta entrega $\alpha = 0.1$: el
diferencial de kalahas domina la evaluación (es la ventaja material segura) y el
diferencial de semillas en el tablero actúa como desempate ligero. La evaluación
devuelve un entero; un estado terminal se valora con el diferencial de kalahas
multiplicado por 1000 para que ganar/perder pese mucho más que cualquier ventaja
heurística intermedia.

### Manejo del turno extra

El turno extra rompe la alternancia estricta de Minimax (un jugador puede mover
varias veces seguidas). Para manejarlo sin casos especiales, la búsqueda se hace
siempre desde la perspectiva del lado de la raíz (`root_side`): en cada nodo se
**maximiza** si mueve `root_side` y se **minimiza** si mueve el rival, leyendo el
turno real del estado en lugar de asumir que alterna. Así un turno extra
simplemente encadena dos nodos maximizadores (o dos minimizadores) seguidos.

### Pseudocódigo

```text
funcion alphabeta(estado, depth, alpha, beta, root_side):
    si estado.terminal():
        retornar diferencial_de_kalahas(estado, root_side) * 1000
    si depth == 0:
        retornar h(estado)

    maximizando = (estado.turno == root_side)
    si maximizando:
        valor = -INF
        para cada movimiento legal m:
            valor = max(valor, alphabeta(aplicar(estado, m), depth-1, alpha, beta, root_side))
            alpha = max(alpha, valor)
            si alpha >= beta: poda; break       # poda beta
        retornar valor
    si no:
        valor = +INF
        para cada movimiento legal m:
            valor = min(valor, alphabeta(aplicar(estado, m), depth-1, alpha, beta, root_side))
            beta  = min(beta, valor)
            si alpha >= beta: poda; break        # poda alpha
        retornar valor
```

### Criterio de corrección

La poda no debe cambiar el resultado: a igual profundidad, Alfa-Beta debe
producir el **mismo valor (y el mismo movimiento óptimo)** que Minimax sin poda.
Esta equivalencia es uno de los criterios de la rúbrica y se verifica en las
pruebas unitarias (ver más abajo).

## Suite de pruebas unitarias

En `motor/tests/test_runner.cpp` (runner propio, sin framework externo). Cubre:

- **Reglas**: tablero inicial, movimientos legales, siembra básica, turno extra
  en kalaha, salto del kalaha rival, regla de captura, fin de juego y recogida.
- **Equivalencia Alfa-Beta = Minimax**: para varias posiciones y profundidades
  (3, 5, 7) el valor de Alfa-Beta coincide con el de un Minimax sin poda
  programado aparte en el propio test.
- **Paralelo = secuencial**: el valor de la búsqueda con 4 hilos coincide con el
  de 1 hilo sobre la misma posición.

Cómo ejecutarla:

```bash
cd motor
cmake -S src -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Salida esperada (todas las comprobaciones en verde):

```text
[tests] 56/56 pasaron, 0 fallaron
```

> Evidencia de la equivalencia Alfa-Beta ↔ Minimax: el test
> `test_alphabeta_equals_minimax_serial` falla si la poda cambia el valor del
> nodo raíz respecto al Minimax exhaustivo, así que un `ctest` en verde es la
> prueba de que la poda es correcta sobre el conjunto de posiciones de prueba.
