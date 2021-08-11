@echo off

set "cmd=--backend vs vs"

if exist "vs" (
	meson --wipe %cmd%
) else (
	meson %cmd%
)