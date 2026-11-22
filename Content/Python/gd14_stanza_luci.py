# -*- coding: utf-8 -*-
"""
GD14 - Task di Level Design: "una stanza illuminata con uno scopo".

Costruisce da zero, dentro l'editor di Unreal, un nuovo livello
(/Game/Maps/L_GD14_StanzaLuci) con:
  * la stanza fatta di geometria (cubi dell'engine), con una finestra
    e una porta ricavate lasciando dei buchi nei muri;
  * i prop del pacchetto Scan Labs (poltrona, lampada, libri, quadri,
    scatole, piante, frutta, scarpe...), disposti con un "pizzico di caos";
  * le luci, ognuna con uno scopo preciso (vedi Docs/GD14_stanza_luci.md);
  * tre CineCamera per i tre screenshot della consegna.

COME SI LANCIA
  1. Abilitare i plugin "Python Editor Script Plugin" e "Editor Scripting
     Utilities" (sono già aggiunti nel .uproject: basta riavviare l'editor).
  2. In Unreal: Tools > Execute Python Script... e scegliere questo file,
     oppure nella Output Log, riga di comando "Python":
         import gd14_stanza_luci; gd14_stanza_luci.costruisci()
     (la cartella Content/Python è già nel path di Unreal).
  3. Lo script crea il livello, lo popola e lo salva. Si può rilanciare:
     ricrea il livello da capo (le modifiche fatte a mano in quel livello
     si perdono: per lavorarci a mano, salvatelo prima con un altro nome).

DOVE MODIFICARE
  Tutto ciò che conta sta nelle COSTANTI qui sotto (misure della stanza,
  colori e intensità delle luci, posizione e rotazione dei prop). Le
  funzioni in fondo sono solo "attrezzi" e di solito non vanno toccate.

Unità: centimetri (come in Unreal). Assi: X avanti, Y destra, Z alto.
Angoli in gradi.
"""

import math
import random

import unreal

# =====================================================================
#  1. PARAMETRI GENERALI
# =====================================================================

LIVELLO = "/Game/Maps/L_GD14_StanzaLuci"

# Stanza: interno da (0,0) a (LUNGHEZZA, LARGHEZZA), pavimento a z = 0.
LUNGHEZZA = 700.0      # lungo X
LARGHEZZA = 520.0      # lungo Y
ALTEZZA = 300.0
SPESSORE = 20.0        # spessore di muri, pavimento e soffitto

# Finestra nel muro OVEST (y = 0): da qui entra il sole.
FINESTRA_X = (180.0, 520.0)   # da x a x
FINESTRA_Z = (70.0, 250.0)    # dal davanzale all'architrave

# Porta nel muro NORD (x = LUNGHEZZA): dà su un corridoio buio.
PORTA_Y = (330.0, 430.0)
PORTA_ALTEZZA = 220.0
CORRIDOIO_PROFONDITA = 260.0

# Brise-soleil: listelli verticali fuori dalla finestra. Spezzano il sole
# in strisce di luce e ombra sul pavimento (il "ritmo" di cui parla Totten).
LISTELLI_NUMERO = 7
LISTELLI_LARGHEZZA = 9.0
LISTELLI_DISTANZA_DAL_MURO = 45.0

SEME_CAOS = 14         # cambia il numero per un "disordine" diverso

# Materiali (Scan Labs). Se nel progetto c'è lo Starter Content, il
# pavimento usa il parquet dello Starter Content; altrimenti il quarzo.
MAT_MURI = "/Game/Materials/MI_Room_Walls"
MAT_PAVIMENTO = ["/Game/StarterContent/Materials/M_Wood_Floor_Walnut_Polished",
                 "/Game/Materials/MI_Quartz_Floor"]
MAT_LEGNO = "/Game/Materials/MI_System_Wood"
MAT_ESTERNO = "/Game/Materials/MI_Ground"

CUBO = "/Engine/BasicShapes/Cube"        # cubo da 100 cm, pivot al centro

# =====================================================================
#  2. LE LUCI (lo scopo di ciascuna è spiegato nella documentazione)
# =====================================================================

# Sole di tardo pomeriggio: basso, caldo, entra dalla finestra a ovest.
SOLE_PITCH = -22.0           # quanto è alto sull'orizzonte (negativo = verso il basso)
SOLE_YAW = 78.0              # 90 = punta dritto verso +Y (dentro la stanza)
SOLE_INTENSITA_LUX = 9.0
SOLE_TEMPERATURA_K = 3300.0

