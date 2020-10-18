@echo off
FOR /d %%d IN ("_*") DO @IF EXIST "%%d" rd /s /q "%%d"
