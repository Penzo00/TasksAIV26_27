# -*- coding: utf-8 -*-
"""
Genera le tabelle della guida "Rifare i livelli in Blueprint" leggendo
direttamente lo script che costruisce i livelli C++
(Content/Python/gd13_platformer.py) e gli header C++.

Non serve Unreal: lo script viene eseguito con dei moduli finti (unreal,
aiv_livelli, aiv_materiali) che, invece di piazzare gli actor, li annotano.
Così le tabelle della guida restano sempre uguali ai livelli veri: se si
cambia lo script, basta rilanciare

    python3 Docs/Blueprint_Livelli/strumenti/estrai_livelli.py

e ricompilare la guida (pdflatex main.tex, due volte).
"""

import inspect
import os
import re
import sys
import types

QUI = os.path.dirname(os.path.abspath(__file__))
RADICE = os.path.abspath(os.path.join(QUI, "..", "..", ".."))
SCRIPT = os.path.join(RADICE, "Content", "Python", "gd13_platformer.py")
HEADERS = os.path.join(RADICE, "Source", "AIV_UE5Intro", "Platformer")
USCITA = os.path.abspath(os.path.join(QUI, ".."))

TINTE = {
    "percorso": "MI\\_AIV\\_Percorso", "percorso2": "MI\\_AIV\\_PercorsoScuro", "salto": "MI\\_AIV\\_Salto",
    "muro": "MI\\_AIV\\_Muro", "pericolo": "MI\\_AIV\\_Pericolo", "traguardo": "MI\\_AIV\\_Traguardo",
    "riparo": "MI\\_AIV\\_LegnoScuro",
}


def tex(s):
    s = str(s)
    for a, b in (("\\", "\\textbackslash{}"), ("&", "\\&"), ("%", "\\%"), ("$", "\\$"), ("#", "\\#"),
                 ("_", "\\_"), ("{", "\\{"), ("}", "\\}"), ("~", "\\textasciitilde{}"), ("^", "\\^{}")):
        s = s.replace(a, b)
    return s.replace("\n", " / ")


def num(v):
    if isinstance(v, float) and v.is_integer():
        v = int(v)
    if isinstance(v, float):
        return ("%.2f" % v).rstrip("0").rstrip(".").replace(".", ",")
    return str(v)


def terna(t):
    return "(%s)" % "; ".join(num(x) for x in t)


# ---------------------------------------------------------------------
# moduli finti
# ---------------------------------------------------------------------

EVENTI = []          # tutto quello che lo script "piazza", in ordine


class Nome(str):
    """Un valore che si stampa con il suo nome (materiali, asset, enum)."""


class Finto(object):
    """Oggetto che accetta qualunque attributo o chiamata (per unreal.*)."""

    def __init__(self, nome="unreal"):
        self._nome = nome

    def __getattr__(self, k):
        if k.startswith("__"):
            raise AttributeError(k)
        return Finto(self._nome + "." + k)

    def __call__(self, *a, **k):
        return Finto(self._nome + "()")


class Abilita(object):
    def __getattr__(self, k):
        if k.startswith("__"):
            raise AttributeError(k)
        return Nome({"DOUBLE_JUMP": "DoppioSalto", "WALL_RUN": "WallRun", "JETPACK": "Jetpack",
                     "CRATE_SPAWNER": "GeneratoreCasse"}.get(k, k))


class Attore(object):
    def __init__(self, nome):
        self.nome = nome

    def get_component_by_class(self, _cls):
        return Componente(self)


class Componente(object):
    def __init__(self, attore):
        self.attore = attore

    def set_material(self, _i, m):
        registra("materiale_extra", attore=self.attore.nome, materiale=m)


def chiamante():
    """La riga di gd13_platformer.py dentro livello_N/menu e l'helper usato."""
    riga, helper = None, None
    for f in inspect.stack()[2:]:
        if os.path.basename(f.filename) != "gd13_platformer.py":
            continue
        if f.function in ("livello_1", "livello_2", "livello_3", "menu"):
            riga = f.lineno
            break
        helper = f.function
    return riga, helper


