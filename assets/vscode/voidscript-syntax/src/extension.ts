import * as vscode from 'vscode';

export function activate(context: vscode.ExtensionContext) {
  context.subscriptions.push(
    vscode.languages.registerDocumentFormattingEditProvider('voidscript', {
      provideDocumentFormattingEdits(
        document: vscode.TextDocument
      ): vscode.TextEdit[] {
        const fullText = document.getText();
        const first = document.positionAt(0);
        const last = document.positionAt(fullText.length);
        const fullRange = new vscode.Range(first, last);
        const formatted = formatVoidScript(fullText);
        return [vscode.TextEdit.replace(fullRange, formatted)];
      }
    })
  );
}

export function deactivate() { }

function formatVoidScript(text: string): string {
  const lines = text.split(/\r?\n/);
  let indentLevel = 0;
  const indentSize = 4;
  const formattedLines: string[] = [];

  const increaseIndentNextLine = (line: string): boolean => {
    return (
      /{\s*$/.test(line) || // block start
      /:\s*{/.test(line)    // inline object key: {
    );
  };

  const decreaseIndentCurrentLine = (line: string): boolean => {
    return /^\s*}/.test(line);
  };

  for (let line of lines) {
    const trimmed = line.trim();

    if (trimmed === '') {
      formattedLines.push('');
      continue;
    }

    if (decreaseIndentCurrentLine(trimmed)) {
      indentLevel = Math.max(indentLevel - 1, 0);
    }

    const indent = ' '.repeat(indentLevel * indentSize);
    formattedLines.push(indent + trimmed);

    if (increaseIndentNextLine(trimmed)) {
      indentLevel++;
    }

    // balance adjustment in case of multiple closing braces
    const openCount = (trimmed.match(/{/g) || []).length;
    const closeCount = (trimmed.match(/}/g) || []).length;
    indentLevel += openCount - closeCount;
    if (indentLevel < 0) indentLevel = 0;
  }

  return formattedLines.join('\n');
}