# Sky Light: il colore delle ombre. Leggermente fredda, per contrasto.
CIELO_COLORE = (0.80, 0.88, 1.0)
CIELO_INTENSITA = 0.85

# Lampada da lettura: point light SOPRA e DAVANTI al paralume, non dentro,
# e il paralume non proietta ombre (la "lampada senza ombra" di Lies of P).
LAMPADA_INTENSITA_CD = 22.0
LAMPADA_TEMPERATURA_K = 2600.0
LAMPADA_RAGGIO = 520.0

# Luce di rinforzo sulla libreria: spot debole, SENZA ombre. Rende leggibili
# i libri sul muro est, che altrimenti sarebbe tutto in ombra.
RINFORZO_INTENSITA_CD = 7.0
RINFORZO_TEMPERATURA_K = 3800.0

# Luce del corridoio: rect light fredda oltre la porta. Segnala l'uscita
# (la luce agli incroci di Alexander) e dà profondità all'inquadratura.
CORRIDOIO_INTENSITA_CD = 5.0
CORRIDOIO_COLORE = (0.70, 0.82, 1.0)

# =====================================================================
#  3. I PROP
#  (mesh, x, y, yaw, scala, cartella)
#  x, y = centro dell'oggetto in pianta; l'oggetto viene appoggiato a
#  terra da solo (oppure sulla quota "z" se indicata con la chiave z=).
#  Se un oggetto risulta girato al contrario, cambiate il suo yaw
#  di 90 o 180 gradi: è l'unica cosa che lo script non può sapere.
# =====================================================================

SCANLABS = "/Game/Meshes/"

PROP = [
    # --- angolo lettura, vicino alla finestra ---------------------------
    dict(mesh=SCANLABS + "Upholstery/SM_Couch_01_Single", x=170, y=170, yaw=-135,
         nome="Poltrona", cartella="Prop/AngoloLettura"),
    dict(mesh=SCANLABS + "Decor/SM_Lamp_Pear_Stand", x=90, y=100, yaw=30,
         nome="Lampada_Lettura", cartella="Prop/AngoloLettura", ombra=False),
    dict(mesh=SCANLABS + "Decor/SM_Shoe_Boat", x=235, y=230, yaw=12,
         nome="Scarpa_1", cartella="Prop/AngoloLettura"),
    dict(mesh=SCANLABS + "Decor/SM_Shoe_Boat", x=255, y=200, yaw=-31,
         nome="Scarpa_2", cartella="Prop/AngoloLettura"),
    dict(mesh=SCANLABS + "Books/SM_Book_012", x=225, y=150, yaw=63,
         nome="Libro_aperto_a_terra", cartella="Prop/AngoloLettura"),

    # --- tavolino (costruito con i cubi) e sopra la frutta --------------
    dict(mesh=SCANLABS + "Fruits/SM_FruitsOnPlate", x=310, y=150, yaw=20,
         z="TAVOLINO", nome="Frutta", cartella="Prop/Tavolino"),
    dict(mesh=SCANLABS + "Decor/SM_Glas_Wine_A", x=280, y=185, yaw=0,
         z="TAVOLINO", nome="Bicchiere", cartella="Prop/Tavolino"),

    # --- muro sud (dietro la telecamera principale): piante e scatole ----
    dict(mesh=SCANLABS + "Decor/SM_Groundplant_12", x=60, y=455, yaw=0,
         nome="Pianta", cartella="Prop/Piante"),
    dict(mesh=SCANLABS + "Decor/SM_Vase_Flower", x=640, y=60, yaw=0,
         nome="Vaso", cartella="Prop/Piante"),

    # --- scatole del trasloco vicino alla porta: il pizzico di caos -----
    dict(mesh=SCANLABS + "Decor/SM_Box_A", x=600, y=230, yaw=7,
         nome="Scatola_A", cartella="Prop/Scatole"),
    dict(mesh=SCANLABS + "Decor/SM_Box_B", x=605, y=150, yaw=-11,
         nome="Scatola_B", cartella="Prop/Scatole"),
    dict(mesh=SCANLABS + "Decor/SM_Box_C", x=600, y=230, yaw=-4,
         z="SOPRA:Scatola_A", nome="Scatola_C", cartella="Prop/Scatole"),

    # --- dettagli -------------------------------------------------------
    dict(mesh=SCANLABS + "Decor/SM_Buddha", x=640, y=470, yaw=-90,
         nome="Statuetta", cartella="Prop/Dettagli"),
    dict(mesh=SCANLABS + "Decor/SM_Deco_Bean", x=330, y=120, yaw=40,
         z="TAVOLINO", nome="Soprammobile", cartella="Prop/Tavolino"),
]

