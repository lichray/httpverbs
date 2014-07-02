@echo off

for /F %%i in (test_server.pid) do taskkill /PID %%i
del test_server.pid
