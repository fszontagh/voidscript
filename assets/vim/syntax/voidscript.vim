" Vim syntax file for VoidScript
" Save as ~/.vim/syntax/voidscript.vim

if exists("b:current_syntax")
  finish
endif


" --- Keywords ---
syntax keyword voidscriptKeyword const function return for if else new sizeof this

" --- Access Modifiers ---
syntax keyword voidscriptAccess public private protected

" --- Types ---
syntax keyword voidscriptType int string float double boolean object class void

" --- Control values ---
syntax keyword voidscriptBoolean true false null

" --- Variable Names ---
syntax match voidscriptVariable /\$\k\+/

" --- Class definitions ---
syntax match voidscriptClass /\<class\s\+\k\+\>/

" --- Object property access (this->something or object->key) ---
syntax match voidscriptObjectAccess /\<this\>\|->/

" --- Function calls (e.g. func(...)) ---
syntax match voidscriptFunction /\<\k\+\s*(/ containedin=ALL

" --- Numbers ---
syntax match voidscriptNumber /\<\d\+\(\.\d\+\)\?\>/

" --- Strings with variable highlighting ---
syntax region voidscriptString start=/"/ skip=/\\"/ end=/"/ contains=voidscriptVariable

" --- Object literal keys (key: value) ---
syntax match voidscriptObjectKey /\<\k\+\>\s*:/

" Comments
syntax match voidscriptComment "#.*"
syntax match voidscriptComment "\/\/.*" contains=voidscriptTodo
highlight link voidscriptComment Comment

" Optional: highlight TODO, FIXME, NOTE in comments
syntax match voidscriptTodo "\(TODO\|FIXME\|NOTE\):" contained
highlight link voidscriptTodo Todo


" --- Operators ---
syntax match voidscriptOperator /==\|!=\|<=\|>=\|[-+*/%<>=!&|]/

" --- Braces & Delimiters ---
syntax match voidscriptBraces /[{}[\]()]/
syntax match voidscriptArrow /->/

" --- Highlight groups ---
highlight link voidscriptKeyword Keyword
highlight link voidscriptAccess StorageClass
highlight link voidscriptType Type
highlight link voidscriptBoolean Boolean
highlight link voidscriptVariable Identifier
highlight link voidscriptFunction Function
highlight link voidscriptNumber Number
highlight link voidscriptString String
highlight link voidscriptComment Comment
highlight link voidscriptTodo Todo
highlight link voidscriptOperator Operator
highlight link voidscriptBraces Delimiter
highlight link voidscriptArrow Operator
highlight link voidscriptClass Structure
highlight link voidscriptObjectAccess Operator
highlight link voidscriptObjectKey Identifier

let b:current_syntax = "voidscript"