# Quadri appesi: (mesh, muro, posizione lungo il muro, altezza del centro)
# muro = "sud" (x=0), "est" (y=LARGHEZZA), "nord" (x=LUNGHEZZA)
QUADRI = [
    dict(mesh=SCANLABS + "Decor/SM_Picture_A", muro="sud", lungo=300, z=165, nome="Quadro_A"),
    dict(mesh=SCANLABS + "Decor/SM_Picture_C", muro="nord", lungo=160, z=160, nome="Quadro_C"),
]
PRESA = dict(mesh=SCANLABS + "Decor/SM_Plug_Socket", muro="sud", lungo=120, z=30, nome="Presa")

# Libreria costruita con i cubi contro il muro EST, riempita di libri.
LIBRERIA_CENTRO_X = 330.0
LIBRERIA_LARGHEZZA = 260.0
LIBRERIA_PROFONDITA = 35.0
LIBRERIA_RIPIANI_Z = [12.0, 60.0, 108.0, 156.0, 204.0]   # quote dei ripiani
LIBRI_GRUPPI = [SCANLABS + "Books/SM_Books_Mix_%d" % i for i in range(1, 9)]

# Tavolino: piano a questa quota, costruito con 5 cubi.
TAVOLINO = dict(x=305, y=150, lato_x=80, lato_y=60, altezza=45)

# =====================================================================
#  4. LE TELECAMERE PER GLI SCREENSHOT
#  (posizione, punto guardato, lunghezza focale in mm)
# =====================================================================

TELECAMERE = [
    dict(nome="Cam_1_Insieme", pos=(40, 500, 170), guarda=(450, 120, 90), focale=16.0),
    dict(nome="Cam_2_AngoloLettura", pos=(380, 330, 140), guarda=(150, 140, 70), focale=24.0),
    dict(nome="Cam_3_LibreriaEPorta", pos=(160, 120, 150), guarda=(520, 480, 110), focale=18.0),
]


# =====================================================================
#  ATTREZZI (di solito non serve modificarli)
# =====================================================================

attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
livelli = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
_creati = {}   # nome -> attore, per riferirsi agli oggetti già piazzati


def log(msg):
    unreal.log("[GD14] " + msg)


def avviso(msg):
    unreal.log_warning("[GD14] " + msg)


def carica(percorso):
    """Carica un asset; se manca, avvisa e restituisce None (lo script continua)."""
    if isinstance(percorso, (list, tuple)):
        for p in percorso:
            if unreal.EditorAssetLibrary.does_asset_exist(p):
                return unreal.EditorAssetLibrary.load_asset(p)
        avviso("nessuno di questi asset esiste: %s" % (percorso,))
        return None
    if not unreal.EditorAssetLibrary.does_asset_exist(percorso):
        avviso("asset mancante: " + percorso)
        return None
    return unreal.EditorAssetLibrary.load_asset(percorso)


def imposta(oggetto, proprieta, valore):
    """set_editor_property che non blocca lo script se una proprietà cambia nome."""
    try:
        oggetto.set_editor_property(proprieta, valore)
        return True
    except Exception as e:  # noqa: BLE001
        avviso("non riesco a impostare %s.%s (%s)" % (oggetto.get_name(), proprieta, e))
        return False


def rot(pitch=0.0, yaw=0.0, roll=0.0):
    return unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw)


def vec(x, y, z):
    return unreal.Vector(float(x), float(y), float(z))


def colore(rgb):
    return unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0)


def etichetta(attore, nome, cartella):
    attore.set_actor_label(nome)
    attore.set_folder_path(cartella)
    _creati[nome] = attore


