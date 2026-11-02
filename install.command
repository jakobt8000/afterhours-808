#!/bin/bash
# 808 · AFTERHOURS — installs the plugin system-wide (asks for your password)
cd "$(dirname "$0")"
echo "Installing 808..."
sudo mkdir -p /Library/Audio/Plug-Ins/VST3 /Library/Audio/Plug-Ins/Components
# removes the old 808 (also old copies in your user folder)
sudo rm -rf /Library/Audio/Plug-Ins/VST3/808.vst3 /Library/Audio/Plug-Ins/Components/808.component
rm -rf ~/Library/Audio/Plug-Ins/VST3/808.vst3 ~/Library/Audio/Plug-Ins/Components/808.component 2>/dev/null
sudo cp -R 808.vst3 /Library/Audio/Plug-Ins/VST3/
sudo cp -R 808.component /Library/Audio/Plug-Ins/Components/
sudo xattr -cr /Library/Audio/Plug-Ins/VST3/808.vst3 /Library/Audio/Plug-Ins/Components/808.component
killall -9 AudioComponentRegistrar 2>/dev/null
echo "Done. Restart Ableton and rescan plug-ins. 808 is in the AFTERHOURS folder."
