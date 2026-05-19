@echo off
set DEPS_ROOT=C:\dev\barony-deps
set DATADIR=C:\Program Files (x86)\Steam\steamapps\common\Barony
set PATH=%PATH%;%DEPS_ROOT%\bin

zig build run %* -Ddeps_root="%DEPS_ROOT%" -Ddatadir="%DATADIR%" -Doptimize=ReleaseFast