def ingombro(mesh, yaw, scala):
    """Bounding box della mesh ruotata di yaw e scalata, con l'attore nell'origine.
    Restituisce (minx, maxx, miny, maxy, minz, maxz)."""
    box = mesh.get_bounding_box()
    a = math.radians(yaw)
    c, s = math.cos(a), math.sin(a)
    xs, ys = [], []
    for px in (box.min.x, box.max.x):
        for py in (box.min.y, box.max.y):
            px2, py2 = px * scala[0], py * scala[1]
            xs.append(px2 * c - py2 * s)
            ys.append(px2 * s + py2 * c)
    return (min(xs), max(xs), min(ys), max(ys),
            box.min.z * scala[2], box.max.z * scala[2])


def piazza_mesh(mesh, nome, cartella, yaw=0.0, scala=(1, 1, 1),
                x=None, y=None, xmin=None, xmax=None, ymin=None, ymax=None,
                zmin=0.0, ombra=True, materiale=None):
    """Piazza una static mesh scegliendo DOVE va il suo ingombro, non il pivot.
    Per ogni asse si dà o il centro (x / y) o un bordo (xmin, xmax, ...).
    In verticale si dà la quota su cui appoggiarla (zmin)."""
    if mesh is None:
        return None
    b = ingombro(mesh, yaw, scala)
    if x is not None:
        ax = x - (b[0] + b[1]) / 2.0
    elif xmin is not None:
        ax = xmin - b[0]
    elif xmax is not None:
        ax = xmax - b[1]
    else:
        ax = 0.0
    if y is not None:
        ay = y - (b[2] + b[3]) / 2.0
    elif ymin is not None:
        ay = ymin - b[2]
    elif ymax is not None:
        ay = ymax - b[3]
    else:
        ay = 0.0
    az = zmin - b[4]
    attore = attori.spawn_actor_from_object(mesh, vec(ax, ay, az), rot(yaw=yaw))
    attore.set_actor_scale3d(vec(*scala))
    comp = attore.static_mesh_component
    if not ombra:
        comp.set_cast_shadow(False)
    if materiale is not None:
        for i in range(comp.get_num_materials()):
            comp.set_material(i, materiale)
    etichetta(attore, nome, cartella)
    return attore


def blocco(nome, cartella, x0, x1, y0, y1, z0, z1, materiale):
    """Un parallelepipedo da (x0,y0,z0) a (x1,y1,z1) fatto con il cubo dell'engine."""
    cubo = carica(CUBO)
    return piazza_mesh(cubo, nome, cartella,
                       scala=((x1 - x0) / 100.0, (y1 - y0) / 100.0, (z1 - z0) / 100.0),
                       x=(x0 + x1) / 2.0, y=(y0 + y1) / 2.0, zmin=z0,
                       materiale=materiale)


def quota_superiore(attore):
    origine, estensione = attore.get_actor_bounds(False)
    return origine.z + estensione.z


def spawn_luce(classe, nome, posizione, rotazione):
    attore = attori.spawn_actor_from_class(classe, posizione, rotazione)
    etichetta(attore, nome, "Luci")
    return attore


def imposta_luce(comp, intensita=None, temperatura=None, rgb=None, ombre=True,
                 unita_candele=True):
    if unita_candele:
        imposta(comp, "intensity_units", unreal.LightUnits.CANDELAS)
    if intensita is not None:
        comp.set_intensity(intensita)
    if temperatura is not None:
        imposta(comp, "use_temperature", True)
        imposta(comp, "temperature", temperatura)
    if rgb is not None:
        comp.set_light_color(colore(rgb), True)
    comp.set_cast_shadows(ombre)


# =====================================================================
#  COSTRUZIONE
# =====================================================================

