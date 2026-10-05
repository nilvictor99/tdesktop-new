@echo off
setlocal EnableExtensions
cd /d "%~dp0"

rem --- Localizar la carpeta de Release ---
rem     %~dp0 ya termina en barra; se quita para no duplicarla.
set "BASE=%~dp0"
if "%BASE:~-1%"=="\" set "BASE=%BASE:~0,-1%"
set "TARGET=%BASE%\out\Release"
if not exist "%TARGET%\Telegram.exe" set "TARGET=%BASE%"
if not exist "%TARGET%\Telegram.exe" set "TARGET=%BASE%\Release"
if not exist "%TARGET%\Telegram.exe" (
	echo   [ERROR] No se encontro Telegram.exe.
	echo.
	echo   Buscado en:
	echo     %BASE%\out\Release
	echo     %BASE%
	echo     %BASE%\Release
	echo.
	echo   No se borro nada. Deja este .cmd junto a la carpeta
	echo   que contiene Telegram.exe y vuelve a ejecutarlo.
	echo.
	pause
	exit /b 1
)
cd /d "%TARGET%"

echo ============================================================
echo   Limpieza de Release - Telegram Desktop
echo ============================================================
echo.
echo   Carpeta : %CD%
echo.

rem --- 1. La app debe estar cerrada: si corre, tdata se corrompe ---
tasklist /FI "IMAGENAME eq Telegram.exe" 2>nul | find /I "Telegram.exe" >nul
if not errorlevel 1 (
	echo   [FUERA DE SERVICIO] Telegram.exe sigue abierto.
	echo.
	echo   Cierralo por completo y vuelve a ejecutar este archivo.
	echo   No se borro nada.
	echo.
	pause
	exit /b 1
)

rem --- 2. Medir antes ---
powershell -NoProfile -Command "(Get-ChildItem -LiteralPath '.' -Force -Recurse -File -ErrorAction SilentlyContinue | Measure-Object -Property Length -Sum).Sum" > "%TEMP%\lr_antes.txt" 2>nul

rem --- 3. Logs y backup antiguo ---
del /q "depurador.txt" 2>nul
del /q "log.txt" 2>nul
del /q "Telegram_old.exe" 2>nul

rem --- 4. Cache de imagenes: solo los buckets (0, 1, 2...).
rem        Se conservan la carpeta cache\ y su marcador version. ---
for /d %%D in ("tdata\user_data\cache\*") do rd /s /q "%%~fD"

rem --- 5. Cache de emojis: se regenera sola ---
if exist "tdata\emoji" rd /s /q "tdata\emoji"

rem --- 6. El exe debe seguir en pie ---
if not exist "Telegram.exe" (
	echo   [ALERTA] Telegram.exe ya no esta en la carpeta.
	echo   Revisa la carpeta antes de seguir usando la app.
	echo.
	pause
	exit /b 1
)

rem --- 7. Medir despues y mostrar resumen ---
powershell -NoProfile -Command "(Get-ChildItem -LiteralPath '.' -Force -Recurse -File -ErrorAction SilentlyContinue | Measure-Object -Property Length -Sum).Sum" > "%TEMP%\lr_despues.txt" 2>nul

powershell -NoProfile -Command "function G($p){$t=Get-Content -LiteralPath $p -Raw -ErrorAction SilentlyContinue;if($t -and $t.Trim()){[double]$t.Trim()}else{0}}; $a=G '%TEMP%\lr_antes.txt'; $d=G '%TEMP%\lr_despues.txt'; Write-Host ('   Peso antes   : {0:N1} MB' -f ($a/1MB)); Write-Host ('   Peso despues : {0:N1} MB' -f ($d/1MB)); if($a -gt 0){Write-Host ('   Liberados    : {0:N1} MB' -f (($a-$d)/1MB)); Write-Host ('   Reducido en  : {0:N1} pct' -f ((($a-$d)/$a)*100))}"
del /q "%TEMP%\lr_antes.txt" 2>nul
del /q "%TEMP%\lr_despues.txt" 2>nul

echo.
echo   Conservado : Telegram.exe, modules\, Telegram.pdb,
echo                tu sesion en tdata\ y la configuracion.
echo   Nota       : el .pdb se mantiene a proposito, por eso la
echo                carpeta sigue pesando bastante.
echo.
pause
exit /b 0