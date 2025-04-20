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
  const formattedLines: string[] = [];
  const indentSize = 4;

  let indentLevel = 0;
  const blockStack: string[] = [];

  function getIndent(): string {
    return ' '.repeat(indentLevel * indentSize);
  }

  function isBlockStart(line: string): boolean {
    return /{\s*$/.test(line.trim());
  }

  function isBlockEnd(line: string): boolean {
    return /^}/.test(line.trim());
  }

  for (let i = 0; i < lines.length; i++) {
    let line = lines[i].trim();

    if (line === '') {
      formattedLines.push('');
      continue;
    }

    if (isBlockEnd(line)) {
      indentLevel = Math.max(0, indentLevel - 1);
      const top = blockStack.pop();
    }

    const indent = getIndent();
    formattedLines.push(indent + line);

    if (isBlockStart(line)) {
      // Heuristic: guess block type for better future expansion
      if (line.includes('class')) blockStack.push('class');
      else if (line.includes('function')) blockStack.push('function');
      else if (line.startsWith('if')) blockStack.push('if');
      else if (line.startsWith('else')) blockStack.push('else');
      else blockStack.push('block');

      indentLevel++;
    }
  }

  return formattedLines.join('\n');
}
