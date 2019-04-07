###-begin-gitbook-completion-###
function _gitbook_completion
  set cmd (commandline -opc)
  set cursor (commandline -C)
  set completions (eval env DEBUG=\"" \"" COMP_CWORD=\""$cmd\"" COMP_LINE=\""$cmd \"" COMP_POINT=\""$cursor\"" gitbook-completions completion -- $cmd)

  for completion in $completions
    echo -e $completion
  end
end

complete -f -d 'gitbook' -c gitbook -a "(eval _gitbook_completion)"
###-end-gitbook-completion-###
