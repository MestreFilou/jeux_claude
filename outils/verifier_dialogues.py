"""Vérifie les fichiers de dialogue sans lancer Unreal.

1. Contrôles statiques : noeuds cibles existants, compétences et personnages connus,
   difficultés valides, drapeaux demandés mais jamais posés.
2. Parties simulées au hasard (mêmes règles que le jeu) : détecte les impasses,
   c'est-à-dire un noeud sans fin de scène où aucun choix n'est proposé.

Usage : python3 outils/verifier_dialogues.py [--parties 3000]
Code de sortie 1 s'il y a une erreur.
"""
import argparse
import random
import re
import sys
from pathlib import Path

RACINE = Path(__file__).resolve().parent.parent
DOSSIER = RACINE / "HuitThermidor" / "Content" / "Dialogues"

COMPETENCES = ["logique", "rhetorique", "empathie", "volonte", "ferveur", "spectres",
               "endurance", "poigne", "perception", "sangfroid"]
PERSONNAGES = {"narrateur", "marie", "messager", "crieur", "porteur", "mercier",
               "anselme", "vadier", "sansculotte", "tricoteuse", "huissier", "tallien", "secretaire", "saintjust", "robespierre", "assemblee", "garnier", "augustin", "piquier", "guichetier"}
PROFILS = {
    "Le Zélé": dict(ferveur=4, poigne=4, endurance=3, volonte=3, sangfroid=2, perception=2,
                    logique=1, rhetorique=1, empathie=1, spectres=1),
    "L'Avocat": dict(logique=4, rhetorique=4, perception=3, sangfroid=3, ferveur=2, volonte=2,
                     empathie=2, spectres=1, endurance=1, poigne=1),
    "Le Hanté": dict(empathie=4, spectres=4, volonte=3, perception=2, rhetorique=2, logique=2,
                     endurance=2, sangfroid=1, poigne=1, ferveur=1),
}


# ---------- Lecture du format à accolades ----------
def lire_kv(texte):
    motif = re.compile(r'//[^\n]*|"((?:\\.|[^"\\])*)"|(\{)|(\})|([^\s{}"]+)')
    pile = [[]]
    cle = None
    for m in motif.finditer(texte):
        chaine, ouvre, ferme, mot = m.groups()
        if ouvre:
            bloc = []
            pile[-1].append((cle, bloc))
            pile.append(bloc)
            cle = None
        elif ferme:
            pile.pop()
        elif chaine is not None or mot is not None:
            valeur = chaine if chaine is not None else mot
            if cle is None:
                cle = valeur
            else:
                pile[-1].append((cle, valeur))
                cle = None
    return pile[0]


def get(paires, cle):
    return next((v for k, v in paires if k == cle), None)


def get_all(paires, cle):
    return [v for k, v in paires if k == cle]


def charger():
    scenes = {}
    for f in sorted(DOSSIER.glob("scene*.txt")):
        racine = lire_kv(f.read_text(encoding="utf-8"))
        d = get(racine, "dialogue")
        scenes[get(d, "id")] = {
            "fichier": f.name, "title": get(d, "title"), "start": get(d, "start"),
            "next": get(d, "next"), "nodes": dict(get(d, "nodes")),
        }
    return scenes


# ---------- Contrôles statiques ----------
def controler(scenes):
    erreurs, alertes = [], []
    poses = set()
    demandes = set()
    for sid, sc in scenes.items():
        for nid, noeud in sc["nodes"].items():
            poses.update(get_all(noeud, "set"))
    for sid, sc in scenes.items():
        nodes = sc["nodes"]
        ou = sc["fichier"]
        if sc["start"] not in nodes:
            erreurs.append(f"{ou} : noeud de départ « {sc['start']} » introuvable")
        if sc["next"] and sc["next"] not in scenes:
            erreurs.append(f"{ou} : scène suivante « {sc['next']} » introuvable")
        for nid, noeud in nodes.items():
            for k, v in noeud:
                if k == "line":
                    sp = get(v, "speaker")
                    if sp not in PERSONNAGES and sp not in COMPETENCES:
                        erreurs.append(f"{ou}/{nid} : personnage inconnu « {sp} »")
                    p = get(v, "passive")
                    if p and p not in COMPETENCES:
                        erreurs.append(f"{ou}/{nid} : compétence passive inconnue « {p} »")
                    if p and not (get(v, "difficulty") or "").isdigit():
                        erreurs.append(f"{ou}/{nid} : difficulté manquante pour un test passif")
                    if not get(v, "text"):
                        erreurs.append(f"{ou}/{nid} : réplique sans texte")
                if k == "choice":
                    chk = get(v, "check")
                    cibles = [get(v, "success"), get(v, "failure")] if chk else [get(v, "goto")]
                    for c in cibles:
                        if c not in nodes:
                            erreurs.append(f"{ou}/{nid} : le choix « {get(v, 'text')} » mène à « {c} », introuvable")
                    if chk:
                        if chk not in COMPETENCES:
                            erreurs.append(f"{ou}/{nid} : compétence inconnue « {chk} »")
                        if get(v, "type") not in ("blanc", "rouge"):
                            erreurs.append(f"{ou}/{nid} : type de test « {get(v, 'type')} » (blanc ou rouge attendu)")
                        if not (get(v, "difficulty") or "").isdigit():
                            erreurs.append(f"{ou}/{nid} : difficulté manquante")
                if k in ("line", "choice"):
                    demandes.update(get_all(v, "require"))
                if k == "next" and v not in scenes:
                    erreurs.append(f"{ou}/{nid} : scène suivante « {v} » introuvable")
    for f in sorted(demandes - poses):
        alertes.append(f"drapeau demandé mais jamais posé : « {f} »")
    return erreurs, alertes


