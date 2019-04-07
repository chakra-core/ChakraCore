###-begin-gitbook-completion-###
if type compdef &>/dev/null; then
  _gitbook_completion () {
    local reply
    local si=$IFS

    IFS=$'\n' reply=($(COMP_CWORD="$((CURRENT-1))" COMP_LINE="$BUFFER" COMP_POINT="$CURSOR" gitbook-completions completion -- "${words[@]}"))
    IFS=$si

    _describe 'values' reply
  }
  compdef _gitbook_completion gitbook
fi
###-end-gitbook-completion-###
