import * as vscode from 'vscode';

// ---------------------------------------------------------------------------
// Language data - kept in one place so highlighting, completion and hover stay
// in sync with the interpreter. Names verified against the VoidScript build.
// ---------------------------------------------------------------------------

const KEYWORDS = [
  'if', 'else', 'for', 'while', 'switch', 'case', 'default', 'break', 'continue',
  'return', 'try', 'catch', 'throw', 'function', 'class', 'enum', 'new', 'include',
  'private', 'public', 'protected', 'const', 'this', 'true', 'false', 'null'
];

const TYPES = ['int', 'double', 'float', 'string', 'boolean', 'bool', 'object', 'auto'];

// Built-in / dynamic-module global functions. `signature` drives the completion
// placeholder and the hover; `doc` is a one-line description.
interface BuiltinDoc {
  signature: string;
  doc: string;
}

const BUILTINS: Record<string, BuiltinDoc> = {
  print: { signature: 'print(...args)', doc: 'Print without a trailing newline.' },
  printnl: { signature: 'printnl(...args)', doc: 'Print the comma-separated args and a newline.' },
  error: { signature: 'error(...args)', doc: 'Print to standard error.' },
  exit: { signature: 'exit(int code)', doc: 'Terminate the program with an exit code.' },
  format: { signature: 'format(string fmt, ...args) string', doc: 'Format with {} placeholders.' },
  format_print: { signature: 'format_print(string fmt, ...args)', doc: 'Format and print in one call.' },
  sizeof: { signature: 'sizeof(arrayOrObject) int', doc: 'Number of elements / keys.' },
  typeof: { signature: 'typeof(value) string', doc: 'Runtime type name of a value.' },
  isset: { signature: 'isset(value) boolean', doc: 'Whether a value is set.' },
  is_null: { signature: 'is_null(value) boolean', doc: 'Whether a value is null.' },
  var_dump: { signature: 'var_dump(value)', doc: 'Debug-print a value with its structure.' },
  call_user_func: { signature: 'call_user_func(string fn, ...args)', doc: 'Call a function by name (callbacks / dynamic dispatch).' },

  number_to_string: { signature: 'number_to_string(number) string', doc: 'Convert a number to its string form.' },
  string_to_number: { signature: 'string_to_number(string s)', doc: 'Parse a number (int for "12", float for "1.5").' },
  current_unix_timestamp: { signature: 'current_unix_timestamp() int', doc: 'Seconds since the Unix epoch.' },
  date: { signature: 'date(string fmt) string', doc: 'Format the current date/time.' },

  json_encode: { signature: 'json_encode(value) string', doc: 'Serialize a value to JSON.' },
  json_decode: { signature: 'json_decode(string s)', doc: 'Parse JSON into a value.' },

  file_get_contents: { signature: 'file_get_contents(string path) string', doc: 'Read a file into a string.' },
  file_put_contents: { signature: 'file_put_contents(string path, string data, boolean overwrite)', doc: 'Write a string to a file.' },
  file_exists: { signature: 'file_exists(string path) boolean', doc: 'Whether a path exists.' },
  file_unlink: { signature: 'file_unlink(string path)', doc: 'Delete a file.' },
  file_size: { signature: 'file_size(string path) int', doc: 'File size in bytes.' },
  file_copy: { signature: 'file_copy(string from, string to)', doc: 'Copy a file.' },
  file_rename: { signature: 'file_rename(string from, string to)', doc: 'Rename / move a file.' },
  mkdir: { signature: 'mkdir(string path)', doc: 'Create a directory.' },
  rmdir: { signature: 'rmdir(string path)', doc: 'Remove a directory.' },

  path_join: { signature: 'path_join(...parts) string', doc: 'Join path segments.' },
  path_basename: { signature: 'path_basename(string path) string', doc: 'Final path component.' },
  path_dirname: { signature: 'path_dirname(string path) string', doc: 'Parent directory.' },
  path_extension: { signature: 'path_extension(string path) string', doc: 'File extension.' },
  path_exists: { signature: 'path_exists(string path) boolean', doc: 'Whether a path exists.' },
  path_glob: { signature: 'path_glob(string pattern)', doc: 'Glob for matching paths.' },

  env_get: { signature: 'env_get(string name) string', doc: 'Read an environment variable.' },
  env_set: { signature: 'env_set(string name, string value)', doc: 'Set an environment variable.' },
  env_has: { signature: 'env_has(string name) boolean', doc: 'Whether an env var is set.' },

  process_run: { signature: 'process_run(string prog, array argv) object', doc: 'Run a program (no shell); returns { exit_code, stdout, stderr }.' },
  process_spawn: { signature: 'process_spawn(string prog, array argv) object', doc: 'Spawn a program for streaming I/O.' },

  curlGet: { signature: 'curlGet(string url) string', doc: 'HTTP GET.' },
  curlPost: { signature: 'curlPost(string url, string body) string', doc: 'HTTP POST.' },
  curlPut: { signature: 'curlPut(string url, string body) string', doc: 'HTTP PUT.' },
  curlDelete: { signature: 'curlDelete(string url) string', doc: 'HTTP DELETE.' },

  hash_string: { signature: 'hash_string(string algo, string s) string', doc: 'Hash a string, e.g. hash_string("sha256", s).' },
  hash_file: { signature: 'hash_file(string algo, string path) string', doc: 'Hash a file.' },
  base64_encode: { signature: 'base64_encode(string s) string', doc: 'Base64-encode.' },
  base64_decode: { signature: 'base64_decode(string s) string', doc: 'Base64-decode.' },
  sleep: { signature: 'sleep(int seconds)', doc: 'Sleep for whole seconds.' },
  usleep: { signature: 'usleep(int microseconds)', doc: 'Sleep for microseconds.' },

  string_length: { signature: 'string_length(string s) int', doc: 'Length of a string.' },
  string_substr: { signature: 'string_substr(string s, int start, int len) string', doc: 'Substring.' },
  string_replace: { signature: 'string_replace(string s, string from, string to) string', doc: 'Replace occurrences.' },
  string_split: { signature: 'string_split(string s, string sep)', doc: 'Split into an array.' },
  string_join: { signature: 'string_join(array parts, string sep) string', doc: 'Join with a separator.' },
  string_trim: { signature: 'string_trim(string s) string', doc: 'Trim whitespace.' },
  string_to_upper: { signature: 'string_to_upper(string s) string', doc: 'Uppercase.' },
  string_to_lower: { signature: 'string_to_lower(string s) string', doc: 'Lowercase.' },
  string_contains: { signature: 'string_contains(string s, string sub) boolean', doc: 'Substring test.' },
  string_starts_with: { signature: 'string_starts_with(string s, string p) boolean', doc: 'Prefix test.' },
  string_ends_with: { signature: 'string_ends_with(string s, string p) boolean', doc: 'Suffix test.' },
  string_index_of: { signature: 'string_index_of(string s, string sub) int', doc: 'Index of a substring (-1 if absent).' },
  string_repeat: { signature: 'string_repeat(string s, int n) string', doc: 'Repeat a string.' },
  string_reverse: { signature: 'string_reverse(string s) string', doc: 'Reverse a string.' },

  abs: { signature: 'abs(number) number', doc: 'Absolute value.' },
  ceil: { signature: 'ceil(number) number', doc: 'Round up.' },
  floor: { signature: 'floor(number) number', doc: 'Round down.' },
  round: { signature: 'round(number) number', doc: 'Round to nearest.' },
  sqrt: { signature: 'sqrt(number) number', doc: 'Square root.' },
  pow: { signature: 'pow(number base, number exp) number', doc: 'Power.' },
  min: { signature: 'min(a, b) number', doc: 'Smaller of two.' },
  max: { signature: 'max(a, b) number', doc: 'Larger of two.' },

  readline: { signature: 'readline(string prompt) string', doc: 'Read a line from stdin.' },
  module_exists: { signature: 'module_exists(string name) boolean', doc: 'Whether a module is available.' },
  function_exists: { signature: 'function_exists(string name) boolean', doc: 'Whether a function is defined.' },
  class_exists: { signature: 'class_exists(string name) boolean', doc: 'Whether a class is defined.' }
};

