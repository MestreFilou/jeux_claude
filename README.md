# Huit Thermidor

Jeu narratif autonome inspiré de Disco Elysium, dans le Paris de la Terreur.
Le 8 thermidor an II (26 juillet 1794), un commissaire du comité de surveillance
de la section des Piques se réveille sans mémoire, une suspecte ligotée devant lui.
Le lendemain, Robespierre tombera.

On avance surtout en parlant. Dix compétences ont chacune une voix intérieure qui
commente la scène, et les tests se règlent avec 2d6 + le niveau de la compétence.

Moteur visé : Unreal Engine 5, en vue à la première personne.

## Contenu du dépôt

| Dossier | Contenu |
| --- | --- |
| `dialogues/` | Une scène par fichier, au format texte à accolades. C'est la source unique du texte du jeu. |
| `prototype/` | Prototype jouable dans le navigateur, qui lit les fichiers de `dialogues/`. Sert à tester l'écriture et l'équilibrage avant Unreal. |

## Jouer au prototype

```
python3 prototype/build.py
```

puis ouvrir `prototype/index.html` dans un navigateur. Les touches 1 à 9 choisissent une réplique.

## Règles

- **Test actif** : 2d6 + niveau ≥ difficulté (Facile 8, Moyen 10, Difficile 12, Redoutable 14, Héroïque 16). Double 6 réussit toujours, double 1 échoue toujours.
- **Test blanc** : retentable après avoir monté la compétence. **Test rouge** : une seule chance.
- **Test passif** : niveau + 6 ≥ difficulté, sans dé. S'il passe, la compétence parle d'elle-même.
- **Jauges** : Santé (Endurance + 2) et Moral (Volonté + 2). À zéro, la partie s'arrête.
- **Progression** : 1 point d'expérience par test réussi, 2 par fin de scène. Tous les 3 points, une compétence monte de 1.

## Compétences

| Attribut | Compétence | Sa voix |
| --- | --- | --- |
| Intellect | Logique | Froide, déduit et accuse |
| Intellect | Rhétorique | Cherche la faille dans chaque phrase |
| Psyché | Empathie | Sent ce que les gens taisent |
| Psyché | Volonté | Tient le moral, refuse de céder |
| Psyché | Ferveur | La voix de la Révolution, en majuscules : slogans, vertu, soupçon |
| Psyché | Spectres | Visions des morts et de l'Ancien Régime |
| Physique | Endurance | Le corps, la douleur, la gueule de bois |
| Physique | Poigne | Envie d'autorité et de violence |
| Motricité | Perception | Voit et entend les détails |
| Motricité | Sang-froid | Garde le visage neutre, ment bien |

## Format des dialogues

```
"noeud"
{
	"music"	"Musique.Tension"
	"set"	"drapeau"
	"line"	{ "speaker" "empathie" "passive" "empathie" "difficulty" "10" "text" "..." }
	"choice"	{ "text" "..." "check" "rhetorique" "difficulty" "10" "type" "rouge" "success" "a" "failure" "b" }
	"choice"	{ "text" "..." "goto" "c" "require" "drapeau" "unless" "autre_drapeau" }
}
```

Les clés sont traitées dans l'ordre. Les drapeaux restent posés d'une scène à l'autre.