def costruisci_stanza():
    muri = carica(MAT_MURI)
    pav = carica(MAT_PAVIMENTO)
    legno = carica(MAT_LEGNO)
    L, W, H, S = LUNGHEZZA, LARGHEZZA, ALTEZZA, SPESSORE

    # pavimento e soffitto
    blocco("Pavimento", "Stanza", -S, L + S, -S, W + S, -S, 0.0, pav)
    blocco("Soffitto", "Stanza", -S, L + S, -S, W + S, H, H + S, muri)

    # muro SUD (x = 0) e muro EST (y = W): pieni
    blocco("Muro_Sud", "Stanza/Muri", -S, 0.0, -S, W + S, 0.0, H, muri)
    blocco("Muro_Est", "Stanza/Muri", 0.0, L, W, W + S, 0.0, H, muri)

    # muro OVEST (y = 0) con il buco della finestra: quattro pezzi
    fx0, fx1 = FINESTRA_X
    fz0, fz1 = FINESTRA_Z
    blocco("Muro_Ovest_Sinistra", "Stanza/Muri", 0.0, fx0, -S, 0.0, 0.0, H, muri)
    blocco("Muro_Ovest_Destra", "Stanza/Muri", fx1, L, -S, 0.0, 0.0, H, muri)
    blocco("Muro_Ovest_Sottofinestra", "Stanza/Muri", fx0, fx1, -S, 0.0, 0.0, fz0, muri)
    blocco("Muro_Ovest_Architrave", "Stanza/Muri", fx0, fx1, -S, 0.0, fz1, H, muri)
    # davanzale di legno
    blocco("Davanzale", "Stanza/Finestra", fx0 - 10, fx1 + 10, -S - 4, 18.0, fz0 - 4, fz0, legno)

    # muro NORD (x = L) con la porta: tre pezzi
    py0, py1 = PORTA_Y
    blocco("Muro_Nord_Sinistra", "Stanza/Muri", L, L + S, -S, py0, 0.0, H, muri)
    blocco("Muro_Nord_Destra", "Stanza/Muri", L, L + S, py1, W + S, 0.0, H, muri)
    blocco("Muro_Nord_Sopraporta", "Stanza/Muri", L, L + S, py0, py1, PORTA_ALTEZZA, H, muri)
    # stipiti di legno
    blocco("Stipite_1", "Stanza/Porta", L - 3, L + S + 3, py0 - 6, py0, 0.0, PORTA_ALTEZZA, legno)
    blocco("Stipite_2", "Stanza/Porta", L - 3, L + S + 3, py1, py1 + 6, 0.0, PORTA_ALTEZZA, legno)
    blocco("Stipite_Alto", "Stanza/Porta", L - 3, L + S + 3, py0 - 6, py1 + 6,
           PORTA_ALTEZZA, PORTA_ALTEZZA + 6, legno)

    # corridoio oltre la porta: una scatola chiusa
    cx0, cx1 = L + S, L + S + CORRIDOIO_PROFONDITA
    cy0, cy1 = py0 - 120.0, py1 + 120.0
    blocco("Corridoio_Pavimento", "Stanza/Corridoio", cx0, cx1, cy0, cy1, -S, 0.0, pav)
    blocco("Corridoio_Soffitto", "Stanza/Corridoio", cx0, cx1, cy0, cy1, H, H + S, muri)
    blocco("Corridoio_Fondo", "Stanza/Corridoio", cx1, cx1 + S, cy0, cy1, 0.0, H, muri)
    blocco("Corridoio_Lato_1", "Stanza/Corridoio", cx0, cx1, cy0 - S, cy0, 0.0, H, muri)
    blocco("Corridoio_Lato_2", "Stanza/Corridoio", cx0, cx1, cy1, cy1 + S, 0.0, H, muri)

    # battiscopa: un dettaglio che "chiude" i muri
    for nome, a in (("Battiscopa_Sud", (0, 2, 0, W)), ("Battiscopa_Est", (0, L, W - 2, W))):
        blocco(nome, "Stanza/Dettagli", a[0], a[1], a[2], a[3], 0.0, 9.0, legno)


