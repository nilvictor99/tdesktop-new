# Documentación — Telegram Desktop personalizado

Esta carpeta es `C:\Users\VICTOR\code\docs-tdesktop`. Documenta **exclusivamente**
el fork de Telegram Desktop en `C:\Users\VICTOR\code\tdesktop`.

Aquí se registran todos los cambios aplicados, ordenados por rondas correlativas.

> No confundir con `C:\Users\VICTOR\code\tdesktop\docs\`, que es documentación
> oficial de Telegram Desktop (upstream) y no de este proyecto.

## Cómo está organizada esta carpeta

Dos series con rangos numéricos separados. La serie `00-08` es la documentación
**vigente**: es la que hay que usar para trabajar. La serie `10-19` es la
documentación **histórica** rescatada de la carpeta `temp\` (ya eliminada), que se
conserva porque contiene detalle que la serie vigente no reproduce.

### Serie vigente (`00-08`)

| Archivo | Contiene | Cuándo consultar |
|---|---|---|
| `00-INDICE.md` | Este índice + resumen de funcionalidades | Quieres saber qué hace distinta esta app |
| `01-ENTORNO.md` | Toolchain, uso de disco, cómo compilar y reutilizar el build en versiones nuevas | Limpiar disco o compilar |
| `02-PLANES.md` | Índice de los diseños previos (planos) de los módulos implementados | Contexto técnico de cómo se hizo |
| `03-CAMBIOS.md` | Registro cronológico por Ronda: problema → causa → fix → archivos. Llega a la **Ronda 26.1** | Detalle de cualquier cambio aplicado |
| `04-MEJORAS.md` | Catálogo de mejoras futuras (riesgo) + límites técnicos | Planear la siguiente mejora |
| `05-REGISTRO-REENVIO-MASIVO.md` | Historia completa del reenvío masivo (>1000 msgs): qué funcionó y qué no | Depurar problemas de reenvío |
| `06-CAMBIOS-GIT.md` | Detalle por archivo del diff `eef2d48..41286e02` (914 líneas) | Saber qué toca en cada fichero |
| `07-ESTADO-ACTUAL.md` | Estado real de la app tras la última reversión | Punto de partida del trabajo actual |
| `08-DIVERGENCIAS.md` | Diff real frente a Telegram Desktop oficial `v7.0.9` | Base oficial y alcance del fork |

### Serie histórica (`10-19`) — detalle que la serie vigente no cubre

| Archivo | Contiene | Por qué se conserva |
|---|---|---|
| `10-INVENTARIO-ANTERIOR.md` | Índice y funcionalidades tal como estaban en agosto 2026 | Permite ver la evolución del propio índice |
| `11-DIARIO-RONDAS.md` | **Diario técnico ronda a ronda (927 líneas)**, el más completo | Detalle de las Rondas 1-22 que `03-CAMBIOS.md` resume |
| `12-CAMBIOS-RONDAS-1-22B.md` | Registro por Ronda **hasta la 22b**, con más detalle que `03-CAMBIOS.md` (676 líneas) | Contiene la **Ronda 19.4**, ausente en `03-CAMBIOS.md` |
| `13-PLAN-GALERIA-REENVIO-MASIVO.md` | Plan original de galería y reenvío/borrado masivo | Versión completa del plan de las Rondas 1-6 |
| `14-PLAN-MODULO-ALL.md` | Plan de implementación del módulo "Todo" (All) | Versión completa del plan de la Ronda 9 |
| `15-PLAN-FIX-CARGA-COMPLETA.md` | Plan del fix de carga completa de FilesPhotos/All | Versión completa del plan de las Rondas 11-19.7 |
| `16-TOOLCHAIN-COMPILADOR.md` | Plan del "compiler tool" reutilizable | Antecedente de `01-ENTORNO.md` |
| `17-MEJORAS-CATALOGO-ANTERIOR.md` | Catálogo de mejoras en su versión previa | Antecedente de `04-MEJORAS.md` |
| `18-PROMPT-MODULO-COMBINADO.md` | Prompt de trabajo del módulo combinado FilesPhotos/All | Cómo se encargó el módulo combinado |
| `19-PROMPT-REENVIO-ICONOS.md` | Prompt de reenvío masivo, selección desde perfil e iconos | Cómo se encarga el trabajo por lotes |

### Reglas de esta carpeta

- Los nombres llevan prefijo numérico: `00-08` vigente, `10-19` histórico.
- Para añadir documentación **nueva y vigente**, usar el siguiente número libre de
  la serie `00-08` y actualizar la tabla de arriba.
- No se borra nada de la serie `10-19`: aunque esté superado por la vigente, es la
  única copia de parte del detalle.
- Los enlaces entre documentos usan el nombre del fichero completo, de modo que
  un rename obliga a actualizar las referencias en el resto de la carpeta.

## Funcionalidades implementadas

| # | Función | Ronda |
|---|---|---|
| 1 | Copiar texto desbloqueado | 22 |
| 2 | Guardar/descargar desbloqueado | 22 |
| 3 | Selección de media desbloqueada | 22 |
| 4 | Selección sin límite de 100 | 1 |
| 5 | Shift + clic (rangos) | 1 |
| 6 | Seleccionar todo: Ctrl+A + menú + botón | 1, 3 |
| 7 | Cuadrícula densa (Archivos Lista/4/6/8) | 5 |
| 8 | Filtros Todo / Fotos / Videos (sin GIF) | 1, 10 |
| 9 | Botón único "Media" en el perfil (incl. classic) | 3 |
| 10 | Reenvío masivo estable (lotes de 100, cronológico) | 4, 5, 6 |
| 11 | Cadena secuencial, 1 petición en vuelo, fallo que no rompe | 20, 21 |
| 12 | Sin límite práctico de selección (50000) | 20 |
| 13 | Opciones "Sin autor" / "sin captions" recordadas | 1 |
| 14 | Borrado masivo fiable (no reaparece) | 4 |
| 15 | Descargar siempre visible / Ctrl+Shift+G | 1 |
| 16 | Módulo "Todo" (All): fotos+videos+archivos | 9 |
| 17 | Módulo "Fotos + Archivos" (FilesPhotos) | 7 |
| 18 | Carga completa de FilesPhotos/All sin huecos | 11–19.7 |
| 19 | Reenvío "como copia" ante `CHAT_FORWARDS_RESTRICTED` | 22b |
| 20 | Reenvío con reintentos (3x) + gasto único de estrellas | 22c |
| 21 | Contador en vivo "Reenviados: X de N" | 22d |
| 22 | Watchdog anti-cuelgue: el reenvío masivo SIEMPRE termina | 22e |

Todo el detalle (problema, causa, fix, archivos) está en `03-CAMBIOS.md`.

## Notas honestas

- El flag `noforwards` es del **servidor**: la app ya permite seleccionar/copiar,
  pero un reenvío "puro" puede dar `CHAT_FORWARDS_RESTRICTED` → logrado con el
  fallback "reenviar como copia" (Ronda 22b).
- Copiar texto y guardar archivos funcionan porque esos flujos no se validan en
  el servidor.
- "Seleccionar todo" marca lo cargado en memoria; no descarga todo el historial.
- El flag `noforwards` es del servidor y el reenvío "puro" puede dar
  `CHAT_FORWARDS_RESTRICTED`; la app lo resuelve reenviando como copia (22b).
- El reenvío masivo tiene cierre garantizado: un watchdog por lote (10 min de
  paciencia) descongela la cadena aunque la capa MTP se atasque en un reintento
  silencioso (22e). Los errores temporales se reintentan hasta 3 veces (22c) y
  los FLOOD_WAIT los resuelve la propia capa MTP sin cortar el reenvío.

## Regla de oro (permanente)

Investigar la causa raíz primero, corregir de forma mínima y localizada, y
**nunca romper lo que ya funciona**. Cada cambio se registra en `03-CAMBIOS.md`.
El agente no compila: entrega el cambio y el usuario compila con:

```
msbuild Telegram.sln /p:Configuration=Release /p:Platform=x64 /m /t:Telegram
```