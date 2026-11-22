#!/usr/bin/env python3
"""Publie le programme compilé sur la branche corbeau (canal « test » du Core2).

Usage : publier_corbeau.py <bin compilé> <version> <version.json> <firmware/core2_francais.bin> <sujet du commit>

Contrôles avant d'écrire quoi que ce soit : taille <= 6 553 600 octets, en-tête de programme ESP32.
Écrit : le .bin dans firmware/, et version.json (version, bin, sha256, size, notes, canal, build).
Codes de sortie : 0 publié ; 2 contrôle échoué (rien écrit) ; 3 une version plus récente est déjà publiée (on ne la remplace pas).
"""
import hashlib, json, os, shutil, sys

RAW = "https://raw.githubusercontent.com/hadesdray-crypto/core2-francais/corbeau/"
TAILLE_MAX = 6553600
TAILLE_MIN = 1000000


def tuple_version(v):
    try:
        return tuple(int(x) for x in v.split("."))
    except Exception:
        return (0,)


def main():
    binaire, version, chemin_json, chemin_bin, sujet = sys.argv[1:6]
    donnees = open(binaire, "rb").read()
    taille = len(donnees)
    if taille > TAILLE_MAX or taille < TAILLE_MIN:
        print("ERREUR : taille %d hors limites (%d..%d)" % (taille, TAILLE_MIN, TAILLE_MAX)); return 2
    if donnees[0] != 0xE9 or donnees[12] != 0 or donnees[13] != 0 or donnees[32:36] != bytes([0x32, 0x54, 0xCD, 0xAB]):
        print("ERREUR : ce fichier n'est pas un programme ESP32 valide"); return 2
    if not version or any(not p.isdigit() for p in version.split(".")):
        print("ERREUR : version illisible : %r" % version); return 2
    sha = hashlib.sha256(donnees).hexdigest()

    d = json.load(open(chemin_json, "r", encoding="utf-8"))
    deja = d.get("build", {}).get("version") if isinstance(d.get("build"), dict) else None
    if deja and tuple_version(deja) > tuple_version(version):
        print("Une version plus récente (%s) est déjà publiée : %s n'est pas publiée." % (deja, version)); return 3

    notes = [sujet[:150]]
    changes = d.get("changes", [])
    for c in changes[-2:][::-1]:
        c = " ".join(str(c).split())
        notes.append(c if len(c) <= 150 else c[:147] + "...")
    d["version"] = version
    d["bin"] = RAW + "firmware/core2_francais.bin?v=" + sha[:12]      # l'adresse change à chaque programme : jamais une vieille copie en cache
    d["sha256"] = sha
    d["size"] = taille
    d["canal"] = "test"
    d["notes"] = notes[:3]
    d["build"] = {"version": version, "run": os.environ.get("GITHUB_RUN_ID", ""), "commit": os.environ.get("GITHUB_SHA", "")[:12]}
    # même ordre de clés qu'avant, les nouvelles à la suite
    with open(chemin_json, "w", encoding="utf-8", newline="\n") as f:
        json.dump(d, f, ensure_ascii=False, indent=4); f.write("\n")
    os.makedirs(os.path.dirname(chemin_bin) or ".", exist_ok=True)
    shutil.copyfile(binaire, chemin_bin)
    # relecture : ce qui est déposé doit être exactement ce qui a été compilé
    if hashlib.sha256(open(chemin_bin, "rb").read()).hexdigest() != sha:
        print("ERREUR : le fichier déposé diffère du fichier compilé"); return 2
    print("Publié : version %s, %d octets, sha256 %s" % (version, taille, sha))
    return 0


if __name__ == "__main__":
    sys.exit(main())