# ---------- Parties simulées ----------
def simuler(scenes, parties, graine=1):
    rng = random.Random(graine)
    impasses = {}
    fins = {}
    premiere = next(iter(scenes))
    for n in range(parties):
        profil = list(PROFILS.values())[n % 3]
        skills = dict(profil)
        flags, tried, failed = set(), set(), {}
        sante, moral = skills["endurance"] + 2, skills["volonte"] + 2
        sid = premiere
        noeud = scenes[sid]["start"]
        pas = 0
        chemin = []
        while pas < 400:
            pas += 1
            nodes = scenes[sid]["nodes"]
            if noeud not in nodes:
                impasses.setdefault(f"{sid}/{noeud} (noeud manquant)", " > ".join(chemin[-6:]))
                break
            nd = nodes[noeud]
            chemin.append(f"{sid}/{noeud}")
            mort = False
            for k, v in nd:
                if k == "set":
                    flags.add(v)
                elif k in ("health", "morale"):
                    if k == "health":
                        sante += int(v)
                    else:
                        moral += int(v)
                    if sante <= 0 or moral <= 0:
                        mort = True
                        break
            if mort:
                fins["fin de partie"] = fins.get("fin de partie", 0) + 1
                break
            if get(nd, "end"):
                fins[f"{sid}/{noeud}"] = fins.get(f"{sid}/{noeud}", 0) + 1
                suivante = get(nd, "next") or scenes[sid]["next"]
                if not suivante:
                    break
                sid = suivante
                noeud = scenes[sid]["start"]
                sante = min(skills["endurance"] + 2, sante + 1)
                moral = min(skills["volonte"] + 2, moral + 1)
                continue
            visibles = []
            for i, c in enumerate(get_all(nd, "choice")):
                if not all(f in flags for f in get_all(c, "require")):
                    continue
                if any(f in flags for f in get_all(c, "unless")):
                    continue
                cle = f"{sid}:{noeud}:{i}"
                chk = get(c, "check")
                if chk and get(c, "type") == "rouge" and cle in tried:
                    continue
                if chk and cle in failed and skills[chk] <= failed[cle]:
                    continue
                visibles.append((i, c, cle))
            if not visibles:
                impasses.setdefault(f"{sid}/{noeud}", " > ".join(chemin[-6:]))
                break
            i, c, cle = rng.choice(visibles)
            chk = get(c, "check")
            if not chk:
                noeud = get(c, "goto")
                continue
            a, b = rng.randint(1, 6), rng.randint(1, 6)
            ok = (a == 6 and b == 6) or (not (a == 1 and b == 1) and a + b + skills[chk] >= int(get(c, "difficulty")))
            tried.add(cle)
            if get(c, "type") == "blanc":
                if ok:
                    failed.pop(cle, None)
                else:
                    failed[cle] = skills[chk]
            noeud = get(c, "success" if ok else "failure")
        else:
            impasses.setdefault("boucle sans fin", " > ".join(chemin[-6:]))
    return impasses, fins


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--parties", type=int, default=3000)
    args = ap.parse_args()

    scenes = charger()
    print(f"{len(scenes)} scènes : " + ", ".join(f"{k} ({len(v['nodes'])} noeuds)" for k, v in scenes.items()))
    erreurs, alertes = controler(scenes)
    impasses, fins = simuler(scenes, args.parties)
    for a in alertes:
        print("ALERTE  ", a)
    for e in erreurs:
        print("ERREUR  ", e)
    for lieu, chemin in impasses.items():
        print(f"IMPASSE  {lieu}   (chemin : {chemin})")
    print(f"\n{args.parties} parties simulées. Fins atteintes :")
    for fin, nb in sorted(fins.items(), key=lambda x: -x[1]):
        print(f"  {nb:5d}  {fin}")
    if erreurs or impasses:
        sys.exit(1)
    print("\nAucune erreur.")


if __name__ == "__main__":
    main()
