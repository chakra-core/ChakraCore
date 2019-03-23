@IF EXIST "%~dp0\node.exe" (
  "%~dp0\node.exe"  "%~dp0\..\handlebars\node_modules\uglify-js\bin\uglifyjs" %*
) ELSE (
  @SETLOCAL
  @SET PATHEXT=%PATHEXT:;.JS;=;%
  node  "%~dp0\..\handlebars\node_modules\uglify-js\bin\uglifyjs" %*
)