def registra(tipo, **dati):
    riga, helper = chiamante()
    dati.update(tipo=tipo, riga=riga, helper=helper)
    EVENTI.append(dati)


def crea_aiv_livelli():
    m = types.ModuleType("aiv_livelli")
    m.SFERA, m.CUBO, m.CILINDRO = "/Engine/BasicShapes/Sphere", "/Engine/BasicShapes/Cube", "/Engine/BasicShapes/Cylinder"
    m.TAG_WALLRUN = "WallRun"
    m.attori = Finto("attori")
    m.vec = lambda x, y, z: (x, y, z)
    m.rot = lambda pitch=0.0, yaw=0.0, roll=0.0: (pitch, yaw, roll)
    m.colore = lambda rgb, a=1.0: tuple(rgb)
    m.carica = lambda p: Nome(p.split("/")[-1])
    m.suono = lambda n: Nome(n)
    m.log = m.avviso = lambda *a, **k: None
    m.imposta = lambda *a, **k: True
    m.nomina = lambda att, nome, cartella: att
    m.salva = lambda: None
    m.sicuro = lambda f, *a, **k: f(*a, **k)

    def nuovo_livello(p):
        registra("livello", percorso=p)
        return True

    def luce_esterna(**k):
        registra("luce_esterna", valori=k)

    def post_process(nome="PostProcess", **k):
        registra("post_process", valori=k)

    def rendi_giocabile(pos, yaw_partenza=0.0, game_mode="PlatformerGameMode"):
        registra("partenza", pos=pos, game_mode=game_mode)

    def blocco(nome, cartella, centro, dimensioni, materiale=None, yaw=0.0, tinta="percorso", tag=None):
        registra("blocco", nome=nome, cartella=cartella, centro=centro, dim=dimensioni, materiale=materiale,
                 yaw=yaw, tinta=tinta, tag=tag)
        return Attore(nome)

    def pedana(nome, cartella, x, y, z_sopra, dx, dy, spessore=40.0, materiale=None, tinta="percorso", tag=None):
        registra("blocco", nome=nome, cartella=cartella, centro=(x, y, z_sopra - spessore / 2.0),
                 dim=(dx, dy, spessore), materiale=materiale, yaw=0.0, tinta=tinta, tag=tag, sopra=z_sopra)
        return Attore(nome)

    def mesh(percorso, nome, cartella, pos, yaw=0.0, scala=1.0, materiale=None, pitch=0.0, roll=0.0,
             ombre=True, collisione=True):
        registra("mesh", nome=nome, cartella=cartella, mesh=percorso.split("/")[-1].split(" ")[0], pos=pos,
                 yaw=yaw, scala=scala, materiale=materiale, ombre=ombre, collisione=collisione)
        return Attore(nome)

    def scritta(testo, pos, yaw=0.0, dimensione=60.0, colore_testo=(1.0, 1.0, 1.0), cartella="Scritte"):
        registra("scritta", testo=testo, pos=pos, yaw=yaw, dim=dimensione, colore=colore_testo)
        return Attore("Scritta")

    def spawn_cpp(classe, nome, cartella, pos, yaw=0.0, **prop):
        registra("cpp", classe=classe, nome=nome, cartella=cartella, pos=pos, yaw=yaw, prop=prop)
        return Attore(nome)

    def collega(att, proprieta, lista):
        if att is not None:
            registra("collega", attore=att.nome, proprieta=proprieta, lista=[x.nome for x in lista if x])

    def spline_punti(att, punti, chiusa=False):
        registra("spline", attore=att.nome, punti=punti)

    def luce_punto(nome, pos, intensita=20.0, temperatura=None, rgb=None, raggio=1000.0, sorgente=5.0,
                   ombre=True, cartella="Luci", volumetrica=1.0):
        registra("luce", genere="Point Light", nome=nome, pos=pos, intensita=intensita, rgb=rgb, raggio=raggio,
                 ombre=ombre)
        return Attore(nome)

    def luce_spot(nome, pos, pitch, yaw, intensita=50.0, temperatura=None, rgb=None, raggio=1500.0,
                  cono_esterno=40.0, cono_interno=20.0, sorgente=5.0, ombre=True, cartella="Luci", volumetrica=1.0):
        registra("luce", genere="Spot Light", nome=nome, pos=pos, pitch=pitch, yaw=yaw, intensita=intensita,
                 rgb=rgb, raggio=raggio, coni=(cono_interno, cono_esterno), ombre=ombre)
        return Attore(nome)

    def decal(nome, materiale, pos, dimensioni=(64, 200, 200), pitch=-90.0, yaw=0.0, cartella="Decal"):
        registra("decal", nome=nome, materiale=materiale, pos=pos, dim=dimensioni, pitch=pitch, yaw=yaw)
        return Attore(nome)

    for f in (nuovo_livello, luce_esterna, post_process, rendi_giocabile, blocco, pedana, mesh, scritta,
              spawn_cpp, collega, spline_punti, luce_punto, luce_spot, decal):
        setattr(m, f.__name__, f)
    return m


