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
| `HuitThermidor/` | Le projet Unreal Engine 5 (C++). |
| `HuitThermidor/Content/Dialogues/` | Une scène par fichier, au format texte à accolades. C'est la source unique du texte : le jeu et le prototype lisent les mêmes fichiers. |
| `prototype/` | Prototype jouable dans le navigateur, pour tester l'écriture et l'équilibrage. |
| `outils/verifier_dialogues.py` | Vérifie les dialogues et simule des milliers de parties pour trouver les impasses. Lancé automatiquement sur GitHub à chaque envoi. |

## Lancer le jeu dans Unreal

Il faut Windows, Unreal Engine 5.4 ou plus récent (Epic Games Launcher) et Visual Studio 2022
avec la charge de travail « Développement de jeux en C++ ».

1. Clic droit sur `HuitThermidor/HuitThermidor.uproject` > **Generate Visual Studio project files**.
2. Double-clic sur `HuitThermidor.uproject`. Unreal propose de compiler le module : accepter.
3. Cliquer sur **Play**. Aucune carte à créer : le jeu démarre sur la carte vide du moteur et construit les décors par le code.

Commandes : souris pour choisir, touches **1 à 9** pour répondre, **Tab** pour fermer le carnet et explorer
(seulement quand la conversation le permet), **ZQSD** ou **WASD** pour marcher, **E** pour interagir.

Si la compilation échoue, copier le message d'erreur de Visual Studio (fenêtre « Liste d'erreurs ») :
le code n'a pas encore pu être compilé, il faudra sans doute quelques corrections.

### Musique

Importer les pistes dans `Content/Audio` avec ces noms : `Musique_Reveil`, `Musique_Place`,
`Musique_Tension`, `Musique_Fin`. Tant qu'un fichier manque, la scène reste silencieuse.

### Comment le code est organisé

| Fichier | Rôle |
| --- | --- |
| `Dialogue/KeyValues` | Lit le format à accolades. |
| `Dialogue/DialogueSubsystem` | Les règles : noeuds, drapeaux, jets de dés, jauges, expérience. Mêmes règles que le prototype. |
| `UI/SDialoguePanel` | L'interface façon Disco Elysium, en Slate (pas de fichier binaire). |
| `Game/Commissaire` | Le joueur à la première personne : marche, vise, interagit. |
| `Game/ThermidorPlayerController` | Bascule entre le carnet de dialogue et l'exploration. |
| `Game/Interactif` | Personnages et objets qui déclenchent un choix du dialogue. |
| `Game/ThermidorSets` | Les décors en maquette (formes simples) : salle du comité, place des Piques. |
| `Game/ThermidorGameMode` | Construit le décor de chaque scène et joue la musique. |

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

Autres clés d'un noeud : `"health"` / `"morale"` (modifient les jauges), `"hub" "1"` (dans Unreal, on peut
fermer le carnet et explorer le décor), `"end" "1"` (fin de scène, avec `"next"` pour choisir la scène suivante
selon la fin obtenue).

Dans Unreal, un personnage ou un objet du décor déclenche le choix du noeud courant qui mène au noeud indiqué
dans ses `TargetNodes` (voir `Game/ThermidorSets.cpp`).
