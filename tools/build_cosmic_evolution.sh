#!/usr/bin/env bash
# Run in MSYS2 MinGW64 from the NEW evolution checkout.
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
reference="${1:-$HOME/CosmicFX_Test}"
reference="$(cd "$reference" && pwd)"
if [[ "$root" == "$reference" ]]; then
  echo 'Use a separate evolution checkout; the reference build must stay untouched.' >&2
  exit 1
fi
for tool in git cmake ninja; do
  command -v "$tool" >/dev/null || { echo "Missing $tool; run from MSYS2 MinGW64." >&2; exit 1; }
done
if [[ ! -f "$root/mmx.sfc" ]]; then
  [[ -f "$reference/mmx.sfc" ]] || { echo "Place your own mmx.sfc in $root before building." >&2; exit 1; }
  cp "$reference/mmx.sfc" "$root/mmx.sfc"
fi
cd "$root"
bash tools/bootstrap.sh
bash tools/regen.sh usa --no-tests
cmake -S . -B build-evolution -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/mingw64
cmake --build build-evolution --parallel 4
ctest --test-dir build-evolution --output-on-failure
# Reuse local settings and extracted source caches, never overwrite a prior
# evolution configuration. The generated mod catalog stays with this build.
previous="$reference/build-recompui"
if [[ -d "$previous" ]]; then
  for name in config.ini config.local.ini keybinds.ini; do
    if [[ -f "$previous/$name" && ! -e "build-evolution/$name" ]]; then cp "$previous/$name" "build-evolution/$name"; fi
  done
  if [[ -d "$previous/cache" && ! -e build-evolution/cache ]]; then cp -R "$previous/cache" build-evolution/cache; fi
  for name in mods/state.toml mods/preloaded/state.toml; do
    if [[ -f "$previous/$name" && ! -e "build-evolution/$name" ]]; then
      mkdir -p "build-evolution/$(dirname "$name")"
      cp "$previous/$name" "build-evolution/$name"
    fi
  done
fi
printf '\nBuilt experimental update. Launch:\ncd "%s/build-evolution"\n./MegaManXSNESRecomp.exe\n' "$root"
printf 'If prompted, select your own original X1/X3 ROMs in the launcher.\n'
