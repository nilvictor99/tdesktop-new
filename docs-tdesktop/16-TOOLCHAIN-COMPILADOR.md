# Plan — Herramienta de compilación reutilizable para Telegram Desktop ("compiler tool")

**Fecha:** 22 de agosto de 2026  
**Carpeta analizada:** `C:\Users\VICTOR\code` (~126 GB totales)

---

## 1. Diagnóstico: qué pesa y por qué

### Desglose medido (datos reales)

| Carpeta | Tamaño | Qué es | ¿Se puede borrar? |
|---|---|---|---|
| `tdesktopver01\out\` | **64 GB** | Artefactos de compilación de la versión antigua (archivos `.obj`, `.pdb`, exe intermedios) | ✅ SÍ — 100% regenerable y esa versión ya cumplió su papel |
| `tdesktop\out\` | **38 GB** | Artefactos de la compilación actual: `out\Telegram\Telegram.dir` = 23 GB de `.obj` intermedios, `td_ui.dir` = 4.1 GB, `Release` = 3.8 GB | ⚠️ Sí, pero se regeneran al recompilar (tarda) — borrar solo cuando no vayas a compilar en un tiempo |
| `Libraries\win64\` | **21 GB** | Dependencias de terceros YA COMPILADAS: Qt (9.4 GB), tg_angle (2.4 GB), tg_owt/WebRTC (2.3 GB), libjxl (1.2 GB), protobuf (1 GB), ffmpeg (711 MB), openssl3 (666 MB), openh264, tde2e, etc. | ❌ NO — **esta es tu herramienta de trabajo**, reutilizable entre versiones |
| `ThirdParty\` | 557 MB | Herramientas auxiliares (gyp, etc.) | ❌ NO — parte del toolchain |
| `tdesktop\.git` | 80 MB | Historial git del repo limpio | ❌ Útil para comparar con upstream |
| `tdesktopver01\.git` | 735 MB | Historial de la versión 1 | ⚠️ Opcional si ya no necesitas ver01 |
| Fuentes (`Telegram\`) | ~430 MB | Código fuente de ambas versiones | ❌ Esencial |

### ¿Por qué pesa tanto?

1. **Las carpetas `out\` (102 GB juntas)** son basura de compilador: MSVC genera un archivo `.obj` con información de depuración completa por CADA archivo `.cpp` del proyecto, más bases de datos de símbolos `.pdb` enormes. Son miles de archivos temporales. **Ningún valor permanente — se regeneran solos al compilar.**

2. **`Libraries` (21 GB)** es pesado porque cada librería se compiló desde código fuente con símbolos de depuración. Qt solo ocupa 9.4 GB porque es un framework completo. **Pero esta carpeta NO cambia entre versiones de Telegram Desktop**: es la inversión que ya hiciste.

3. **Descubrimiento clave:** ambos repos (`tdesktop` y `tdesktopver01`) apuntan a la MISMA carpeta externa:
   ```
   QT_DIR = C:/Users/VICTOR/code/Libraries/win64/Qt-5.15.19/...
   ```
   Es decir, **el sistema de "compiler tool" ya funciona así de fábrica**: el diseño oficial de tdesktop separa el código fuente (repo, ligero) de las librerías compiladas (`Libraries\`, pesada pero única). Una nueva versión del código NO requiere re-descargar ni recompilar las librerías.

---

## 2. Clasificación: qué conservar, qué borrar, qué empaquetar

### 🔴 Borrar ya (recuperas ~64 GB sin riesgo)
```
C:\Users\VICTOR\code\tdesktopver01\out\
```
La versión 1 ya sirvió como referencia. Su build de 64 GB no se necesita. Si algún día quieres recompilar ver01, se regenera solo (tarda horas, pero es automático).

*Opcional:* si ya no necesitas para nada ver01 (el árbol limpio ya tiene todo portado), puedes archivar/borrar el repo entero y recuperas otros ~950 MB (fuentes + git).

### 🟡 Borrar entre sesiones (recuperas 38 GB cuando quieras)
```
C:\Users\VICTOR\code\tdesktop\out\
```
Bórralo solo si no vas a compilar en días/semanas. Al volver a compilar, CMake re-genera todo (la primera vez tarda mucho, luego incremental rápido). Alternativa menos drástica: borrar solo `out\Telegram\Telegram.dir` (23 GB de objetos intermedios) mantiene la configuración lista.

### 🟢 Conservar SIEMPRE — esto ES tu "compiler tool" (~22 GB)
```
C:\Users\VICTOR\code\Libraries\     ← dependencias compiladas (21 GB)
C:\Users\VICTOR\code\ThirdParty\    ← herramientas auxiliares (0.5 GB)
```
Con estas dos carpetas + Visual Studio instalado, puedes compilar **cualquier versión nueva de Telegram Desktop sin descargar nada más**.

---

## 3. Empaquetado del compiler tool (para PC nuevo o respaldo)

### Opción A — Backup comprimido (recomendado)
Comprimir `Libraries\` + `ThirdParty\` a un archivo 7z:
- Tamaño estimado comprimido: **~10–13 GB** (las librerías .lib/.dll comprimen bien)
- Guardar en disco externo o nube
- En un PC nuevo: extraer a `C:\Users\VICTOR\code\Libraries` y listo

Comando PowerShell (desde Windows):
```powershell
& "C:\Program Files\7-Zip\7z.exe" a -mx=9 -mmt=on `
  "D:\backup\tg-toolchain-libraries-win64.7z" `
  "C:\Users\VICTOR\code\Libraries" "C:\Users\VICTOR\code\ThirdParty"
