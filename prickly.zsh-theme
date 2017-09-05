# -*- mode: shell-script; -*-

DIR=$(dirname $(readlink -f ${(%):-%x}))

prompt_prickly_main() {
    $DIR/prickly-prompt $?
}

prompt_prickly_precmd() {
  vcs_info
  PROMPT='%{%f%b%k%}$(prompt_prickly_main) '
}

prompt_prickly_setup() {
  autoload -Uz add-zsh-hook
  autoload -Uz vcs_info

  prompt_opts=(cr subst percent)

  add-zsh-hook precmd prompt_prickly_precmd

  zstyle ':vcs_info:*' enable git
  zstyle ':vcs_info:*' check-for-changes false
  zstyle ':vcs_info:git*' formats '%b'
  zstyle ':vcs_info:git*' actionformats '%b (%a)'
}

prompt_prickly_setup $? "$@"
