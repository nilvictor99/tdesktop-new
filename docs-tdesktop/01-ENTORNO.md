# Entorno — toolchain, disco y compilación

Fecha análisis: 22 agosto 2026. Carpeta `C:\Users\VICTOR\code` (~126 GB).

## Qué pesar en `code\` y qué conservar

| Carpeta | Tamaño | Qué es | ¿Borrar? |
|---|---|---|---|
| `tdesktopver01\out\` | 64 GB | Artefactos de compilación de ver01 (obsolete) | ✅ SÍ, regenerable |
| `tdesktop\out\` | 38 GB | Artefactos de la build actual | ⚠️ Sí, solo cuando no compilas (se regeneran) |
| `Libraries\win64\` | 21 GB | Dependencias YA compiladas (Qt 9.4 GB, tg_owt/WebRTC, ffmpeg, openssl…) | ❌ NO — es la herramienta de trabajo |
| `ThirdParty\` | 557 MB | Herramientas auxiliares (gyp…) | ❌ NO |
| Fuentes (`Telegram\`) | ~430 MB | Código fuente | ❌ Esencial |

Clave: ambos repos apuntan a la MISMA librerías
(`QT_DIR = C:/Users/VICTOR/code/Libraries/win64/Qt-5.15.19/...`). El diseño
oficial de tdesktop ya separa código (repo, ligero) de librerías compiladas
(`Libraries`, pesada pero única). Una versión nueva del código NO requiere
re-descargar ni recompilar las librerías.

## Limpieza recomendada

- Borrar ya: `tdesktopver01\out\` (−64 GB, sin riesgo; regenerable).
- Entre sesiones: `tdesktop\out\` (−38 GB). Si no compilas en días, borrar solo
  `out\Telegram\Telegram.dir` (23 GB de intermedios) mantiene la config lista.
- Conservar siempre: `Libraries\` + `ThirdParty\` (~22 GB) = tu "compiler tool".

## Backup portable (PC nuevo o respaldo)

```powershell
& "C:\Program Files\7-Zip\7z.exe" a -mx=9 -mmt=on `
  "D:\backup\tg-toolchain-libraries-win64.7z" `
  "C:\Users\VICTOR\code\Libraries" "C:\Users\VICTOR\code\ThirdParty"
```

Comprimido ≈ 10–13 GB. No hace falta empaquetar fuentes (se clonan) ni `out\`
(se regenera).

## Versión NUEVA de tdesktop sin descargar librerías

1. `git clone --recursive https://github.com/telegramdesktop/tdesktop.git code\tdesktop-new`
2. Aplicar los cambios documentados en `docs`. Reusar el toolchain: el script ya
   busca `..\Libraries` automáticamente.
3. `cd tdesktop-new\Telegram` -> `.\configure.bat -D TDESKTOP_API_ID=TU_ID -D TDESKTOP_API_HASH=TU_HASH`
4. `cmake --build ..\out --config Release`

Costo total de una versión nueva: ~300 MB de descarga + tiempo de compilación.

⚠️ Condición única: que la versión nueva no exija una librería mayor (p. ej.
Qt 6); en ese caso solo habría que añadir esa librería, no las demás.

## Compilar esta build

```powershell
msbuild Telegram.sln /p:Configuration=Release /p:Platform=x64 /m /t:Telegram
```

Si el compilador agota memoria (C1060/C3859/C1076 en archivos generados o SDK,
no en nuestro código): bajar el paralelismo a `/m:4` o `/m:2` y liberar RAM.