###-begin-{pkgname}-completion-###

# Copyright (c) npm, Inc. and Contributors
# All rights reserved.

# Credits to npm, this file is coming directly from isaacs/npm repo
#
# npm command completion script
#
# Installation: {completer} completion >> ~/.bashrc  (or ~/.zshrc)
# Or, maybe: {completer} completion > /usr/local/etc/bash_completion.d/{completer}
#

if type complete &>/dev/null; then
  _{pkgname}_completion () {
    local words cword
    if type _get_comp_words_by_ref &>/dev/null; then
      _get_comp_words_by_ref -n = -n @ -w words -i cword
    else
      cword="$COMP_CWORD"
      words=("${COMP_WORDS[@]}")
    fi

    local si="$IFS"
    IFS=$'\n' COMPREPLY=($(COMP_CWORD="$cword" \
                           COMP_LINE="$COMP_LINE" \
                           COMP_POINT="$COMP_POINT" \
                           {completer} completion -- "${words[@]}" \
                           2>/dev/null)) || return $?
    IFS="$si"
  }
  complete -o default -F _{pkgname}_completion {pkgname}
elif type compdef &>/dev/null; then
  _{pkgname}_completion() {
    local si=$IFS
    compadd -- $(COMP_CWORD=$((CURRENT-1)) \
                 COMP_LINE=$BUFFER \
                 COMP_POINT=0 \
                 {completer} completion -- "${words[@]}" \
                 2>/dev/null)
    IFS=$si
  }
  compdef _{pkgname}_completion {pkgname}
elif type compctl &>/dev/null; then
  _{pkgname}_completion () {
    local cword line point words si
    read -Ac words
    read -cn cword
    let cword-=1
    read -l line
    read -ln point
    si="$IFS"
    IFS=$'\n' reply=($(COMP_CWORD="$cword" \
                       COMP_LINE="$line" \
                       COMP_POINT="$point" \
                       {completer} completion -- "${words[@]}" \
                       2>/dev/null)) || return $?
    IFS="$si"
  }
  compctl -K _{pkgname}_completion {pkgname}
fi
###-end-{pkgname}-completion-###
