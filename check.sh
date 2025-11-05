#!/bin/bash

echo "🔧 BitPet - Vérification de la structure"
echo "========================================"
echo ""

# Vérifier les fichiers HTML et JSON
if [ -f "index.html" ]; then
    echo "✅ index.html trouvé"
else
    echo "❌ index.html manquant"
fi

if [ -f "manifest.json" ]; then
    echo "✅ manifest.json trouvé"
else
    echo "❌ manifest.json manquant"
fi

echo ""
echo "Fichiers binaires:"

# Vérifier les binaires
if [ -f "bootloader.bin" ]; then
    echo "✅ bootloader.bin trouvé ($(du -h bootloader.bin | cut -f1))"
else
    echo "❌ bootloader.bin manquant - REQUIS POUR FLASHER"
fi

if [ -f "partitions.bin" ]; then
    echo "✅ partitions.bin trouvé ($(du -h partitions.bin | cut -f1))"
else
    echo "❌ partitions.bin manquant - REQUIS POUR FLASHER"
fi

if [ -f "boot_app0.bin" ]; then
    echo "✅ boot_app0.bin trouvé ($(du -h boot_app0.bin | cut -f1))"
else
    echo "❌ boot_app0.bin manquant - REQUIS POUR FLASHER"
fi

if [ -f "firmware.bin" ]; then
    echo "✅ firmware.bin trouvé ($(du -h firmware.bin | cut -f1))"
else
    echo "❌ firmware.bin manquant - REQUIS POUR FLASHER"
fi

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
    echo "Tu peux push sur GitHub et activer Pages"
else
    echo "⚠️  $missing fichier(s) binaire(s) manquant(s)"
    echo "Ajoute tes fichiers .bin à la racine du repo"
fi