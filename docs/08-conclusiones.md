# 08 — Conclusiones

## Limitaciones encontradas

- La paralelización que usamos es la más sencilla (paralelismo a la raíz). Funciona bien y da
  el mismo movimiento que la versión secuencial, pero no aprovecha al máximo todos los hilos:
  al usar 8 hilos solo conseguimos un speedup de 2.20 (depth=12), no de 8. Esto pasa porque al
  repartir el trabajo entre hilos se pierden algunas "podas" del Alfa-Beta y se terminan
  explorando más nodos (con 8 hilos se exploraron casi 1.5 veces más nodos que con 1).
- Las pruebas en la nube las hicimos en un clúster pequeño de GKE con la cuenta de créditos, así
  que los recursos eran limitados. Por eso la latencia en la nube salió más alta que en local.
- No alcanzamos a implementar estrategias más avanzadas (YBWC o PVS) por tiempo; las dejamos como
  trabajo futuro.

## Retos durante el desarrollo

- **CORS:** al principio el navegador bloqueaba las peticiones del frontend al backend. Aprendimos
  que era por la política de "mismo origen" y lo solucionamos configurando CORS en el backend con
  los orígenes permitidos (no con `*`).
- **Comunicación entre contenedores:** el backend no encontraba al motor. El problema era que cada
  contenedor es independiente, así que tuvimos que usar el nombre del servicio (`MOTOR_HOST=motor`)
  para que se encontraran por la red interna, en vez de usar `localhost`.
- **Entender por qué con más hilos no iba mucho más rápido:** esto nos sorprendió. Al medir los
  nodos y las podas entendimos que paralelizar rompe el orden de las cotas α y β del Alfa-Beta y
  por eso se pierden podas. Fue uno de los aprendizajes más importantes del proyecto.
- **Health checks (probes) de Kubernetes:** al inicio los pods se reiniciaban solos. Era porque las
  probes apuntaban mal o sin suficiente tiempo de espera. Lo arreglamos ajustando `/healthz` y
  `/readyz` y los tiempos de `initialDelaySeconds`.

## Lecciones aprendidas

- Separar la aplicación en contenedores (motor, backend y frontend) hace todo más ordenado: cada
  parte se puede arreglar, reiniciar o escalar por separado.
- Medir antes de opinar: muchas cosas que creíamos (como "más hilos = siempre más rápido") no eran
  ciertas hasta que las medimos con tablas y gráficas.
- Tener el CI/CD (GitHub Actions) ayudó mucho, porque cada vez que subíamos código se compilaba el
  motor y se corrían las pruebas automáticamente, y nos avisaba si algo se rompía.

## Recomendaciones de mejora futura

- Implementar una estrategia de paralelización mejor (YBWC o PVS) para perder menos podas y ganar
  más velocidad con los hilos.
- Agregar una tabla de transposición para no recalcular posiciones que ya se evaluaron antes.
- Mejorar la observabilidad usando Prometheus y Grafana para ver las métricas del motor en tiempo
  real, en vez de solo el endpoint `/metrics`.
- Hacer pruebas de carga más grandes en la nube si se tuviera un clúster con más recursos.
