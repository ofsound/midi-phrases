#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"

git -C "$root" config core.hooksPath .githooks
chmod +x "$root/.githooks/pre-commit"

echo "Git hooks enabled: core.hooksPath=.githooks"
echo "Commits that touch ui/ will run: npm run check (in ui/)"
