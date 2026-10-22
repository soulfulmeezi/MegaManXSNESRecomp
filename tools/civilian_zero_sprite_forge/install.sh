#!/usr/bin/env bash
set -Eeuo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
PIN=5324342fdfab72430f4e9604f3fe427009f8596d
NOW="$(date -u +%Y%m%dT%H%M%SZ)"
CACHE="${HOME}/.cache/cosmicx/agent-sprite-forge"
OUT="$ROOT/updates/agent-sprite-forge-install-$NOW"
mkdir -p "$OUT"
exec > >(tee "$OUT/build_log.txt") 2>&1
trap 'rc=$?; echo "Installer failed at: $BASH_COMMAND (exit $rc)" >> "$OUT/report.txt"' ERR

# Protect tracked project edits; never add arbitrary untracked files/tokens to a commit.
git -C "$ROOT" add -u
if ! git -C "$ROOT" diff --cached --quiet; then
  git -C "$ROOT" commit -m "backup: tracked Cosmic X state before sprite forge"
fi
BACKUP="backup/pre-sprite-forge-local-$NOW"
git -C "$ROOT" branch "$BACKUP"
git -C "$ROOT" switch -c "tooling/sprite-forge-local-$NOW"
printf 'git switch %s\n' "$BACKUP" > "$OUT/rollback_command.txt"

PY=python
if command -v python3 >/dev/null; then PY=python3; fi
"$PY" -c 'import sys; assert sys.version_info >= (3,10)'
# When launched from MSYS2, prefer the Windows Codex home over the MSYS2 home.
if [[ -z "${CODEX_HOME:-}" && -n "${USERPROFILE:-}" ]] && command -v cygpath >/dev/null 2>&1; then
  export CODEX_HOME="$(cygpath -m "$USERPROFILE")/.codex"
fi
if [ ! -e "$CACHE/.git" ]; then
  mkdir -p "$(dirname "$CACHE")"
  git clone --filter=blob:none --no-checkout https://github.com/0x0funky/agent-sprite-forge.git "$CACHE"
fi
git -C "$CACHE" sparse-checkout init --cone
git -C "$CACHE" sparse-checkout set skills shared tools
git -C "$CACHE" fetch --depth=1 origin "$PIN"
git -C "$CACHE" checkout --detach "$PIN"
test "$(git -C "$CACHE" rev-parse HEAD)" = "$PIN"
if ! "$PY" -c 'import numpy, PIL, scipy' >/dev/null 2>&1; then
  echo 'Missing Python modules. Install dependencies first with:'
  echo "$PY -m pip install -r $CACHE/requirements.txt"
  exit 2
fi
"$PY" "$CACHE/tools/install_skills.py" --apply --host codex --skills generate2dsprite,video2dsprite,generate2dmedia,codeart2d
echo 'Tooling installed for Codex; game runtime not changed. Restart Codex.' | tee "$OUT/report.txt"
