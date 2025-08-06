#!/bin/env bash
###
 # @Author: Uyanide pywang0608@foxmail.com
 # @Date: 2025-08-06 01:43:32
 # @LastEditTime: 2025-08-06 02:28:12
 # @Description:
###

path="$(dirname "$(readlink -f "$0")")"

cmake -S "$path/.." -B "$path/../build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$HOME"/.local || exit 1

cmake --build "$path/../build" --target install || exit 1

cp "$path/wallpaper_chooser.desktop" "$HOME"/.local/share/applications/wallpaper_chooser.desktop

echo "Exec=$HOME/.local/bin/wallpaper_chooser" >> "$HOME"/.local/share/applications/wallpaper_chooser.desktop

if command -v update-desktop-database &> /dev/null; then
    update-desktop-database "$HOME"/.local/share/applications/
fi