// Names that exist in the runtime but do not need a hand-written doc; still offered
// for completion so nothing common is missing.
const EXTRA_BUILTINS = [
  'archive_create', 'archive_extract', 'archive_list', 'hash_compare', 'cos', 'sin', 'tan',
  'log', 'log10', 'PI', 'header', 'getline', 'readchar', 'object_set', 'throw_error',
  'env_all', 'env_apply', 'env_unset', 'file_chmod', 'file_stat', 'file_mtime',
  'file_set_mtime', 'file_symlink', 'file_rm_rf', 'path_canonical', 'path_relative',
  'path_is_dir', 'path_is_file', 'path_is_symlink', 'list_modules', 'list_module_functions',
  'list_module_classes', 'list_class_methods', 'method_exists', 'process_check',
  'process_kill', 'process_wait', 'process_read_stdout_line', 'process_write_stdin'
];

// ---------------------------------------------------------------------------
// Activation
// ---------------------------------------------------------------------------

export function activate(context: vscode.ExtensionContext) {
  context.subscriptions.push(
    vscode.languages.registerDocumentFormattingEditProvider('voidscript', {
      provideDocumentFormattingEdits(document: vscode.TextDocument): vscode.TextEdit[] {
        const fullText = document.getText();
        const fullRange = new vscode.Range(
          document.positionAt(0),
          document.positionAt(fullText.length)
        );
        return [vscode.TextEdit.replace(fullRange, formatVoidScript(fullText))];
      }
    })
  );

  context.subscriptions.push(
    vscode.languages.registerCompletionItemProvider('voidscript', {
      provideCompletionItems(): vscode.CompletionItem[] {
        const items: vscode.CompletionItem[] = [];

        for (const kw of KEYWORDS) {
          items.push(new vscode.CompletionItem(kw, vscode.CompletionItemKind.Keyword));
        }
        for (const t of TYPES) {
          const it = new vscode.CompletionItem(t, vscode.CompletionItemKind.TypeParameter);
          it.detail = 'type';
          items.push(it);
        }
        for (const [name, info] of Object.entries(BUILTINS)) {
          const it = new vscode.CompletionItem(name, vscode.CompletionItemKind.Function);
          it.detail = info.signature;
          it.documentation = new vscode.MarkdownString(info.doc);
          it.insertText = new vscode.SnippetString(`${name}($0)`);
          items.push(it);
        }
        for (const name of EXTRA_BUILTINS) {
          const it = new vscode.CompletionItem(name, vscode.CompletionItemKind.Function);
          it.insertText = new vscode.SnippetString(`${name}($0)`);
          items.push(it);
        }
        return items;
      }
    })
  );

  context.subscriptions.push(
    vscode.languages.registerHoverProvider('voidscript', {
      provideHover(document: vscode.TextDocument, position: vscode.Position): vscode.Hover | undefined {
        const range = document.getWordRangeAtPosition(position, /[A-Za-z_][A-Za-z0-9_]*/);
        if (!range) {
          return undefined;
        }
        const word = document.getText(range);
        const info = BUILTINS[word];
        if (info) {
          const md = new vscode.MarkdownString();
          md.appendCodeblock(info.signature, 'voidscript');
          md.appendMarkdown(info.doc);
          return new vscode.Hover(md, range);
        }
        if (TYPES.includes(word)) {
          return new vscode.Hover(new vscode.MarkdownString(`\`${word}\` - VoidScript type`), range);
        }
        return undefined;
      }
    })
  );
}

