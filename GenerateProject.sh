#!/bin/bash

cd "$(dirname "$0")"

# Detect if the compatibility.sharpmake.cs file exists, and set CompatibilityCS accordingly
CompatibilityCS="Build/nocompatibility.sharpmake.cs"
if [ -f "_generated/imgui/compatibility.sharpmake.cs" ]; then
  CompatibilityCS="_generated/imgui/compatibility.sharpmake.cs"
fi

./Build/Sharpmake/Sharpmake.Application "/sources(@'Build/shared.sharpmake_mac.cs', @'$CompatibilityCS', @'Build/netImgui.sharpmake.cs') /verbose() "

if [ "$1" = "1" ]; then
  exit 0
fi

read -p "Press any key to continue..."