class Materiali(dict):
    def __missing__(self, k):
        return Nome("MI_AIV_" + k)


def esegui_script():
    fake = Finto()
    fake.PlatformerAbility = Abilita()
    unreal = types.ModuleType("unreal")
    unreal.__getattr__ = lambda k: getattr(fake, k)  # noqa
    unreal.PlatformerAbility = Abilita()
    sys.modules["unreal"] = unreal
    sys.modules["aiv_livelli"] = crea_aiv_livelli()
    mat = types.ModuleType("aiv_materiali")
    mat.tutti = lambda ricostruisci=False: Materiali()
    sys.modules["aiv_materiali"] = mat
    sys.path.insert(0, os.path.dirname(SCRIPT))
    import gd13_platformer as gd  # noqa
    gd.M = Materiali()
    for k in ("livello_1", "livello_2", "livello_3"):
        getattr(gd, k).__globals__["M"] = gd.M
    livelli = []
    for f in (gd.livello_1, gd.livello_2, gd.livello_3):
        inizio = len(EVENTI)
        f()
        livelli.append((f.__name__, EVENTI[inizio:]))
    return gd, livelli


# ---------------------------------------------------------------------
# sezioni e note dai commenti dello script
# ---------------------------------------------------------------------

def commenti(funzione):
    righe, primo = inspect.getsourcelines(funzione)
    sezioni, note = [], {}
    nota = []
    for i, r in enumerate(righe):
        n = primo + i
        t = r.strip()
        if t.startswith("# ---"):
            titolo = t.strip("#- ").strip()
            if titolo:
                sezioni.append((n, titolo))
            nota = []
        elif t.startswith("#"):
            nota.append(t.lstrip("# ").strip())
            note[n] = " ".join(nota)
        else:
            nota = []
    # una nota vale per la prima riga di codice che la segue
    note_per_riga = {}
    ultima = None
    for i, r in enumerate(righe):
        n = primo + i
        t = r.strip()
        if n in note:
            ultima = note[n]
        elif t and not t.startswith("#") and ultima:
            note_per_riga[n] = ultima
            ultima = None
    return sezioni, note_per_riga


# ---------------------------------------------------------------------
# formattazione delle righe
# ---------------------------------------------------------------------

def nome_variabile(k):
    return "".join(p.capitalize() for p in k.split("_"))


def valore(v, bp_livelli):
    if isinstance(v, bool):
        return "sì" if v else "no"
    if isinstance(v, (int, float)):
        return num(v)
    if isinstance(v, tuple):
        return terna(v)
    if isinstance(v, list):
        return "[" + ", ".join(valore(x, bp_livelli) for x in v) + "]"
    if v is None:
        return "--"
    s = str(v)
    if s.startswith("/Game/Maps/"):
        s = s.split("/")[-1]
        return bp_livelli.get(s, s)
    return s


def materiale(e):
    if e.get("materiale"):
        return str(e["materiale"])
    return TINTE.get(e.get("tinta") or "percorso").replace("\\_", "_")


