"""Assemble le prototype jouable : template.html + tous les fichiers de dialogues/.

Usage : python3 prototype/build.py  ->  écrit prototype/index.html
"""
from pathlib import Path

root = Path(__file__).resolve().parent.parent
scenes = sorted((root / "dialogues").glob("scene*.txt"))
blocks = "".join(
    f'<script type="text/plain" class="kv">{p.read_text(encoding="utf-8")}</script>\n' for p in scenes
)
template = (root / "prototype" / "template.html").read_text(encoding="utf-8")
(root / "prototype" / "index.html").write_text(template.replace("__KV__", blocks), encoding="utf-8")
print(f"prototype/index.html construit avec {len(scenes)} scènes")
