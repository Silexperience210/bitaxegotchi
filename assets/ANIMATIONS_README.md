# BitaxGotchi - Guide d'Animation Personnalisée

## 🎨 Bitcoin Personnifié Animé

Ce projet inclut un système d'animation évolutif pour un Bitcoin personnifié (BitCoin-Chan) qui réagit aux activités de minage.

## 🎭 États Disponibles

Le personnage Bitcoin possède 6 états différents avec animations uniques :

### 1. **Idle** (Repos)
- Animation : Flottement doux
- Utilisation : Quand le mineur est connecté mais calme
- Bras : Balancement léger

### 2. **Mining** (Minage)
- Animation : Vibration/secousse énergique
- Utilisation : Pendant le minage actif
- Effets : Particules d'énergie orange
- Bras : Mouvements de travail rapides

### 3. **Happy** (Heureux)
- Animation : Rebondissements joyeux
- Utilisation : Quand des shares sont trouvés
- Bouche : Grand sourire
- Bras : Vagues de célébration

### 4. **Excited** (Super Excité)
- Animation : Sauts énergiques avec rotation
- Utilisation : Quand le hashrate est élevé ou block trouvé
- Effets : Éclat lumineux intense, particules d'énergie
- Yeux : Brillent avec animation

### 5. **Tired** (Fatigué)
- Animation : Balancement lent
- Utilisation : Hashrate faible ou surchauffe
- Yeux : Mi-fermés (lignes)
- Bouche : Petit sourire fatigué

### 6. **Sleeping** (Endormi)
- Animation : Respiration douce
- Utilisation : Mineur déconnecté ou hashrate = 0
- Yeux : Fermés
- Bouche : Petite "o"

## 🎮 Interactions

### Suivi du Curseur
Les yeux du Bitcoin suivent le curseur de la souris pour une interaction vivante.

### Click
Cliquer sur le personnage le fait passer temporairement en état "Excited" pendant 2 secondes.

## 🔧 Intégration avec API Bitaxe

Pour connecter les animations aux données réelles du mineur :

```javascript
// Exemple d'utilisation
bitcoinChar.updateFromMiningStats(hashrate, shares, temperature);
```

La méthode `updateFromMiningStats()` change automatiquement l'état selon :
- `hashrate === 0` → Sleeping
- `hashrate < 100` → Tired
- `hashrate > 500` → Excited
- `shares % 10 === 0` → Happy
- Sinon → Mining

## 🎨 Ajouter des GIFs Personnalisés

### Option 1 : Remplacer par des GIFs réels

1. Créez vos GIFs animés pour chaque état :
   - `idle.gif`
   - `mining.gif`
   - `happy.gif`
   - `excited.gif`
   - `tired.gif`
   - `sleeping.gif`

2. Placez-les dans `/assets/animations/`

3. Modifiez le HTML pour utiliser des balises `<img>` :

```html
<div class="creature-container">
    <img id="bitcoinGif" src="./assets/animations/idle.gif" alt="Bitcoin Character">
</div>
```

4. Mettez à jour le JavaScript pour changer la source :

```javascript
setState(state) {
    this.display.querySelector('#bitcoinGif').src = `./assets/animations/${state}.gif`;
    this.nameElement.textContent = this.stateNames[state];
}
```

### Option 2 : Sprite Sheets

Si vous avez des sprite sheets :

1. Placez votre sprite sheet dans `/assets/animations/bitcoin-sprites.png`
2. Utilisez des animations CSS avec `background-position` pour chaque état

### Option 3 : Animations SVG

Pour des animations vectorielles :
1. Créez des fichiers SVG animés
2. Placez-les dans `/assets/animations/`
3. Utilisez `<object>` ou `<embed>` pour les afficher

## 📝 Personnalisation

### Modifier les Couleurs

Changez les couleurs du Bitcoin dans le CSS :

```css
.bitcoin-body {
    background: linear-gradient(135deg, #f7931a 0%, #ff9500 50%, #f7931a 100%);
}
```

### Ajuster la Vitesse des Animations

Modifiez les durées dans les `@keyframes` :

```css
@keyframes mining-shake {
    /* Changez 0.5s pour plus rapide/lent */
    animation: mining-shake 0.5s ease-in-out infinite;
}
```

### Ajouter de Nouveaux États

1. Ajoutez le CSS pour le nouvel état
2. Ajoutez l'état dans `this.states` et `this.stateNames`
3. Créez les animations correspondantes

## 🎬 Mode Démo

Par défaut, le personnage change d'état toutes les 4 secondes pour démonstration.

Pour désactiver le mode démo :
```javascript
// Commentez cette ligne dans le constructeur
// this.startStateRotation();
```

## 💡 Exemples d'Utilisation

### Changement Manuel d'État
```javascript
bitcoinChar.setState('mining');  // Passe en mode minage
bitcoinChar.setState('happy');   // Passe en mode heureux
```

### Connexion Temps Réel
```javascript
// Dans votre fonction de mise à jour API
async function updateFromBitaxe() {
    const data = await fetchBitaxeAPI();
    bitcoinChar.updateFromMiningStats(
        data.hashrate,
        data.shares,
        data.temperature
    );
}
setInterval(updateFromBitaxe, 5000); // Mise à jour toutes les 5 secondes
```

## 🎯 Prochaines Améliorations Possibles

- [ ] Ajouter des sons pour chaque état
- [ ] Créer des variantes de personnages (évolutions)
- [ ] Ajouter des accessoires déblocables
- [ ] Système d'XP et de niveaux
- [ ] Particules personnalisées par état
- [ ] Support multi-langues pour les noms d'états

---

**Créé par Silexperience pour Plebs Miners** 🚀