def costruisci_finestra():
    """La mesh della finestra di Scan Labs, adattata al buco nel muro."""
    finestra = carica(SCANLABS + "Room/SM_Window")
    fx0, fx1 = FINESTRA_X
    fz0, fz1 = FINESTRA_Z
    if finestra is not None:
        box = finestra.get_bounding_box()
        dx, dy = box.max.x - box.min.x, box.max.y - box.min.y
        dz = box.max.z - box.min.z
        # il lato più sottile della mesh va messo perpendicolare al muro (asse Y)
        yaw = 90.0 if dx < dy else 0.0
        larghezza = max(dx, dy)
        scala_l = (fx1 - fx0) / larghezza
        scala_h = (fz1 - fz0) / dz
        scala = (scala_l, 1.0, scala_h) if yaw == 0.0 else (1.0, scala_l, scala_h)
        piazza_mesh(finestra, "Finestra", "Stanza/Finestra", yaw=yaw, scala=scala,
                    x=(fx0 + fx1) / 2.0, y=-SPESSORE / 2.0, zmin=fz0)

    # brise-soleil: listelli verticali fuori dalla finestra
    legno = carica(MAT_LEGNO)
    passo = (fx1 - fx0) / float(LISTELLI_NUMERO)
    yl = -SPESSORE - LISTELLI_DISTANZA_DAL_MURO
    for i in range(LISTELLI_NUMERO):
        xc = fx0 + passo * (i + 0.5)
        blocco("Listello_%02d" % (i + 1), "Esterno/BriseSoleil",
               xc - LISTELLI_LARGHEZZA / 2, xc + LISTELLI_LARGHEZZA / 2,
               yl - 6, yl, fz0 - 40, fz1 + 40, legno)

    # fuori: un prato e una fila di alberi. Solo ciò che si vede dalla
    # finestra ("occhio non vede, cuore non duole").
    piazza_mesh(carica(CUBO), "Prato", "Esterno", scala=(30.0, 20.0, 0.1),
                x=LUNGHEZZA / 2.0, y=-1000.0, zmin=-10.0, materiale=carica(MAT_ESTERNO))
    alberi = carica(SCANLABS + "Room/SM_Treeline")
    if alberi is not None:
        box = alberi.get_bounding_box()
        yaw = 0.0 if (box.max.x - box.min.x) >= (box.max.y - box.min.y) else 90.0
        piazza_mesh(alberi, "Alberi", "Esterno", yaw=yaw, x=LUNGHEZZA / 2.0,
                    y=-1700.0, zmin=-10.0)


def costruisci_mobili():
    legno = carica(MAT_LEGNO)
    # --- tavolino: piano + 4 gambe ---
    t = TAVOLINO
    x0, x1 = t["x"] - t["lato_x"] / 2, t["x"] + t["lato_x"] / 2
    y0, y1 = t["y"] - t["lato_y"] / 2, t["y"] + t["lato_y"] / 2
    blocco("Tavolino_Piano", "Mobili/Tavolino", x0, x1, y0, y1, t["altezza"] - 4, t["altezza"], legno)
    for i, (gx, gy) in enumerate(((x0, y0), (x1 - 4, y0), (x0, y1 - 4), (x1 - 4, y1 - 4))):
        blocco("Tavolino_Gamba_%d" % (i + 1), "Mobili/Tavolino",
               gx, gx + 4, gy, gy + 4, 0.0, t["altezza"] - 4, legno)

    # --- libreria contro il muro est ---
    W = LARGHEZZA
    cx, lw, d = LIBRERIA_CENTRO_X, LIBRERIA_LARGHEZZA, LIBRERIA_PROFONDITA
    lx0, lx1 = cx - lw / 2, cx + lw / 2
    ly0, ly1 = W - d, W
    alto = LIBRERIA_RIPIANI_Z[-1] + 3
    blocco("Libreria_Fianco_1", "Mobili/Libreria", lx0 - 3, lx0, ly0, ly1, 0.0, alto, legno)
    blocco("Libreria_Fianco_2", "Mobili/Libreria", lx1, lx1 + 3, ly0, ly1, 0.0, alto, legno)
    for i, z in enumerate(LIBRERIA_RIPIANI_Z):
        blocco("Libreria_Ripiano_%d" % (i + 1), "Mobili/Libreria", lx0, lx1, ly0, ly1, z - 3, z, legno)

    # libri sui ripiani (tranne l'ultimo, che è il "tetto" della libreria)
    caos = random.Random(SEME_CAOS)
    gruppi = [carica(p) for p in LIBRI_GRUPPI]
    gruppi = [g for g in gruppi if g is not None]
    if not gruppi:
        return
    for r, z in enumerate(LIBRERIA_RIPIANI_Z[:-1]):
        x = lx0 + 8
        n = 0
        while True:
            g = gruppi[(r * 3 + n) % len(gruppi)]
            yaw = 90.0 + caos.uniform(-4, 4)       # dorso verso la stanza (da verificare)
            b = ingombro(g, yaw, (1, 1, 1))
            larg = b[1] - b[0]
            alt = b[5] - b[4]
            spazio = (LIBRERIA_RIPIANI_Z[r + 1] - 3) - z
            if x + larg > lx1 - 6 or alt > spazio - 2:
                break
            piazza_mesh(g, "Libri_R%d_%d" % (r + 1, n + 1), "Mobili/Libreria/Libri",
                        yaw=yaw, xmin=x, ymax=W - 3, zmin=z)
            x += larg + caos.uniform(3, 25)      # buchi irregolari tra i gruppi
            n += 1


