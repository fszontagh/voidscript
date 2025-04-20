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

export function deactivate() {}

function formatVoidScript(text: string): string {
  const lines = text.split(/\r?\n/);
  let indentLevel = 0;
  const indentSize = 4;
  return lines
    .map(line => {
      const trimmed = line.trim();
      if (trimmed === '') {
        return '';
      }
      if (trimmed.startsWith('}')) {
        indentLevel = Math.max(indentLevel - 1, 0);
      }
      const indent = ' '.repeat(indentLevel * indentSize);
      const newLine = indent + trimmed;
      const openCount = (trimmed.match(/{/g) || []).length;
      const closeCount = (trimmed.match(/}/g) || []).length;
      indentLevel += openCount - closeCount;
      if (indentLevel < 0) {
        indentLevel = 0;
      }
      return newLine;
    })
    .join('\n');
}