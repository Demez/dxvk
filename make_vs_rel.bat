@echo off

set "cmd=--backend vs -Dbuildtype=release -Ddebug=false vs"

if exist "vs" (
	meson --wipe %cmd%
) else (
	meson %cmd%
)

