@echo off

scons "platform=windows" "module_mono_enabled=yes" "arch=x86_64" "precision=double" "dev_build=yes" "use_llvm=yes" "use_mingw=yes" "separate_debug_symbols=yes" "-j30"

.\bin\godot.windows.editor.dev.double.x86_64.llvm.mono "--headless" "--generate-mono-glue" "modules\mono\glue" "--precision=double"
::python "%CD%\modules\mono\build_scripts\build_assemblies.py" "--godot-output-dir" "%CD%\bin" "--push-nupkgs-local" "%CD%\..\GodotNugetSourceData" "--precision=double"
python "%CD%\modules\mono\build_scripts\build_assemblies.py" "--godot-output-dir" "%CD%\bin" "--godot-platform=windows" "--push-nupkgs-local" "%CD%\..\GodotNugetSourceData" "--precision=double"
scons "platform=windows" "module_mono_enabled=yes" "arch=x86_64" "precision=double" "dev_build=yes" "use_llvm=yes" "use_mingw=yes" "separate_debug_symbols=yes" "-j30"
::COPY  "%CD%\..\GodotNugetSourceData" "%APPDATA%\NuGet\GodotNugetSource"
::COPY  "%CD%\..\GodotNugetSourceData" "%APPDATA%\NuGet\LocalNugetSource"


::IF [[ "-n" "%TEMPLATES%" "]]" (
::  scons "target=template_release" "tools=no" "module_mono_enabled=yes" "precision=double" "-j8"
::  scons "target=template_debug" "tools=no" "module_mono_enabled=yes" "precision=double" "-j8"
::)