def costruisci_prop():
    tavolino_top = TAVOLINO["altezza"]
    caos = random.Random(SEME_CAOS + 1)
    for p in PROP:
        mesh = carica(p["mesh"])
        if mesh is None:
            continue
        z = 0.0
        quota = p.get("z")
        if quota == "TAVOLINO":
            z = tavolino_top
        elif isinstance(quota, str) and quota.startswith("SOPRA:"):
            sotto = _creati.get(quota.split(":", 1)[1])
            z = quota_superiore(sotto) if sotto else 0.0
        elif quota is not None:
            z = float(quota)
        # un grado o due di rotazione casuale in più: niente è mai perfettamente dritto
        yaw = p["yaw"] + caos.uniform(-2.0, 2.0)
        s = p.get("scala", 1.0)
        piazza_mesh(mesh, p["nome"], p["cartella"], yaw=yaw, scala=(s, s, s),
                    x=p["x"], y=p["y"], zmin=z, ombra=p.get("ombra", True))

    # quadri e presa: appoggiati al muro con il lato sottile verso il muro
    for q in QUADRI + [PRESA]:
        mesh = carica(q["mesh"])
        if mesh is None:
            continue
        box = mesh.get_bounding_box()
        sottile_su_x = (box.max.x - box.min.x) < (box.max.y - box.min.y)
        h = box.max.z - box.min.z
        zmin = q["z"] - h / 2.0
        if q["muro"] == "sud":            # muro a x = 0, si guarda verso +X
            yaw = 0.0 if sottile_su_x else 90.0
            piazza_mesh(mesh, q["nome"], "Prop/Muri", yaw=yaw, xmin=0.5, y=q["lungo"], zmin=zmin)
        elif q["muro"] == "nord":         # muro a x = L
            yaw = 0.0 if sottile_su_x else 90.0
            piazza_mesh(mesh, q["nome"], "Prop/Muri", yaw=yaw, xmax=LUNGHEZZA - 0.5,
                        y=q["lungo"], zmin=zmin)
        else:                             # muro est, y = W
            yaw = 90.0 if sottile_su_x else 0.0
            piazza_mesh(mesh, q["nome"], "Prop/Muri", yaw=yaw, ymax=LARGHEZZA - 0.5,
                        x=q["lungo"], zmin=zmin)


