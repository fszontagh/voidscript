#!/bin/bash

# Script to fix non-English commit messages via interactive rebase
# Run this in your repository root. Assumes commits e330a54 and a822cff are on main branch.

# Check if we're on the main branch
if [ "$(git branch --show-current)" != "main" ]; then
    echo "Error: Not on main branch. Switch to main and try again."
    exit 1
fi

# Get the hash of the commit before a822cff (to rebase from there)
# Assuming a822cff is the older commit, rebase from its parent.
# If a822cff is the first commit, adjust accordingly.
BASE_COMMIT=$(git rev-parse a822cff~1 2>/dev/null || echo "HEAD~2")

# Start interactive rebase from the base commit to edit the last two commits
git rebase -i "$BASE_COMMIT"

# After running, your editor will open with instructions.
# Change "pick" to "edit" for the commits you want to amend:
# - For a822cff: Keep as "pick" (already English).
# - For e330a54: Change to "edit", then run:
#   git commit --amend -m "fix: Implementation of missing methods in SymbolContainer"
#   git rebase --continue

echo "Rebase initiated. Follow the editor instructions to edit the messages."
echo "After editing, run 'git rebase --continue' for each edited commit."
