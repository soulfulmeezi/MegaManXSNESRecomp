#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../build-recompui"
export PATH="/mingw64/bin:$PATH"
# Restore the existing recovery routes used by the companion experiment.
export MMX_CPU_CRUMBLE=1 MMX_CPU_PIT_WALL=1 MMX_CPU_WALL_ARC=1
if [[ ! -f MegaManXSNESRecomp.exe || ! -f rom.cfg ]]; then
  echo 'Build the game and choose your ROM in the launcher first.' >&2
  exit 1
fi
cosmic_rom=$(cat rom.cfg)
cosmic_rom=${cosmic_rom//$'\r'/}
exec ./MegaManXSNESRecomp.exe --no-launcher --rom "$cosmic_rom"
