# Reenvío masivo + selección desde perfil + iconos de módulos + registro de cambios

## Orden de trabajo OBLIGATORIO (no saltar pasos)

### PASO 1 — Estudiar la tecnología
Antes de proponer nada:
1. Lee y usa como referencia los documentos de:
   `C:\Users\VICTOR\code\docs-tdesktop`
   - `00-INDICE.md`
   - `01-ENTORNO.md`
   - `02-PLANES.md`
   - `03-CAMBIOS.md`
   - `04-MEJORAS.md`
2. Revisa el log más reciente en:
   `C:\Users\VICTOR\code\tdesktop\out\Release`
3. Estudia cómo funciona actualmente en el código:
   - El reenvío por lotes
   - El límite de ~100 mensajes de la API de Telegram (`forwardMessages`)
   - Por qué se detiene alrededor de los ~2000
   - Las restricciones `noforwards` / `CHAT_FORWARDS_RESTRICTED`
   - La selección múltiple en los módulos del perfil
4. Resume qué se intentó antes (según los docs) y **qué no funcionó**, para no repetir el mismo camino.

### PASO 2 — Elaborar un plan nuevo
Con lo aprendido, elabora un **plan distinto** a los anteriores que fallaron.

El plan debe incluir:

**A. Reenvío masivo que no se corte**
- Dividir en lotes de máximo 100 (límite real de la API).
- Cola secuencial que no aborte toda la operación por un error intermedio.
- Contador de progreso en tiempo real: `Enviados: X / Total` (y cuántos faltan).

**Planes de contingencia (obligatorios):**
1. FloodWait / timeout / error temporal → esperar y reintentar el lote.
2. Contenido restringido (`noforwards` / `CHAT_FORWARDS_RESTRICTED`) → cambiar automáticamente a modo **copia** (send as copy / descargar + reenviar como mensaje nuevo).
3. Un mensaje individual falla → registrarlo, saltarlo y continuar con el resto.

**B. Selección desde el perfil**
- En los módulos del perfil debe funcionar correctamente:
  - Seleccionar todos
  - Deseleccionar todos / ninguno
- El número seleccionado debe coincidir con lo realmente cargado.

**C. Iconos distintos en el perfil**
Actualmente varios módulos del perfil usan el **mismo icono**.  
Cambia los iconos para que cada módulo se diferencie visualmente:
- Archivos multimedia
- Archivos
- Photos/Files
- Media items
- Enlaces (si aplica)

Cada uno debe tener un icono propio y claro.

### PASO 3 — Evaluar factibilidad
Antes de tocar código, responde de forma explícita:
- ¿El plan es viable con la arquitectura actual?
- ¿Qué archivos/funciones exactas se van a modificar?
- ¿Qué riesgo hay de romper Media, Photos & Files, All u otras funciones ya estables?
- ¿Cómo se evita repetir los enfoques que ya fallaron según los docs?

Si el plan no es sólido, ajústalo antes de continuar.

### PASO 4 — Ejecutar en el código
Solo después de los pasos 1–3:
- Implementa el cambio de forma mínima y controlada.
- Usa Spec Kit para ordenar la implementación.
- Al terminar, **crea un archivo de registro nuevo** en:
  `C:\Users\VICTOR\code\docs-tdesktop`
  con un nombre claro y distinto (ejemplo: `05-REGISTRO-REENVIO-MASIVO.md` o el siguiente número disponible).

El archivo de registro debe contener:
- Fecha
- Qué se intentó antes y por qué no funcionó
- Qué plan nuevo se eligió
- Qué archivos se modificaron
- Qué se logró
- Qué quedó pendiente (si hay algo)

## Restricciones estrictas
- NO repetir el mismo enfoque de los intentos anteriores que fallaron.
- NO romper Media, Photos & Files, All ni el resto de módulos estables.
- NO romper el reenvío normal de pocos mensajes.
- Se pueden cambiar funciones internas si hace falta, pero con el menor impacto posible.
- Cada mejora nueva debe dejar su propio archivo de registro para que los próximos prompts partan de ahí.

## Resultado esperado
1. Reenvío de miles de mensajes termina completo (no se corta en ~2000).
2. Contador visible de enviados / faltantes.
3. Contenido restringido se puede reenviar (vía copia cuando el forward nativo esté bloqueado).
4. Desde el perfil se puede seleccionar todos / ninguno correctamente.
5. Cada módulo del perfil tiene un icono distinto.
6. Nuevo archivo de registro creado en `C:\Users\VICTOR\code\docs-tdesktop`.

Empieza por el PASO 1 (estudiar docs + logs + código). No implementes nada hasta completar el plan y la evaluación de factibilidad.