````markdown
# BitaxeGotchi — WebFlasher

Ce dossier contient deux clients web pour déployer un fichier `.bin` vers votre device BitaxeGotchi.

## 1) OTA HTTP (ota-uploader.html) — recommandé
Fonctionnement :
- Le navigateur fait un POST multipart/form-data vers `http://<device-ip>/update` (ou chemin configurable).
- Le device (ESP) doit implémenter un endpoint serveur qui accepte l'upload et écrit le .bin en flash (par ex. Arduino/ESPAsyncWebServer update handler).
- Avantages : simple, progress visible côté navigateur, fonctionne sans accès série.
- Limites : nécessite que le device tourne déjà un serveur réseau et accepte l'upload.

Exemple d'utilisation :
1. Démarrez la page `ota-uploader.html`.
2. Entrez l'URL du device (ex: http://192.168.4.1).
3. Sélectionnez le .bin et cliquez sur "Démarrer l'upload".
4. Vérifiez les logs et le redémarrage du device.

Sécurité :
- Cette page n'ajoute pas d'authentification. Protégez l'endpoint /update (token, mot de passe, réseau isolé).

## 2) Web Serial (serial-flasher.html) — attention
Fonctionnement :
- Utilise l'API Web Serial du navigateur pour ouvrir un port série et envoyer le binaire.
- NE gère PAS le protocole esptool. C'est utile uniquement si votre bootloader accepte un flux binaire brut ou si vous avez un petit chargeur qui reçoit et write le binaire.

Pour flasher un ESP32/ESP8266 en mode bootloader natif, utilisez plutôt :
- esptool.py (cli)
- ou esp-web-flasher (projet open-source utilisant Web Serial + protocole esptool complet)

## Intégration avec BitaxeGotchi
- Si votre firmware BitaxeGotchi prévoit un endpoint HTTP `/update`, l'OTA uploader fonctionnera immédiatement.
- Sinon, ajoutez un petit gestionnaire OTA dans le firmware pour recevoir et écrire le .bin de façon sûre.
- Pour des flashtags massifs ou bootloader-level flashing, préférez esptool/esp-web-flasher.

## Liens utiles
- esptool.py : https://github.com/espressif/esptool
- esp-web-flasher : recherchez "esp-web-flasher" (projet open source qui implémente le protocole de bootloader dans le navigateur via Web Serial).

## Notes finales
Testez toujours sur un appareil de développement avant de déployer en production. Sauvegardez les configurations et double-checkez les versions du firmware.
````
