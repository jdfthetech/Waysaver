#!/usr/bin/env bash
set -euo pipefail

waysaver --create-package \
  --name "Family Photos" \
  --id "org.example.family-photos" \
  --type images \
  --source photos \
  --slide-seconds 12 \
  --output family-photos.waysaver