def righe_livello(gd, funzione, eventi, bp_livelli):
    sezioni, note = commenti(funzione)
    collegamenti = {}
    for e in eventi:
        if e["tipo"] == "collega":
            collegamenti.setdefault(e["attore"], []).append("%s = [%s]" % (
                nome_variabile(e["proprieta"]), ", ".join(e["lista"])))
        elif e["tipo"] == "spline":
            pts = "; ".join(terna(p) for p in e["punti"])
            collegamenti.setdefault(e["attore"], []).append("punti della spline (locali): " + pts)
        elif e["tipo"] == "materiale_extra":
            collegamenti.setdefault(e["attore"], []).append("Material (mesh) = " + str(e["materiale"]))
    setup, gruppi = [], [["Partenza e primo tratto", []]]
    note_usate = set()
    for e in eventi:
        r = e["riga"] or 0
        prima = [titolo for n, titolo in sezioni if n <= r]
        if prima and prima[-1] != gruppi[-1][0]:
            gruppi.append([prima[-1], []])
        riga = None
        t = e["tipo"]
        if t in ("livello", "luce_esterna", "post_process", "partenza"):
            setup.append(e)
            continue
        if t in ("collega", "spline", "materiale_extra"):
            continue
        nota = note.get(r) if r not in note_usate else None
        if nota:
            note_usate.add(r)
        if t == "blocco":
            cosa = "Cubo"
            if e.get("sopra") is not None:
                cosa = "Pedana"
            imp = [materiale(e)]
            if e.get("tag"):
                imp.append("Tag: " + e["tag"])
            if e.get("sopra") is not None:
                imp.append("si cammina a Z = " + num(e["sopra"]))
            riga = (cosa, e["nome"], terna(e["centro"]), terna(e["dim"]) + (" yaw %s" % num(e["yaw"]) if e["yaw"] else ""),
                    ", ".join(imp))
        elif t == "mesh":
            sc = e["scala"] if isinstance(e["scala"], (list, tuple)) else (e["scala"],) * 3
            imp = []
            if e["materiale"]:
                imp.append(str(e["materiale"]))
            if not e["ombre"]:
                imp.append("Cast Shadow no")
            if not e["collisione"]:
                imp.append("Collision: No Collision")
            riga = ("Static Mesh", e["nome"], terna(e["pos"]), "scala " + terna(sc) + (" yaw %s" % num(e["yaw"]) if e["yaw"] else ""),
                    e["mesh"] + ("; " + ", ".join(imp) if imp else ""))
        elif t == "scritta":
            riga = ("Text Render", "Scritta", terna(e["pos"]), "yaw %s" % num(e["yaw"]),
                    "``%s'', World Size %s, colore %s" % (e["testo"].replace("\n", " / "), num(e["dim"]), terna(e["colore"])))
        elif t == "decal":
            riga = ("Decal", e["nome"], terna(e["pos"]), "rot (%s; %s; 0), Decal Size %s" % (
                num(e["pitch"]), num(e["yaw"]), terna(e["dim"])), str(e["materiale"]))
        elif t == "luce":
            rot = ("pitch %s yaw %s" % (num(e["pitch"]), num(e["yaw"]))) if e["genere"] == "Spot Light" else "--"
            imp = ["%s cd" % num(e["intensita"]), "raggio %s" % num(e["raggio"])]
            if e.get("rgb"):
                imp.append("colore " + terna(e["rgb"]))
            if e.get("coni"):
                imp.append("coni %s/%s°" % (num(e["coni"][0]), num(e["coni"][1])))
            if not e["ombre"]:
                imp.append("niente ombre")
            riga = (e["genere"], e["nome"], terna(e["pos"]), rot, ", ".join(imp))
        elif t == "cpp":
            bp = gd.BLUEPRINT.get(e["classe"], "BP_" + e["classe"])
            imp = []
            for k, v in e["prop"].items():
                if k in ("label", "prompt", "message") and v == "":
                    continue
                vv = valore(v, bp_livelli)
                if k in ("label", "prompt", "message"):
                    vv = "``" + vv.replace("\n", " / ") + "''"
                imp.append("%s = %s" % (nome_variabile(k), vv))
            imp += collegamenti.get(e["nome"], [])
            riga = (bp, e["nome"], terna(e["pos"]), ("yaw %s" % num(e["yaw"])) if e["yaw"] else "--", "; ".join(imp))
        if riga:
            gruppi[-1][1].append((riga, nota, e.get("helper")))
    return setup, [g for g in gruppi if g[1]]


