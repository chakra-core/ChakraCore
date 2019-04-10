###-begin-{pkgname}-completion-###
function _{pkgname}_completion
  set cmd (commandline -opc)
  set cursor (commandline -C)
  set completions (eval env DEBUG=\"" \"" COMP_CWORD=\""$cmd\"" COMP_LINE=\""$cmd\"" COMP_POINT=\""$cursor\"" {completer} completion --json)

  for completion in $completions
    set cmd "node -e \"var parts = '$completion'.split(':'); console.log(parts.slice(0, -1).join(':')); console.log(parts.slice(-1)[0]);\""
    set parts (eval $cmd)
    complete -f -c {pkgname} -a "'$parts[1]'" -d "$parts[2]"
    echo $parts[1]
  end
end

complete -d '{pkgname}' -c {pkgname} -a "(eval _{pkgname}_completion)"
###-end-{pkgname}-completion-###
