# Documentación del proyecto Telegram Desktop personalizado

Carpeta central de documentación. Todo lo relacionado al trabajo sobre
`C:\Users\VICTOR\code\tdesktop` está aquí, ya no hay archivos sueltos.

## Estado actual (agosto 2026)

- Árbol de trabajo único: `code\tdesktop` (la versión 1 fue eliminada tras portar todo).
- Todas las mejoras aplicadas y verificadas en compilación.
- Toolchain compartido en `code\Libraries` + `code\ThirdParty`.

## Funcionalidades implementadas en la app

Todo lo que hace distinta esta build frente al Telegram Desktop original.
Detalle técnico de cada punto en [12-CAMBIOS-RONDAS-1-22B.md](12-CAMBIOS-RONDAS-1-22B.md).

### Gestión de contenido (desbloqueos)
| # | Función | En qué consiste |
|---|---|---|
| 1 | Copiar texto desbloqueado | Copiar texto en canales que lo prohibían |
| 2 | Guardar/descargar desbloqueado | Guardar fotos, videos y documentos restringidos |
| 3 | Selección de media desbloqueada | Marcar fotos/videos aunque el canal "no permita reenvíos" |
| 4 | Selección sin límite de 100 | Marcar 200, 500 o 900+ ítems a la vez |
| 5 | Shift + clic | Seleccionar por rangos de un golpe |
| 6 | Seleccionar todo | Ctrl+A, menú contextual o botón superior (check circular) |
| 7 | Cuadrícula densa | Miniaturas más pequeñas para ver más por pantalla |
| 8 | Descargar siempre visible | Botón en el visor de media que fuerza la descarga original |
| 9 | Ctrl+Shift+G en el visor | Saltar al mensaje en el chat desde la foto abierta |

### Galería y perfiles
| # | Función | En qué consiste |
|---|---|---|
| 10 | Botón único "Media" | Fotos y videos juntos en el perfil (incluye layout classic del canal) |
| 11 | Filtros Todo / Fotos / Videos | Submenú superior de la galería unificada (sin GIF), "Todo" por defecto |
| 12 | Pestañas ocultas si vacías | Fotos/Videos solo se muestran si hay contenido de ese tipo |
| 13 | Módulo "Todo" (All) | Una cuadrícula con fotos + videos + archivos (sin stickers/GIF/voz), carga completa y orden cronológico |
| 14 | Módulo "Fotos + Archivos" | En el perfil del canal: fotos y archivos juntos (sin videos), con lazy loading, conteo combinado, soporte de foros/temas, carga completa y nombres de archivo ocultos; se obtiene fusionando dos búsquedas MTP (Photo + File) |

### Reenvío y borrado masivos
| # | Función | En qué consiste |
|---|---|---|
| 15 | Reenvío masivo estable | Lotes de 100 en orden cronológico; 900+ llegan completos |
| 16 | Cadena secuencial | Una petición en vuelo a la vez (sin flood ni desorden entre lotes) |
| 17 | Sin límite práctico | Selección de hasta 10000 ítems (MaxSelectedItems) |
| 18 | Fallo que no rompe | Un lote con error se reporta y la cadena continúa; successCallback siempre al final |
| 19 | Opciones recordadas | "Sin autor" / "sin captions" se recuerdan entre reenvíos |
| 20 | Orden cronológico | Primero los antiguos, después los recientes |
| 21 | Borrado masivo fiable | Lotes de 100 al servidor; los borrados NO reaparecen |

### Contenido protegido (canales/grupos con `noforwards`)
| # | Función | En qué consiste |
|---|---|---|
| 22 | Selección desbloqueada | Se puede entrar en modo selección y "Seleccionar todos" aunque el canal prohíba copiar/reenviar |
| 23 | Copiar desbloqueado | Copiar texto y contenido aunque "no esté permitido copiar" |
| 24 | Reenviar disponible | "Reenviar" aparece en menú contextual y también al seleccionar varios mensajes |
| 25 | Sin aviso de bloqueo | Desaparece el mensaje "No está permitido copiar ni reenviar..." en la UI |
| 26 | Límite del servidor | El reenvío "puro" de contenido protegido puede dar `CHAT_FORWARDS_RESTRICTED` (la UI queda disponible; ver 12-CAMBIOS-RONDAS-1-22B.md Ronda 22) |

### Estabilidad de carga
- Ventana de carga grande (≈2000 ids) en "Todo" y "Fotos + Archivos", mantenida al hacer scroll.
- Materialización proactiva de todo el contenido, con paralelismo acotado (3 peticiones en vuelo) para no congelar la app.
- Fin de lista honesto (sin "tope fantasma" que cortase la lista antes de tiempo).
- Anti-bucle de reintentos y auto-reparación de búsquedas fallidas (PEER_ID_INVALID / red).
- Depurador propio: archivo `depurador.txt` con diagnostico de la carga de FilesPhotos/All.

### Correcciones de crash destacadas
- "Seleccionar todo" en Fotos + Archivos (geometría mixta foto/documento en la cuadrícula).
- Arranque del módulo Fotos + Archivos (caso `Type::FilesPhotos` en la barra superior).
- Cierre inesperado al reenviar varios archivos a la vez: el callable recursivo de la cadena de reenvío se auto-capturaba por valor (versión vacía) → `std::bad_function_call` → `abort`. Ahora se guarda en `shared_ptr<std::function>` y solo se invoca si existe; los lotes con peer origen vacío se saltan (evita `INPUT_PEERS_EMPTY`).

## Índice

| Archivo | Qué contiene | Cuándo consultar |
|---|---|---|
| [12-CAMBIOS-RONDAS-1-22B.md](12-CAMBIOS-RONDAS-1-22B.md) | Resumen sencillo de TODOS los cambios aplicados hasta hoy | Quieres saber qué hace distinta esta app |
| [13-PLAN-GALERIA-REENVIO-MASIVO.md](13-PLAN-GALERIA-REENVIO-MASIVO.md) | Plan de implementación original (marcado como completado) | Contexto técnico de cómo se hizo cada cambio |
| [11-DIARIO-RONDAS.md](11-DIARIO-RONDAS.md) | Estado sección por sección + archivos tocados + reglas de trabajo | Antes de tocar código nuevo |
| [17-MEJORAS-CATALOGO-ANTERIOR.md](17-MEJORAS-CATALOGO-ANTERIOR.md) | Catálogo de lo que AÚN se puede mejorar, con cómo hacerlo y su riesgo | Planear la siguiente mejora |
| [16-TOOLCHAIN-COMPILADOR.md](16-TOOLCHAIN-COMPILADOR.md) | Qué pesa en `code\`, qué conservar y cómo reutilizar el compilador en versiones nuevas | Limpiar disco o compilar una versión nueva |
| [historico-errores.txt](historico-errores.txt) | Log de errores de compilación antiguos (ya resueltos) | Solo referencia histórica |

## Regla de oro para futuros cambios

Primero investigar la causa raíz, después corregir de forma mínima y localizada,
y **nunca romper lo que ya funciona**. Cada corrección aplicada se registra en
`12-CAMBIOS-RONDAS-1-22B.md` y `11-DIARIO-RONDAS.md`.