def costruisci_luci():
    # 1) il sole: UNA sola directional light. Conta solo la rotazione.
    sole = spawn_luce(unreal.DirectionalLight, "Sole_TardoPomeriggio",
                      vec(-300, -300, 400), rot(pitch=SOLE_PITCH, yaw=SOLE_YAW))
    c = sole.get_component_by_class(unreal.DirectionalLightComponent)
    imposta_luce(c, SOLE_INTENSITA_LUX, SOLE_TEMPERATURA_K, unita_candele=False)
    imposta(c, "atmosphere_sun_light", True)

    # 2) il cielo: atmosfera + sky light (il colore delle ombre) + nebbia leggera
    attori.spawn_actor_from_class(unreal.SkyAtmosphere, vec(0, 0, 0), rot())
    cielo = spawn_luce(unreal.SkyLight, "Cielo_ColoreOmbre", vec(-300, -200, 400), rot())
    c = cielo.get_component_by_class(unreal.SkyLightComponent)
    imposta(c, "real_time_capture", True)
    c.set_intensity(CIELO_INTENSITA)
    c.set_light_color(colore(CIELO_COLORE), True)
    nebbia = attori.spawn_actor_from_class(unreal.ExponentialHeightFog, vec(0, -800, 0), rot())
    etichetta(nebbia, "Nebbia_Esterno", "Luci")

    # 3) la lampada da lettura: point light sopra-davanti al paralume
    lampada = _creati.get("Lampada_Lettura")
    if lampada is not None:
        origine, est = lampada.get_actor_bounds(False)
        pos = vec(origine.x + 25, origine.y + 25, origine.z + est.z - 10)
    else:
        pos = vec(115, 125, 150)
    luce = spawn_luce(unreal.PointLight, "Lampada_LuceMotivata", pos, rot())
    c = luce.get_component_by_class(unreal.PointLightComponent)
    imposta_luce(c, LAMPADA_INTENSITA_CD, LAMPADA_TEMPERATURA_K, ombre=True)
    c.set_attenuation_radius(LAMPADA_RAGGIO)
    imposta(c, "source_radius", 12.0)       # ombre morbide, come un paralume

    # 4) rinforzo sulla libreria: spot senza ombre, dall'alto verso il muro est
    rinforzo = spawn_luce(unreal.SpotLight, "Rinforzo_Libreria_SenzaOmbre",
                          vec(LIBRERIA_CENTRO_X, LARGHEZZA - 170, ALTEZZA - 15),
                          rot(pitch=-45, yaw=90))
    c = rinforzo.get_component_by_class(unreal.SpotLightComponent)
    imposta_luce(c, RINFORZO_INTENSITA_CD, RINFORZO_TEMPERATURA_K, ombre=False)
    c.set_inner_cone_angle(25.0)
    c.set_outer_cone_angle(50.0)
    c.set_attenuation_radius(450.0)
    imposta(c, "source_radius", 30.0)

    # 5) corridoio: rect light fredda sul soffitto oltre la porta
    py0, py1 = PORTA_Y
    corr = spawn_luce(unreal.RectLight, "Corridoio_SegnalaUscita",
                      vec(LUNGHEZZA + SPESSORE + CORRIDOIO_PROFONDITA / 2, (py0 + py1) / 2, ALTEZZA - 5),
                      rot(pitch=-90))
    c = corr.get_component_by_class(unreal.RectLightComponent)
    imposta_luce(c, CORRIDOIO_INTENSITA_CD, rgb=CORRIDOIO_COLORE, ombre=True)
    imposta(c, "source_width", 120.0)
    imposta(c, "source_height", 60.0)
    c.set_attenuation_radius(500.0)

    # 6) post process: unbound, esposizione un po' contenuta, vignetta leggera
    pp = attori.spawn_actor_from_class(unreal.PostProcessVolume, vec(LUNGHEZZA / 2, LARGHEZZA / 2, 150), rot())
    etichetta(pp, "PostProcess_Globale", "Luci")
    imposta(pp, "unbound", True)
    s = pp.get_editor_property("settings")
    for prop, val in (("override_auto_exposure_bias", True), ("auto_exposure_bias", -0.3),
                      ("override_vignette_intensity", True), ("vignette_intensity", 0.35),
                      ("override_bloom_intensity", True), ("bloom_intensity", 0.5)):
        try:
            s.set_editor_property(prop, val)
        except Exception as e:  # noqa: BLE001
            avviso("post process: %s (%s)" % (prop, e))
    imposta(pp, "settings", s)


def costruisci_telecamere():
    for t in TELECAMERE:
        pos = vec(*t["pos"])
        guarda = vec(*t["guarda"])
        r = unreal.MathLibrary.find_look_at_rotation(pos, guarda)
        cam = attori.spawn_actor_from_class(unreal.CineCameraActor, pos, r)
        etichetta(cam, t["nome"], "Telecamere")
        comp = cam.get_cine_camera_component()
        imposta(comp, "current_focal_length", t["focale"])
        # niente sfocatura: per gli screenshot vogliamo tutto a fuoco
        fs = comp.get_editor_property("focus_settings")
        try:
            fs.set_editor_property("focus_method", unreal.CameraFocusMethod.DISABLE)
            comp.set_editor_property("focus_settings", fs)
        except Exception as e:  # noqa: BLE001
            avviso("focus: %s" % e)


def costruisci():
    """Punto d'ingresso: crea il livello, lo riempie e lo salva."""
    log("creo il livello " + LIVELLO)
    if unreal.EditorAssetLibrary.does_asset_exist(LIVELLO):
        # rilancio: si butta la versione generata in precedenza
        if not unreal.EditorAssetLibrary.delete_asset(LIVELLO):
            avviso("non riesco a cancellare il livello vecchio: se è aperto, "
                   "apri un altro livello e rilancia lo script.")
            return
    if not livelli.new_level(LIVELLO):
        avviso("new_level ha fallito: il livello esiste già o è aperto? "
               "Chiudilo, oppure cancellalo dal Content Browser, e rilancia.")
        return
    _creati.clear()
    costruisci_stanza()
    costruisci_finestra()
    costruisci_mobili()
    costruisci_prop()
    costruisci_luci()
    costruisci_telecamere()
    livelli.save_current_level()
    log("fatto: %d attori. Livello salvato." % len(_creati))


if __name__ == "__main__":
    costruisci()
