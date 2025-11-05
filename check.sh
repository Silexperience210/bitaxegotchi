#!/bin/bash

echo "🔧 BitaxGotchi - Vérification ESP32-S3"
echo "========================================"
echo ""

# Vérifier les fichiers HTML et JSON
if [ -f "index.html" ]; then
    size=$(stat -f%z "index.html" 2>/dev/null || stat -c%s "index.html" 2>/dev/null)
    echo "✅ index.html trouvé ($(numfmt --to=iec-i --suffix=B $size 2>/dev/null || echo $(($size/1024))KB))"
else
    echo "❌ index.html manquant"
fi

if [ -f "manifest.json" ]; then
    # Vérifier que c'est bien ESP32-S3
    if grep -q "ESP32-S3" manifest.json; then
        echo "✅ manifest.json trouvé (ESP32-S3 ✓)"
    else
        echo "⚠️  manifest.json trouvé mais chipFamily n'est pas ESP32-S3 !"
    fi
else
    echo "❌ manifest.json manquant"
fi

echo ""
echo "Fichiers binaires ESP32-S3:"
echo "----------------------------"

# Fonction pour vérifier taille
check_bin() {
    local file=$1
    local min_size=$2
    local max_size=$3
    local desc=$4
    
    if [ -f "$file" ]; then
        size=$(stat -f%z "$file" 2>/dev/null || stat -c%s "$file" 2>/dev/null)
        size_kb=$((size/1024))
        
        if [ $size -ge $min_size ] && [ $size -le $max_size ]; then
            echo "✅ $file ($size_kb KB) - $desc ✓"
        else
            echo "⚠️  $file ($size_kb KB) - Taille suspecte!"
        fi
    else
        echo "❌ $file manquant - $desc"
    fi
}

# Vérifier chaque binaire avec tailles attendues pour S3
check_bin "bootloader.bin" 25000 40000 "Bootloader ESP32-S3 (offset 0x0)"
check_bin "partitions.bin" 2000 5000 "Table de partitions"
check_bin "boot_app0.bin" 4096 4096 "Boot OTA"
check_bin "firmware.bin" 100000 3000000 "Firmware principal"

echo ""
echo "========================================"
echo ""

# Compter les fichiers manquants
missing=0
for file in bootloader.bin partitions.bin boot_app0.bin firmware.bin; do
    if [ ! -f "$file" ]; then
        ((missing++))
    fi
done

if [ $missing -eq 0 ]; then
    echo "🎉 Tous les fichiers sont présents !"
    echo ""
    echo "Tailles attendues pour ESP32-S3:"
    echo "  • bootloader.bin  → 30-35 KB"
    echo "  • partitions.bin  → 3 KB"
    echo "  • boot_app0.bin   → 4 KB"
    echo "  • firmware.bin    → 200-1500 KB"
    echo ""
    echo "Offsets ESP32-S3 (différents de ESP32 !):"
    echo "  • 0x0     → bootloader.bin"
    echo "  • 0x8000  → partitions.bin"
    echo "  • 0xe000  → boot_app0.bin"
    echo "  • 0x10000 → firmware.bin"
    echo ""
    echo "✅ Prêt pour GitHub Pages!"
else
    echo "⚠️  $missing fichier(s) binaire(s) manquant(s)"
    echo ""
    echo "Lis ESP32-S3_GUIDE.txt pour les instructions"
    echo "complètes de récupération depuis Arduino IDE"
fi