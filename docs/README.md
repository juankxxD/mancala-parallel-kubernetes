# Informe del proyecto final

Este informe se reparte en ocho archivos Markdown temáticos más este índice.
Cada archivo es autocontenido y, cuando necesite referenciar a otro, enlaza
con ruta relativa.

## Índice

1. [Arquitectura](01-arquitectura.md)
2. [Motor de Mancala](02-motor.md)
3. [Paralelización con OpenMP](03-paralelizacion.md)
4. [Despliegue local](04-despliegue-local.md)
5. [Despliegue en la nube](05-despliegue-nube.md)
6. [CI/CD y calidad de código](06-cicd.md)
7. [Análisis comparativo local vs. nube](07-analisis-comparativo.md)
8. [Conclusiones](08-conclusiones.md)

## Mapeo criterio de la rúbrica → archivo

| Criterio                                          | Archivo                       |
| ------------------------------------------------- | ----------------------------- |
| Motor de Mancala: corrección                      | `02-motor.md`                 |
| Paralelización con OpenMP                         | `03-paralelizacion.md`        |
| Instrumentación local                             | `03-paralelizacion.md`        |
| Separación de componentes                         | `01-arquitectura.md`          |
| Despliegue local                                  | `04-despliegue-local.md`      |
| Despliegue en la nube con Kubernetes              | `05-despliegue-nube.md`       |
| CI/CD y calidad de código                         | `06-cicd.md`                  |
| Análisis comparativo local vs. nube               | `07-analisis-comparativo.md`  |
| Claridad de explicaciones                         | transversal a todos           |
| Conclusiones                                      | `08-conclusiones.md`          |

> Si en este índice falta una entrada o el archivo correspondiente está vacío
> la evaluación recorre los archivos en orden numérico y omite cualquier
> contenido que no se ubique sin ambigüedad.

## Cómo leer este informe

El orden de lectura natural es de `01` a `08`. Cada archivo es autocontenido:
puede leerse en aislamiento sin requerir contexto previo de los demás. Las
referencias cruzadas se resuelven con enlaces relativos
(`[ver paralelización](03-paralelizacion.md)`).

Las decisiones técnicas se documentan en el archivo donde primero aparecen,
no se centralizan en un archivo aparte. Por ejemplo, la elección de
FastAPI vs. Flask se justifica en `01-arquitectura.md`; la elección de
estrategia OpenMP en `03-paralelizacion.md`; la elección de proveedor de
nube en `05-despliegue-nube.md`. Si una decisión afecta a varios capítulos,
se enlaza desde los demás al capítulo donde se argumenta.

## Convenciones de formato

- **Diagramas** (arquitectura, despliegue, secuencia, *pipeline*) en
  bloques `mermaid` embebidos en el Markdown. Sin imágenes en lugar de
  diagramas.
- **Fórmulas** matemáticas en LaTeX embebido (`$...$` y `$$...$$`).
- **Capturas** de consola, `kubectl`, *dashboards* o resultados de
  *benchmarks* en `imagenes/` referenciadas con ruta relativa. Las
  capturas son evidencia experimental, no sustitutos de un diagrama.
- **Tablas** en Markdown nativo, no en imágenes ni en HTML embebido.
- **Bloques de código** con la etiqueta de lenguaje correcta
  (`yaml`, `python`, `cpp`, `bash`, etc.) para que GitHub aplique
  resaltado de sintaxis.

## Qué hacer con este índice

Antes del primer *push* significativo, verifiquen que cada archivo del
informe tiene al menos un párrafo de contenido propio (no sólo los
comentarios HTML de plantilla). Antes de la entrega final, marquen los
criterios completados con un *checkbox* (`[x]`) frente a la fila
correspondiente en la tabla de mapeo. La revisión del docente recorre el
informe en este orden, así que un capítulo incompleto se nota
inmediatamente.
