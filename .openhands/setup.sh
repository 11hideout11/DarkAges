#!/bin/bash
# OpenHands repository setup for DarkAges MMO
# This runs automatically when OpenHands first loads this repository

echo "[OpenHands] Setting up DarkAges project environment..."

# Install pre-commit hooks (optional)
if [ -d ".git" ]; then
    echo "[OpenHands] Installing pre-commit hooks..."
    python -m pip install pre-commit >/dev/null 2>&1 || true
    pre-commit install || true
fi

echo "[OpenHands] Environment ready."
