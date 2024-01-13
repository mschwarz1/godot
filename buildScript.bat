@echo off

scons "platform=windows" "module_mono_enabled=yes" "precision=double" "dev_build=yes" "verbose=yes" "-j1"
bin/godot.windows.editor.dev.double.x86_64.mono.exe "--generate-mono-glue" "modules\mono\glue" "--precision=double"
python "%CD%\modules\mono\build_scripts\build_assemblies.py" "--godot-output-dir" "%CD%\bin" "--push-nupkgs-local" "%CD%\..\GodotNugetSourceData" "--precision=double"
scons "module_mono_enabled=yes" "precision=double" "-j8"
COPY  "%CD%\..\GodotNugetSourceData" "%APPDATA%\NuGet\GodotNugetSource"
COPY  "%CD%\..\GodotNugetSourceData" "%APPDATA%\NuGet\LocalNugetSource"
IF [[ "-n" "%TEMPLATES%" "]]" (
  scons "target=template_release" "tools=no" "module_mono_enabled=yes" "precision=double" "-j8"
  scons "target=template_debug" "tools=no" "module_mono_enabled=yes" "precision=double" "-j8"
)