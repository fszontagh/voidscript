" Advanced VoidScript indent
if exists("b:did_indent")
  finish
endif
let b:did_indent = 1

setlocal indentexpr=GetVoidScriptIndent()
setlocal indentkeys=o,O,0},0),=else,=elif

function! GetVoidScriptIndent()
  let lnum = prevnonblank(v:lnum - 1)
  if lnum == 0
    return 0
  endif

  let prevline = getline(lnum)
  let currline = getline(v:lnum)

  " Strip comments
  let prev = substitute(prevline, '#.*$', '', '')
  let prev = substitute(prev, '//.*$', '', '')
  let curr = substitute(currline, '#.*$', '', '')
  let curr = substitute(curr, '//.*$', '', '')

  let indent = indent(lnum)

  " De-indent for lines that start with } or )
  if curr =~ '^\s*[\]\)}]'
    return indent - &shiftwidth
  endif

  " De-indent for else or elif
  if curr =~ '^\s*\(else\|elif\)\>'
    return indent - &shiftwidth
  endif

  " Object inline literals: ignore increase if likely inline
  if prev =~ '^\s*object\s\+\$\?\w\+\s*=\s*{'
    return indent
  endif

  " Increase indent after these block starters
  if prev =~ '\v<((if|else|elif|for|while|function|class|object)\b.*)?{[ \t]*$'
    return indent + &shiftwidth
  endif

  " Increase indent for lines ending with just a {
  if prev =~ '{\s*$'
    return indent + &shiftwidth
  endif

  " Keep same indent
  return indent
endfunction