def scrivi_livello(gd, idx, nome_f, eventi, bp_livelli):
    funzione = getattr(gd, nome_f)
    setup, gruppi = righe_livello(gd, funzione, eventi, bp_livelli)
    out = []
    out.append("%% generato da strumenti/estrai_livelli.py: non modificare a mano\n")
    # impostazioni del livello
    le = [e for e in setup if e["tipo"] == "luce_esterna"]
    pp = [e for e in setup if e["tipo"] == "post_process"]
    ps = [e for e in setup if e["tipo"] == "partenza"]
    out.append("\\subsection{Luce, atmosfera e partenza}\n")
    out.append("\\begin{itemize}\n")
    if le:
        v = dict(sole_pitch=-40.0, sole_yaw=-30.0, sole_lux=8.0, temperatura=5800.0, cielo_intensita=1.0,
                 nebbia_densita=0.02, nebbia_volumetrica=False, luna=False)
        v.update(le[0]["valori"])
        out.append("\\item \\nodo{Directional Light} (%s): Mobility \\emph{Movable}, Rotation pitch %s, yaw %s; "
                   "Intensity %s lux; \\emph{Use Temperature} sì, Temperature %s K; \\emph{Atmosphere Sun Light} sì; "
                   "Source Angle %s.\n" % ("la luna" if v["luna"] else "il sole", num(v["sole_pitch"]), num(v["sole_yaw"]),
                                           num(v["sole_lux"]), num(v["temperatura"]), "0,5" if v["luna"] else "1"))
        out.append("\\item \\nodo{Sky Atmosphere} (valori di default).\n")
        out.append("\\item \\nodo{Sky Light}: Movable, \\emph{Real Time Capture} sì, Intensity %s.\n" % num(v["cielo_intensita"]))
        out.append("\\item \\nodo{Exponential Height Fog} a Z $-200$: Fog Density %s%s.\n" % (
            num(v["nebbia_densita"]), ", \\emph{Volumetric Fog} sì" if v["nebbia_volumetrica"] else ""))
    if pp:
        vals = ", ".join("%s %s" % (nome_variabile(k).replace("AutoExposure", "Auto Exposure ").replace("Intensity", " Intensity"),
                                    num(x)) for k, x in pp[0]["valori"].items())
        out.append("\\item \\nodo{Post Process Volume}: \\emph{Infinite Extent (Unbound)} sì; spuntate e impostate %s.\n" % tex(vals))
    if ps:
        out.append("\\item \\nodo{Player Start} in %s, rotazione 0.\n" % terna(ps[0]["pos"]))
    out.append("\\end{itemize}\n\n")
    n = 0
    for titolo, righe in gruppi:
        if True:
            t = titolo[0].upper() + titolo[1:]
            out.append("\\subsection{%s}\n" % tex(t).replace("->", "$\\rightarrow$"))
        out.append("\\begin{oggetti}\n")
        for (cosa, nome, pos, mis, imp), nota, _h in righe:
            if nota:
                out.append("\\nota{%s}\n" % tex(nota))
            n += 1
            cella = lambda x: tex(x).replace("\\_", "\\_\\allowbreak{}")
            out.append("%d & %s & %s & %s & %s & %s \\\\\n" % (
                n, cella(cosa), cella(nome), tex(pos), tex(mis), cella(imp)))
        out.append("\\end{oggetti}\n\n")
    with open(os.path.join(USCITA, "gen_livello%d.tex" % idx), "w", encoding="utf-8") as f:
        f.write("".join(out))
    return n


# ---------------------------------------------------------------------
# catalogo dei Blueprint dagli header C++
# ---------------------------------------------------------------------

