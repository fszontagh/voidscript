#!/bin/bash

set -e

ROOT_DIR="$(dirname "$0")"

echo "🔍 ESP8266 inkompatibilis C++ feature-ök keresése a projektben..."

PATTERNS=(
    'std::filesystem'
    'std::thread'
    'std::mutex'
    'std::future'
    'std::condition_variable'
    'std::async'
    'std::this_thread'
    'std::jthread'
    'std::stop_token'
    'std::chrono::high_resolution_clock'
    'pthread_'
    '#include <thread>'
    '#include <filesystem>'
    '#include <future>'
    '#include <mutex>'
    'fork('
    'pipe('
)

for pattern in "${PATTERNS[@]}"; do
    echo -e "\n🔸 Keresés: '$pattern'"
    find "$ROOT_DIR" \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.c" \) -exec grep --color=always -Hn "$pattern" {} +
done

echo -e "\n✅ Keresés kész."
