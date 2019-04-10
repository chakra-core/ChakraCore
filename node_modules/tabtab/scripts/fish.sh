###-begin-{pkgname}-completion-###
function _{pkgname}_completion
  set cmd (commandline -opc)
  set cursor (commandline -C)
  set completions (eval env DEBUG=\"" \"" COMP_CWORD=\""$cmd\"" COMP_LINE=\""$cmd \"" COMP_POINT=\""$cursor\"" {completer} completion -- $cmd)

  for completion in $completions
    echo -e $completion
  end
end

complete -f -d '{pkgname}' -c {pkgname} -a "(eval _{pkgname}_completion)"
###-end-{pkgname}-completion-###