TIPI = [
    (r"^float$", "Float"), (r"^double$", "Float"), (r"^int32$", "Integer"), (r"^bool$", "Boolean"),
    (r"^FVector$", "Vector"), (r"^FRotator$", "Rotator"), (r"^FName$", "Name"), (r"^FText$", "Text"),
    (r"^FString$", "String"), (r"^FLinearColor$", "Linear Color"), (r"^FColor$", "Color"),
    (r"EPlatformerAbility", "E\\_Abilita"), (r"ECollisionChannel", "Collision Channel"),
    (r"^TArray<TObjectPtr<U?A?(\w+)>>$", "Array di \\1"), (r"^TArray<(\w+)>$", "Array di \\1"),
    (r"^TObjectPtr<U(\w+)>$", "\\1 (riferimento)"), (r"^TObjectPtr<A(\w+)>$", "\\1 (riferimento)"),
    (r"^TSoftObjectPtr<U(\w+)>$", "\\1 (riferimento)"), (r"^TSubclassOf<A(\w+)>$", "Classe di \\1"),
]


def tipo_bp(t):
    t = t.strip()
    for pat, rep in TIPI:
        if re.search(pat, t):
            return re.sub(pat, rep, t) if "\\1" in rep else rep
    return t


def leggi_classi():
    classi = {}
    for nome_file in sorted(os.listdir(HEADERS)):
        if not nome_file.endswith(".h"):
            continue
        testo = open(os.path.join(HEADERS, nome_file), encoding="utf-8").read()
        for m in re.finditer(r"class AIV_UE5INTRO_API ([AU]\w+) : public (\w+)", testo):
            nome, padre = m.group(1)[1:], m.group(2)[1:]
            inizio = m.end()
            fine = testo.find("\nclass AIV_UE5INTRO_API", inizio)
            corpo = testo[inizio:fine if fine > 0 else len(testo)]
            props = []
            for p in re.finditer(r"(?:/\*\*((?:(?!\*/).)*)\*/\s*)?UPROPERTY\((.*?)\)[ \t]*\n\s*([^;=\n{}]+?)\s+(\w+)\s*(?:=\s*([^;\n]+))?;",
                                 corpo, re.S):
                spec = p.group(2)
                if "EditAnywhere" not in spec and "EditInstanceOnly" not in spec:
                    continue
                if "BlueprintAssignable" in spec:
                    continue
                desc = re.sub(r"\s*\*\s*", " ", (p.group(1) or "")).strip()
                tipo = p.group(3).strip()
                var = p.group(4)
                if tipo == "bool" and var.startswith("b") and var[1:2].isupper():
                    var = var[1:]
                default = (p.group(5) or "").strip()
                default = re.sub(r"FName\(TEXT\(\"(.*)\"\)\)", r"\1", default)
                default = re.sub(r"FText::FromString\(TEXT\(\"(.*)\"\)\)", r"\1", default)
                default = re.sub(r"TEXT\(\"(.*)\"\)", r"\1", default)
                default = re.sub(r"FVector\((.*)\)", r"(\1)", default).replace(".f", "").replace("f,", ",").replace("f)", ")")
                default = re.sub(r"(\d)\.?f$", r"\1", default)
                default = re.sub(r"EPlatformerAbility::(\w+)", r"\1", default)
                default = re.sub(r"^F(LinearColor|Rotator)\((.*)\)$", r"(\2)", default)
                default = {"true": "sì", "false": "no"}.get(default, default)
                props.append((var, tipo_bp(tipo), default, desc))
            classi[nome] = (padre, props, nome_file)
    return classi


