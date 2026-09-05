# [Telegram Desktop][telegram_desktop] – Official Messenger

This is the complete source code and the build instructions for the official [Telegram][telegram] messenger desktop client, based on the [Telegram API][telegram_api] and the [MTProto][telegram_proto] secure protocol.

[![Version](https://badge.fury.io/gh/telegramdesktop%2Ftdesktop.svg)](https://github.com/telegramdesktop/tdesktop/releases)
[![Build Status](https://github.com/telegramdesktop/tdesktop/workflows/Windows./badge.svg)](https://github.com/telegramdesktop/tdesktop/actions)
[![Build Status](https://github.com/telegramdesktop/tdesktop/workflows/MacOS./badge.svg)](https://github.com/telegramdesktop/tdesktop/actions)
[![Build Status](https://github.com/telegramdesktop/tdesktop/workflows/Linux./badge.svg)](https://github.com/telegramdesktop/tdesktop/actions)
[![Built with Depot](https://img.shields.io/badge/Built%20with-Depot.dev-46A75A)](https://depot.dev)

[![Preview of Telegram Desktop][preview_image]][preview_image_url]

The source code is published under GPLv3 with OpenSSL exception, the license is available [here][license].

> **Build personalizada (fork).** Este árbol contiene modificaciones propias además
> de la base oficial. Las funciones añadidas están resumidas abajo; el detalle
> técnico y el seguimiento de cada ronda viven en `C:\Users\VICTOR\code\docs`
> (`cambios.md`, `seguimiento.md`, `README.md`).

## Funcionalidades implementadas en esta build

Todo lo que hace distinta esta build frente al Telegram Desktop original.

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

### Estabilidad de carga
- Ventana de carga grande (≈2000 ids) en "Todo" y "Fotos + Archivos", mantenida al hacer scroll.
- Materialización proactiva de todo el contenido, con paralelismo acotado (3 peticiones en vuelo) para no congelar la app.
- Fin de lista honesto (sin "tope fantasma" que cortase la lista antes de tiempo).
- Anti-bucle de reintentos y auto-reparación de búsquedas fallidas (PEER_ID_INVALID / red).
- Depurador propio: archivo `depurador.txt` con diagnóstico de la carga de FilesPhotos/All.

### Correcciones de crash destacadas
- "Seleccionar todo" en Fotos + Archivos (geometría mixta foto/documento en la cuadrícula).
- Arranque del módulo Fotos + Archivos (caso `Type::FilesPhotos` en la barra superior).
- Cierre inesperado al reenviar varios archivos a la vez: el callable recursivo de la cadena de reenvío se auto-capturaba por valor (versión vacía) → `std::bad_function_call` → `abort`. Ahora se guarda en `shared_ptr<std::function>` y solo se invoca si existe; los lotes con peer origen vacío se saltan (evita `INPUT_PEERS_EMPTY`).

## Supported systems

The latest version is available for

* [Windows 7 and above (64 bit)](https://telegram.org/dl/desktop/win64) ([portable](https://telegram.org/dl/desktop/win64_portable))
* [Windows 7 and above (32 bit)](https://telegram.org/dl/desktop/win) ([portable](https://telegram.org/dl/desktop/win_portable))
* [macOS 10.13 and above](https://telegram.org/dl/desktop/mac)
* [Linux static build for 64 bit](https://telegram.org/dl/desktop/linux)
* [Snap](https://snapcraft.io/telegram-desktop)
* [Flatpak](https://flathub.org/apps/details/org.telegram.desktop)

## Old system versions

Version **4.9.9** was the last that supports older systems

* [macOS 10.12](https://updates.tdesktop.com/tmac/tsetup.4.9.9.dmg)
* [Linux with glibc < 2.28 static build](https://updates.tdesktop.com/tlinux/tsetup.4.9.9.tar.xz)

Version **2.4.4** was the last that supports older systems

* [OS X 10.10 and 10.11](https://updates.tdesktop.com/tosx/tsetup-osx.2.4.4.dmg)
* [Linux static build for 32 bit](https://updates.tdesktop.com/tlinux32/tsetup32.2.4.4.tar.xz)

Version **1.8.15** was the last that supports older systems

* [Windows XP and Vista](https://updates.tdesktop.com/tsetup/tsetup.1.8.15.exe) ([portable](https://updates.tdesktop.com/tsetup/tportable.1.8.15.zip))
* [OS X 10.8 and 10.9](https://updates.tdesktop.com/tmac/tsetup.1.8.15.dmg)
* [OS X 10.6 and 10.7](https://updates.tdesktop.com/tmac32/tsetup32.1.8.15.dmg)

## Third-party

* Qt 6 ([LGPL](http://doc.qt.io/qt-6/lgpl.html)) and Qt 5.15 ([LGPL](http://doc.qt.io/qt-5/lgpl.html)) slightly patched
* OpenSSL 3.2.1 ([Apache License 2.0](https://openssl-library.org/source/license/apache-license-2.0.txt))
* WebRTC ([New BSD License](https://github.com/desktop-app/tg_owt/blob/master/LICENSE))
* zlib ([zlib License](http://www.zlib.net/zlib_license.html))
* LZMA SDK 9.20 ([public domain](http://www.7-zip.org/sdk.html))
* liblzma ([public domain](http://tukaani.org/xz/))
* Google Breakpad ([License](https://chromium.googlesource.com/breakpad/breakpad/+/master/LICENSE))
* Google Crashpad ([Apache License 2.0](https://chromium.googlesource.com/crashpad/crashpad/+/master/LICENSE))
* GYP ([BSD License](https://github.com/bnoordhuis/gyp/blob/master/LICENSE))
* Ninja ([Apache License 2.0](https://github.com/ninja-build/ninja/blob/master/COPYING))
* OpenAL Soft ([LGPL](https://github.com/kcat/openal-soft/blob/master/COPYING))
* Opus codec ([BSD License](http://www.opus-codec.org/license/))
* FFmpeg ([LGPL](https://www.ffmpeg.org/legal.html))
* Guideline Support Library ([MIT License](https://github.com/Microsoft/GSL/blob/master/LICENSE))
* Range-v3 ([Boost License](https://github.com/ericniebler/range-v3/blob/master/LICENSE.txt))
* Open Sans font ([Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0.html))
* Vazirmatn font ([SIL Open Font License 1.1](https://github.com/rastikerdar/vazirmatn/blob/master/OFL.txt))
* Emoji alpha codes ([MIT License](https://github.com/emojione/emojione/blob/master/extras/alpha-codes/LICENSE.md))
* xxHash ([BSD License](https://github.com/Cyan4973/xxHash/blob/dev/LICENSE))
* QR Code generator ([MIT License](https://github.com/nayuki/QR-Code-generator#license))
* CMake ([New BSD License](https://github.com/Kitware/CMake/blob/master/Copyright.txt))
* Hunspell ([LGPL](https://github.com/hunspell/hunspell/blob/master/COPYING.LESSER))
* Ada ([Apache License 2.0](https://github.com/ada-url/ada/blob/main/LICENSE-APACHE))

## Build instructions

* [Windows (32-bit and 64-bit)][win]
* [macOS][mac]
* [GNU/Linux using Docker][linux]

[//]: # (LINKS)
[telegram]: https://telegram.org
[telegram_desktop]: https://desktop.telegram.org
[telegram_api]: https://core.telegram.org
[telegram_proto]: https://core.telegram.org/mtproto
[license]: LICENSE
[win]: docs/building-win.md
[mac]: docs/building-mac.md
[linux]: docs/building-linux.md
[preview_image]: https://github.com/telegramdesktop/tdesktop/blob/dev/docs/assets/preview.png "Preview of Telegram Desktop"
[preview_image_url]: https://raw.githubusercontent.com/telegramdesktop/tdesktop/dev/docs/assets/preview.png

## Thanks to

<a href="https://depot.dev">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://depot.dev/assets/brand/1693758816/depot-logo-horizontal-on-dark.svg">
    <source media="(prefers-color-scheme: light)" srcset="https://depot.dev/assets/brand/1693758816/depot-logo-horizontal-on-light.svg">
    <img alt="Depot" src="https://depot.dev/assets/brand/1693758816/depot-logo-horizontal-on-light.svg" width="150">
  </picture>
</a>

CI infrastructure sponsored by [Depot](https://depot.dev) — fast GitHub Actions runners.

