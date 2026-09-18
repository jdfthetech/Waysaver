#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
build_dir="${project_dir}/build"

cmake -S "${project_dir}" -B "${build_dir}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="${HOME}/.local"
cmake --build "${build_dir}" -j"$(nproc)"
cmake --install "${build_dir}"

systemctl --user daemon-reload
systemctl --user enable --now waysaver-daemon.service
update-desktop-database "${HOME}/.local/share/applications" 2>/dev/null || true

printf '%s\n' "WaySaver was installed. Open WaySaver from the application launcher."