def scrivi_catalogo(gd, classi, descrizioni):
    usati = list(gd.BLUEPRINT.items())
    out = ["%% generato da strumenti/estrai_livelli.py: non modificare a mano\n"]
    padri_bp = {"PlatformerZone": "BP_Zona", "PlatformerPickup": "BP_Pickup", "SwitchBase": "BP_Interruttore",
                "Activable": "BP_Attivabile"}
    fatti = set()

    def blocco_classe(cpp, bp):
        padre, props, file_h = classi.get(cpp, ("Actor", [], "?"))
        padre_bp = padri_bp.get(padre, "Actor")
        d = descrizioni.get(bp, {})
        out.append("\\subsection{\\texttt{%s}}\\label{bp:%s}\n" % (tex(bp), bp))
        out.append("\\noindent Rifà \\texttt{%s} (\\texttt{%s}). Classe padre: \\nodo{%s}.\\par\n" % (
            tex("A" + cpp if cpp not in ("PlatformerZone", "PlatformerPickup", "SwitchBase", "Activable") else "A" + cpp),
            tex(file_h), tex(padre_bp)))
        if d.get("componenti"):
            out.append("\\smallskip\\noindent\\textbf{Componenti.} %s\\par\n" % d["componenti"])
        if d.get("origine"):
            out.append("\\noindent\\textbf{Origine (pivot).} %s\\par\n" % d["origine"])
        if props:
            out.append("\\smallskip\\noindent\\textbf{Variabili} (tutte \\emph{Instance Editable}):\n")
            out.append("\\begin{variabili}\n")
            for var, tipo, default, desc in props:
                out.append("%s & %s & %s & %s \\\\\n" % (tex(var), tipo, tex(default), tex(desc)))
            out.append("\\end{variabili}\n")
        if padre_bp != "Actor":
            out.append("\\noindent In più eredita le variabili di \\texttt{%s} (sezione~\\ref{bp:%s}).\\par\n" % (
                tex(padre_bp), padre_bp))
        if d.get("logica"):
            out.append("\\smallskip\\noindent\\textbf{Logica.} %s\\par\n" % d["logica"])
        out.append("\n")

    for cpp, bp in (("PlatformerZone", "BP_Zona"), ("PlatformerPickup", "BP_Pickup"),
                    ("SwitchBase", "BP_Interruttore"), ("Activable", "BP_Attivabile")):
        blocco_classe(cpp, bp)
    for cpp, bp in usati:
        if bp in fatti:
            continue
        fatti.add(bp)
        blocco_classe(cpp, bp)
    with open(os.path.join(USCITA, "gen_catalogo.tex"), "w", encoding="utf-8") as f:
        f.write("".join(out))


def main():
    gd, livelli = esegui_script()
    bp_livelli = {"L_GD13_Livello1": "L_GD13_Livello1_BP", "L_GD13_Livello2": "L_GD13_Livello2_BP",
                  "L_GD13_Livello3": "L_GD13_Livello3_BP", "L_GD13_Menu": "L_GD13_Menu"}
    totale = 0
    for i, (nome_f, eventi) in enumerate(livelli, 1):
        totale += scrivi_livello(gd, i, nome_f, eventi, bp_livelli)
    sys.path.insert(0, QUI)
    from descrizioni_bp import DESCRIZIONI  # noqa
    scrivi_catalogo(gd, leggi_classi(), DESCRIZIONI)
    usati = {}
    for _n, eventi in livelli:
        for e in eventi:
            if e["tipo"] == "cpp":
                usati[e["classe"]] = usati.get(e["classe"], 0) + 1
    conti = {}
    for i, (_n, eventi) in enumerate(livelli):
        for e in eventi:
            if e["tipo"] == "cpp":
                bp = gd.BLUEPRINT.get(e["classe"], "BP_" + e["classe"])
                conti.setdefault(bp, [0, 0, 0])[i] += 1
    righe = ["%% generato da strumenti/estrai_livelli.py: non modificare a mano\n"]
    for bp in sorted(conti):
        c = conti[bp]
        righe.append("\\texttt{%s} & %s & %s & %s & \\ref{bp:%s} \\\\\n" % (
            tex(bp), *(str(x) if x else "--" for x in c), bp))
    righe.insert(1, "\\begin{longtable}{lcccl}\n\\toprule\nBlueprint & Livello 1 & Livello 2 & Livello 3 & Sezione \\\\\n\\midrule\\endhead\n")
    righe.append("\\bottomrule\n\\end{longtable}\n")
    with open(os.path.join(USCITA, "gen_uso.tex"), "w", encoding="utf-8") as f:
        f.write("".join(righe))
    print("oggetti nelle tabelle:", totale)
    print("Blueprint usati:", ", ".join("%s x%d" % (gd.BLUEPRINT[k], v) for k, v in sorted(usati.items())))


if __name__ == "__main__":
    main()