export function deactivate() { /* nothing to clean up */ }

// ---------------------------------------------------------------------------
// Formatter - brace-aware, re-indents by net `{` / `}` depth. Braces inside
// strings and line comments (// or #) are ignored, and a line that opens and
// closes on itself (`} else {`, inline object literals) is handled correctly.
// ---------------------------------------------------------------------------

export function formatVoidScript(text: string): string {
  const lines = text.split(/\r?\n/);
  const out: string[] = [];
  const indentUnit = '    ';
  let depth = 0;

  for (const raw of lines) {
    const line = raw.trim();
    if (line === '') {
      out.push('');
      continue;
    }

    const code = codeOnly(line);
    const opens = (code.match(/\{/g) || []).length;
    const closes = (code.match(/\}/g) || []).length;
    const leadingCloses = countLeadingCloses(code);

    const printDepth = Math.max(0, depth - leadingCloses);
    out.push(indentUnit.repeat(printDepth) + line);

    depth = Math.max(0, depth + opens - closes);
  }

  return out.join('\n');
}

// Blank out string bodies and trailing comments so brace counting only sees code.
function codeOnly(line: string): string {
  let result = '';
  let i = 0;
  while (i < line.length) {
    const c = line[i];
    if (c === '/' && line[i + 1] === '/') {
      break;
    }
    if (c === '#') {
      break;
    }
    if (c === '"' || c === "'") {
      const quote = c;
      i++;
      while (i < line.length) {
        if (line[i] === '\\') { i += 2; continue; }
        if (line[i] === quote) { i++; break; }
        i++;
      }
      result += ' ';
      continue;
    }
    result += c;
    i++;
  }
  return result;
}

// Count consecutive `}` (whitespace allowed between) at the start of a line.
function countLeadingCloses(code: string): number {
  let n = 0;
  for (const ch of code.replace(/^\s+/, '')) {
    if (ch === '}') { n++; }
    else if (/\s/.test(ch)) { continue; }
    else { break; }
  }
  return n;
}