```

### Opción B — Carpeta compartida tal cual (sin comprimir)
Copiar las carpetas `Libraries\` y `ThirdParty\` intactas a otro disco. Más simple, 22 GB tal cual.

### Qué NO hace falta empaquetar
- El código fuente: se clona de GitHub en minutos (~300 MB con historial)
- Las carpetas `out\`: se generan solas al compilar

---

## 4. Flujo de trabajo para una versión NUEVA (sin re-descargar nada)

Cuando salga una nueva versión de Telegram Desktop y quieras aplicar tus mejoras:

1. **Clonar solo el código nuevo** (~300 MB, minutos):
   ```
   git clone --recursive https://github.com/telegramdesktop/tdesktop.git C:\Users\VICTOR\code\tdesktop-new
   ```

2. **Aplicar tus cambios** (los que ya están documentados en `mejora-limpio.md` — o hacer cherry-pick desde tu fork).

3. **Configurar apuntando a las librerías existentes** — los scripts oficiales ya buscan `..\Libraries` automáticamente:
   ```powershell
   cd C:\Users\VICTOR\code\tdesktop-new\Telegram
   .\configure.bat -D TDESKTOP_API_ID=TU_ID -D TDESKTOP_API_HASH=TU_HASH
   ```
   No se descarga NADA de internet: Qt, ffmpeg, openssl, WebRTC etc. ya están en `code\Libraries`.

4. **Compilar** (primera vez lenta, luego incremental):
   ```powershell
   cmake --build ..\out --config Release
   ```

**Resultado:** una versión nueva cuesta ~300 MB de descarga + tiempo de compilación. Cero gigabytes de librerías.

⚠️ Única condición: que la nueva versión no exija una versión mayor de alguna librería (p. ej. Qt 6). En ese caso habría que añadir esa librería puntualmente a `Libraries\` — las demás siguen sirviendo.

---

## 5. Resumen ejecutivo

| Acción | Espacio recuperado | Riesgo |
|---|---|---|
| Borrar `tdesktopver01\out\` | −64 GB inmediatos | Ninguno (regenerable) |
| Borrar `tdesktop\out\` cuando no compiles | −38 GB temporales | Solo tiempo de recompilación |
| Conservar `Libraries\` + `ThirdParty\` | — (son el tool) | No tocar |
| Empaquetar toolchain a 7z | Backup portable ~11 GB | Ninguno |

**Estado final tras limpieza:** carpeta `code` pasa de ~126 GB a ~23 GB, manteniendo TODO lo necesario para compilar cualquier versión futura directamente.
