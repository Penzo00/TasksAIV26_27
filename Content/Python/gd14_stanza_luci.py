# -*- coding: utf-8 -*-
"""
GD14 - Task di Level Design: "una stanza significativa, con le luci usate
con uno scopo".

LA STANZA: "Il rifugio all'ora blu"
  La soffitta di un rifugio di montagna, appena fa buio. Fuori la luce
  fredda e blu del crepuscolo e della luna; dentro il fuoco acceso, la
  lampada vicino alla poltrona, la lampada a sospensione sul tavolo. Qualcuno
  è appena rientrato: le scarpe bagnate vicino alla porta ancora socchiusa,
  la legna accatastata, due bicchieri, un libro lasciato aperto.

  L'idea di luce è il CONTRASTO CALDO/FREDDO: il fuoco (1850 K) è il punto
  focale e la "chiave"; la finestra e la porta (8000-9000 K) sono il
  contrappunto freddo. Le luci "pratiche" (lampade, candele) sono motivate:
  si vede da dove vengono. Lo scopo di ogni luce è spiegato in
  Docs/GD14_stanza_luci.md (e nei commenti di costruisci_luci qui sotto).

  Ispirazione: le immagini di riferimento di un interno di baita (tetto
  spiovente con travi a vista, camino, tappeto, poltrona, luce calda). La
  stanza è nostra: pianta, arredo e luci sono progettati da zero.

COSA USA (i vincoli della consegna)
  - geometria costruita da zero (cubi/cilindri/coni dell'engine e
    Blockout_Box del plugin BlockOut Tools), nessuna stanza già pronta
  - prop del pacchetto Scan Labs (NON la sua stanza SM_Room/SM_Window)
  - le texture di Scan Labs nei nostri materiali World Aligned
    (aiv_materiali.py): assi del pavimento, intonaco, pietra del camino,
    tessuti, braci e fiamme emissive
  - niente Fab, niente Medieval Village

DUE FASI, COME NEL LEVEL DESIGN VERO
  costruisci(blockout=True)   -> /Game/Maps/L_GD14_Stanza_Blockout
      solo volumi grigi/colorati del plugin BlockOut Tools e i "proxy" dei
      mobili: serve a decidere misure, percorsi e inquadrature
  costruisci()                -> /Game/Maps/L_GD14_StanzaLuci
      la stanza finita, con materiali, prop, luci e telecamere

COME SI LANCIA
  0. Riavviare l'editor una volta dopo aver scaricato il branch (il redirect
     di Scan Labs in DefaultEngine.ini fa tornare i materiali ai prop).
  1. Tools > Execute Python Script... > questo file
     oppure nel Output Log, riga "Python":
         import gd14_stanza_luci; gd14_stanza_luci.costruisci()
         import gd14_stanza_luci; gd14_stanza_luci.costruisci(blockout=True)
  2. Il livello viene ricreato da capo ogni volta: le modifiche fatte a mano
     in quel livello si perdono (salvatelo prima con un altro nome).
  3. Screenshot: selezionare una delle CineCamera (cartella "Camere"),
     tasto destro > Pilot, poi High Resolution Screenshot.

DOVE MODIFICARE
  Le COSTANTI qui sotto: misure, luci (intensità, temperatura) e orientamento
  dei mobili. Se una poltrona guarda dalla parte sbagliata, cambiare
  FRONTE_SCANLABS (è l'unica cosa che lo script non può sapere: in che
  direzione è girata la mesh quando yaw = 0).

Unità: centimetri. Assi: X verso il camino (est), Y verso nord, Z in alto.
"""

import math
import random

import unreal

import aiv_livelli as a
import aiv_materiali as mat

# =====================================================================
#  1. MISURE
# =====================================================================

# un nome NUOVO: il vecchio L_GD14_StanzaLuci resta com'è, non viene toccato
LIVELLO = "/Game/Maps/L_GD14_Baita"
LIVELLO_BLOCKOUT = "/Game/Maps/L_GD14_Baita_Blockout"

LUNGO = 800.0          # interno lungo X (dalla finestra al camino)
LARGO = 600.0          # interno lungo Y
GRONDA = 180.0         # altezza dei muretti sotto le falde
COLMO = 450.0          # altezza del colmo (la trave centrale)
MURO = 20.0            # spessore dei muri
TETTO = 25.0           # spessore delle falde
PENDENZA = (COLMO - GRONDA) / (LARGO / 2.0)   # 0.9 -> falde a ~42 gradi

# finestra nel timpano OVEST (x = 0)
FINESTRA_Y = (120.0, 280.0)
FINESTRA_Z = (95.0, 245.0)
# porta nel timpano OVEST, socchiusa
PORTA_Y = (430.0, 520.0)
PORTA_Z = 210.0
PORTA_APERTURA = 32.0   # gradi

# camino sul timpano EST (x = LUNGO)
CAMINO_Y = (170.0, 430.0)
CAMINO_FRONTE = 700.0   # x della facciata del camino
ARCO_Z = 80.0           # dove comincia l'arco della bocca
ARCO_R = 60.0           # raggio della bocca
MENSOLA_Z = 190.0       # il gradino in cima al corpo del camino (candele)

# Che ora è: "tramonto" (sole basso e rosso, stanza calda e in penombra,
# come le foto di riferimento), "pomeriggio" (più luce, più colori) oppure
# "ora_blu" (fuori è quasi buio, luna fredda).
ORA = "tramonto"

SEME = 7

# Le mesh di Scan Labs, a yaw = 0, guardano verso +Y (di solito: esportate
# da un programma con Y in avanti). Se le poltrone guardano il muro invece
# del fuoco, provare 0, 90 o 180.
FRONTE_SCANLABS = 90.0

SCANLABS = "/Game/Meshes/"
CUBO = a.CUBO
CILINDRO = a.CILINDRO
CONO = "/Engine/BasicShapes/Cone"
SFERA = a.SFERA

# =====================================================================
#  2. LE LUCI (il perché di ognuna è in costruisci_luci e nella doc)
# =====================================================================

FUOCO_CD = 130.0
SOLE_LUX = 40.0
SOLE_K = 4300.0
SOLE_PITCH = -20.0
TRAMONTO_LUX = 12.0
TRAMONTO_K = 2400.0
TRAMONTO_PITCH = -9.0
FUOCO_K = 1850.0
LUNA_LUX = 2.5
LUNA_K = 8500.0
LUNA_PITCH = -24.0
LUNA_YAW = 12.0            # 0 = la luce va verso +X, cioè entra dalla finestra
CIELO_INTENSITA = 0.35
LAMPADA_TERRA_CD = 22.0
SOSPENSIONE_CD = 28.0
LAMPADE_K = 2700.0
CANDELA_CD = 2.5
PORTA_CD = 14.0
PORTA_K = 9500.0
LIBRI_CD = 5.0

# =====================================================================
#  3. TELECAMERE (posizione, punto guardato, focale mm)
# =====================================================================

TELECAMERE = [
    dict(nome="Cam_1_Insieme", pos=(70, 95, 165), guarda=(760, 330, 90), focale=18.0),
    dict(nome="Cam_2_Camino", pos=(420, 300, 105), guarda=(770, 300, 75), focale=28.0),
    dict(nome="Cam_3_Controcampo_Finestra", pos=(725, 470, 150), guarda=(0, 230, 135), focale=20.0),
    dict(nome="Cam_4_AngoloLettura", pos=(420, 360, 115), guarda=(610, 545, 85), focale=35.0),
    dict(nome="Cam_5_DaiPiediDelLetto", pos=(60, 470, 150), guarda=(700, 290, 80), focale=20.0),
]

PARTENZA_GIOCATORE = (330.0, 150.0, 100.0)
PARTENZA_YAW = 20.0

# =====================================================================
#  ATTREZZI
# =====================================================================

attori = a.attori
BLOCKOUT = False
M = {}
_creati = {}

# nel blockout ogni materiale diventa un colore del plugin
TINTE = {"Assi": "percorso2", "Intonaco": "percorso", "IntonacoChiaro": "percorso",
         "Pietra": "riparo", "Travi": "salto", "LegnoScuro": "salto", "LegnoChiaro": "salto",
         "LegnoDipinto": "muro", "TessutoRosso": "pericolo", "TessutoOcra": "pericolo",
         "Prato": "traguardo", "Braci": "pericolo", "Fiamma": "pericolo",
         "IntonacoArancio": "percorso", "Perline": "percorso2", "LegnoViola": "muro",
         "TessutoVerde": "traguardo", "TessutoGiallo": "salto", "Mattoni": "riparo",
         "MattoniScuri": "riparo", "CartaParati": "percorso", "TessutoBlu": "muro"}


def log(msg):
    unreal.log("[GD14] " + msg)


def avviso(msg):
    unreal.log_warning("[GD14] " + msg)


def _reg(attore, nome):
    if attore is not None:
        _creati[nome] = attore
    return attore


def box(nome, cartella, x0, x1, y0, y1, z0, z1, chiave, ombre=True, collisione=True):
    """Un parallelepipedo da (x0,y0,z0) a (x1,y1,z1)."""
    centro = ((x0 + x1) / 2.0, (y0 + y1) / 2.0, (z0 + z1) / 2.0)
    dim = (x1 - x0, y1 - y0, z1 - z0)
    if BLOCKOUT:
        att = a.blocco(nome, cartella, centro, dim, tinta=TINTE.get(chiave, "percorso"))
    else:
        att = a.blocco(nome, cartella, centro, dim, materiale=M.get(chiave))
    if att is not None and (not ombre or not collisione):
        for c in att.get_components_by_class(unreal.StaticMeshComponent):
            if not ombre:
                c.set_cast_shadow(False)
            if not collisione:
                c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    return _reg(att, nome)


def forma(nome, cartella, mesh_base, centro, scala, rotatore, chiave, ombre=True, collisione=True):
    """Una forma base dell'engine (cubo/cilindro/cono/sfera) orientata a piacere."""
    sm = a.carica(mesh_base)
    att = attori.spawn_actor_from_object(sm, a.vec(*centro), rotatore)
    att.set_actor_scale3d(a.vec(*scala))
    c = att.static_mesh_component
    materiale = M.get("Percorso") if BLOCKOUT and chiave not in ("Braci", "Fiamma") else M.get(chiave)
    if materiale is not None:
        c.set_material(0, materiale)
    c.set_cast_shadow(ombre)
    if not collisione:
        c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    return _reg(a.nomina(att, nome, cartella), nome)


def trave(nome, cartella, p0, p1, larghezza, altezza, su, chiave="Travi"):
    """Una trave da p0 a p1 (sezione larghezza x altezza), con 'su' come alto."""
    d = unreal.Vector(p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2])
    lunghezza = d.length()
    r = unreal.MathLibrary.make_rot_from_xz(d, a.vec(*su))
    centro = ((p0[0] + p1[0]) / 2.0, (p0[1] + p1[1]) / 2.0, (p0[2] + p1[2]) / 2.0)
    return forma(nome, cartella, CUBO, centro, (lunghezza / 100.0, larghezza / 100.0, altezza / 100.0), r, chiave)


def cilindro(nome, cartella, x, y, z0, raggio, altezza, chiave, ombre=True, collisione=True):
    return forma(nome, cartella, CILINDRO, (x, y, z0 + altezza / 2.0),
                 (raggio / 50.0, raggio / 50.0, altezza / 100.0), a.rot(), chiave, ombre, collisione)


def tronco(nome, cartella, centro, lunghezza, raggio, yaw=0.0, chiave="LegnoScuro"):
    """Un ciocco di legna sdraiato (cilindro con l'asse orizzontale)."""
    return forma(nome, cartella, CILINDRO, centro, (raggio / 50.0, raggio / 50.0, lunghezza / 100.0),
                 a.rot(pitch=90.0, yaw=yaw), chiave)


def ingombro(mesh, yaw, scala):
    """Bounding box della mesh ruotata (yaw) e scalata, con l'attore nell'origine."""
    b = mesh.get_bounding_box()
    r = math.radians(yaw)
    c, s = math.cos(r), math.sin(r)
    xs, ys = [], []
    for px in (b.min.x, b.max.x):
        for py in (b.min.y, b.max.y):
            px2, py2 = px * scala[0], py * scala[1]
            xs.append(px2 * c - py2 * s)
            ys.append(px2 * s + py2 * c)
    return (min(xs), max(xs), min(ys), max(ys), b.min.z * scala[2], b.max.z * scala[2])


def dimensioni(percorso):
    sm = a.carica(percorso)
    if sm is None:
        return None
    b = sm.get_bounding_box()
    return (b.max.x - b.min.x, b.max.y - b.min.y, b.max.z - b.min.z)


def prop(percorso, nome, cartella, x=None, y=None, z=0.0, yaw=0.0, scala=1.0, xmin=None, xmax=None,
         ymin=None, ymax=None, zmax=None, ombre=True, collisione=True, adatta=None):
    """Piazza una mesh decidendo dove va il suo INGOMBRO (non il pivot).
    x/y = centro in pianta (oppure un bordo con xmin/xmax/ymin/ymax);
    z = quota su cui appoggiarla (oppure zmax = quota del suo punto più alto).
    adatta = (dx, dy, dz) massimi: la mesh viene rimpicciolita per starci."""
    sm = a.carica(percorso)
    if sm is None:
        return None
    s = scala if isinstance(scala, (list, tuple)) else (scala, scala, scala)
    b = ingombro(sm, yaw, s)
    if adatta:
        f = min(1.0, adatta[0] / max(1.0, b[1] - b[0]), adatta[1] / max(1.0, b[3] - b[2]),
                adatta[2] / max(1.0, b[5] - b[4]))
        s = (s[0] * f, s[1] * f, s[2] * f)
        b = ingombro(sm, yaw, s)
    ax = (x - (b[0] + b[1]) / 2.0 if x is not None else
          xmin - b[0] if xmin is not None else xmax - b[1] if xmax is not None else 0.0)
    ay = (y - (b[2] + b[3]) / 2.0 if y is not None else
          ymin - b[2] if ymin is not None else ymax - b[3] if ymax is not None else 0.0)
    az = zmax - b[5] if zmax is not None else z - b[4]
    if BLOCKOUT:
        # nel blockout i mobili sono solo il loro volume ("proxy")
        return box(nome, "Proxy/" + cartella, ax + b[0], ax + b[1], ay + b[2], ay + b[3],
                   az + b[4], az + b[5], "LegnoScuro", collisione=collisione)
    att = attori.spawn_actor_from_object(sm, a.vec(ax, ay, az), a.rot(yaw=yaw))
    att.set_actor_scale3d(a.vec(*s))
    c = att.static_mesh_component
    c.set_cast_shadow(ombre)
    if not collisione:
        c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    return _reg(a.nomina(att, nome, cartella), nome)


def verso(da, a_punto):
    """Lo yaw (gradi) per guardare da 'da' verso 'a_punto' in pianta."""
    return math.degrees(math.atan2(a_punto[1] - da[1], a_punto[0] - da[0]))


def cima(nome):
    att = _creati.get(nome)
    if att is None:
        return None
    o, e = att.get_actor_bounds(False)
    return o.z + e.z


def fondo(nome):
    att = _creati.get(nome)
    if att is None:
        return None
    o, e = att.get_actor_bounds(False)
    return o.z - e.z


def z_tetto(y):
    """Quota dell'intradosso (la faccia interna) della falda sopra il punto y."""
    return GRONDA + PENDENZA * min(y, LARGO - y)


# =====================================================================
#  LA STANZA
# =====================================================================

def costruisci_involucro():
    L, W, T = LUNGO, LARGO, MURO
    # pavimento di assi e il prato fuori
    box("Pavimento", "Stanza", -T, L + T, -T, W + T, -20, 0, "Assi")
    box("Prato", "Esterno", -4000, 4000, -4000, 4000, -45, -25, "Prato")

    # muretti sotto le falde (sud y=0, nord y=W)
    box("Muretto_Sud", "Stanza/Muri", -T, L + T, -T, 0, 0, GRONDA + 5, "CartaParati")
    box("Muretto_Nord", "Stanza/Muri", -T, L + T, W, W + T, 0, GRONDA + 5, "CartaParati")

    # timpano ovest con la finestra e la porta (muro "a pezzi" intorno ai buchi)
    fy0, fy1 = FINESTRA_Y
    fz0, fz1 = FINESTRA_Z
    py0, py1 = PORTA_Y
    alto = COLMO + 35
    box("Ovest_1", "Stanza/Muri", -T, 0, -T, fy0, 0, alto, "CartaParati")
    box("Ovest_SottoFinestra", "Stanza/Muri", -T, 0, fy0, fy1, 0, fz0, "CartaParati")
    box("Ovest_SopraFinestra", "Stanza/Muri", -T, 0, fy0, fy1, fz1, alto, "CartaParati")
    box("Ovest_2", "Stanza/Muri", -T, 0, fy1, py0, 0, alto, "CartaParati")
    box("Ovest_SopraPorta", "Stanza/Muri", -T, 0, py0, py1, PORTA_Z, alto, "CartaParati")
    box("Ovest_3", "Stanza/Muri", -T, 0, py1, W + T, 0, alto, "CartaParati")
    # timpano est (dietro il camino)
    box("Est", "Stanza/Muri", L, L + T, -T, W + T, 0, alto, "CartaParati")

    # le due falde del tetto: lastre inclinate
    sporgenza = 45.0
    for nome, y_gronda, segno in (("Falda_Sud", 0.0, 1.0), ("Falda_Nord", W, -1.0)):
        y_a = y_gronda - segno * sporgenza
        z_a = GRONDA - PENDENZA * sporgenza
        y_b, z_b = W / 2.0, COLMO
        d = a.vec(0, y_b - y_a, z_b - z_a)
        lung = d.length()
        dn = a.vec(0, d.y / lung, d.z / lung)                 # lungo la falda, verso il colmo
        fuori = a.vec(0, -segno * abs(dn.z), abs(dn.y))        # normale: in alto e fuori dalla stanza
        centro = ((L) / 2.0, (y_a + y_b) / 2.0 + fuori.y * TETTO / 2.0, (z_a + z_b) / 2.0 + fuori.z * TETTO / 2.0)
        r = unreal.MathLibrary.make_rot_from_xy(a.vec(1, 0, 0), dn)
        forma(nome, "Stanza/Tetto", CUBO, centro, ((L + 2 * sporgenza) / 100.0, lung / 100.0, TETTO / 100.0),
              r, "Perline")
    box("Colmo_Esterno", "Stanza/Tetto", -sporgenza, L + sporgenza, W / 2 - 30, W / 2 + 30, COLMO, COLMO + 40,
        "LegnoScuro")


def costruisci_travi():
    """Travi a vista: colmo, puntoni ogni metro, due catene."""
    W = LARGO
    box("Trave_Colmo", "Stanza/Travi", -MURO, LUNGO + MURO, W / 2 - 12, W / 2 + 12, COLMO - 24, COLMO + 2, "Travi")
    lung = math.hypot(W / 2.0, COLMO - GRONDA)
    for lato, segno, y_g in (("S", 1.0, 0.0), ("N", -1.0, W)):
        # verso l'interno (giù) rispetto alla falda
        dentro = (0.0, segno * (COLMO - GRONDA) / lung, -(W / 2.0) / lung)
        su = (0.0, -dentro[1], -dentro[2])
        off = 9.0
        for i, x in enumerate(range(40, int(LUNGO), 100)):
            if x > LUNGO - 90:
                continue   # lì passa la canna fumaria
            p0 = (x, y_g + dentro[1] * off, GRONDA + dentro[2] * off)
            p1 = (x, W / 2.0 + dentro[1] * off, COLMO + dentro[2] * off)
            trave("Puntone_%s_%d" % (lato, i), "Stanza/Travi", p0, p1, 14, 18, su)
    # montanti e correnti scuri lungo i muretti: incorniciano l'intonaco arancio
    # (i montanti che finirebbero dentro un mobile non si mettono)
    for lato, y0, y1, xs in (("S", 0.0, 14.0, (0.0, 400.0, 600.0)), ("N", W - 14.0, W, (0.0, 200.0, 600.0))):
        box("Corrente_%s" % lato, "Stanza/Travi", -MURO, LUNGO, y0, y1, GRONDA - 14, GRONDA + 4, "Travi")
        for x in xs:
            box("Montante_%s_%d" % (lato, int(x)), "Stanza/Travi", x, x + 16, y0, y1, 0, GRONDA - 14, "Travi")
    for x in (240.0, 540.0):
        zc = 335.0
        y0 = (zc - GRONDA) / PENDENZA
        box("Catena_%d" % int(x), "Stanza/Travi", x - 8, x + 8, y0, W - y0, zc - 10, zc + 10, "Travi")


def costruisci_finestra_e_porta():
    fy0, fy1 = FINESTRA_Y
    fz0, fz1 = FINESTRA_Z
    c = "Stanza/Finestra"
    box("Davanzale", c, -MURO - 8, 14, fy0 - 10, fy1 + 10, fz0 - 6, fz0, "LegnoScuro")
    box("Telaio_S", c, -MURO, 0, fy0, fy0 + 8, fz0, fz1, "LegnoScuro")
    box("Telaio_N", c, -MURO, 0, fy1 - 8, fy1, fz0, fz1, "LegnoScuro")
    box("Telaio_Alto", c, -MURO, 0, fy0, fy1, fz1 - 8, fz1, "LegnoScuro")
    box("Montante", c, -12, -6, (fy0 + fy1) / 2 - 3, (fy0 + fy1) / 2 + 3, fz0, fz1, "LegnoScuro")
    box("Traverso", c, -12, -6, fy0, fy1, (fz0 + fz1) / 2 - 3, (fz0 + fz1) / 2 + 3, "LegnoScuro")
    if not BLOCKOUT:
        vetro = a.carica("/Game/Materials/MI_Win_Glas")
        if vetro is not None:
            v = box("Vetro", c, -10, -9, fy0 + 8, fy1 - 8, fz0, fz1 - 8, "LegnoScuro", ombre=False, collisione=False)
            if v is not None:
                for comp in v.get_components_by_class(unreal.StaticMeshComponent):
                    comp.set_material(0, vetro)
    # tende ai lati della finestra (incorniciano la luce fredda)
    box("Tenda_S", c, 2, 7, fy0 - 45, fy0 - 2, 55, 262, "TessutoRosso", collisione=False)
    box("Tenda_N", c, 2, 7, fy1 + 2, fy1 + 45, 55, 262, "TessutoRosso", collisione=False)
    box("Bastone_Tenda", c, 3, 9, fy0 - 55, fy1 + 55, 264, 268, "Ottone" if not BLOCKOUT else "LegnoScuro")

    # porta socchiusa: si apre verso l'interno, cerniera sul lato nord
    py0, py1 = PORTA_Y
    p = "Stanza/Porta"
    box("Stipite_S", p, -MURO - 2, 4, py0 - 8, py0, 0, PORTA_Z + 8, "LegnoScuro")
    box("Stipite_N", p, -MURO - 2, 4, py1, py1 + 8, 0, PORTA_Z + 8, "LegnoScuro")
    box("Architrave_Porta", p, -MURO - 2, 4, py0 - 8, py1 + 8, PORTA_Z, PORTA_Z + 8, "LegnoScuro")
    larg = py1 - py0 - 2
    th = math.radians(PORTA_APERTURA)
    cx = 2.0 + math.sin(th) * larg / 2.0
    cy = py1 - 1.0 - math.cos(th) * larg / 2.0
    pannello = forma("Porta_Socchiusa", p, CUBO, (cx, cy, (PORTA_Z - 6) / 2.0 + 2.5),
                     (0.05, larg / 100.0, (PORTA_Z - 6) / 100.0), a.rot(yaw=PORTA_APERTURA), "LegnoDipinto")
    if pannello is not None and not BLOCKOUT:
        maniglia_x = 2.0 + math.sin(th) * (larg - 8) + math.cos(th) * 5
        maniglia_y = py1 - 1.0 - math.cos(th) * (larg - 8) + math.sin(th) * 5
        forma("Maniglia", p, SFERA, (maniglia_x, maniglia_y, 100), (0.05, 0.05, 0.05), a.rot(), "Ottone")
    # zerbino fuori e dentro
    box("Zerbino", p, 5, 75, py0 + 5, py1 - 5, 0, 1.5, "Rattan" if not BLOCKOUT else "TessutoOcra", collisione=False)


def costruisci_camino():
    """Camino di mattoni: bocca ad arco, corpo che si restringe salendo,
    focolare a semicerchio davanti. Mattoni = materiale procedurale."""
    L = LUNGO
    cy0, cy1 = CAMINO_Y
    xf0 = CAMINO_FRONTE
    ym = (cy0 + cy1) / 2.0
    c = "Camino"
    # spalle fino all'imposta dell'arco
    box("Spalla_S", c, xf0, L, cy0, ym - ARCO_R, 0, ARCO_Z, "Mattoni")
    box("Spalla_N", c, xf0, L, ym + ARCO_R, cy1, 0, ARCO_Z, "Mattoni")
    # sopra: file orizzontali che lasciano libero il semicerchio della bocca
    for i, z in enumerate(range(int(ARCO_Z), int(MENSOLA_Z), 10)):
        dz = z + 5 - ARCO_Z
        mezzo = math.sqrt(ARCO_R ** 2 - dz ** 2) if dz < ARCO_R else 0.0
        if mezzo > 0.0:
            box("Corso_%d_S" % i, c, xf0, L, cy0, ym - mezzo, z, z + 10, "Mattoni")
            box("Corso_%d_N" % i, c, xf0, L, ym + mezzo, cy1, z, z + 10, "Mattoni")
        else:
            box("Corso_%d" % i, c, xf0, L, cy0, cy1, z, z + 10, "Mattoni")
    box("Fondo_Focolare", c, L - 15, L, ym - ARCO_R, ym + ARCO_R, 0, ARCO_Z + ARCO_R, "MattoniScuri")
    # l'arco: conci disposti a raggiera, appena sporgenti dalla facciata
    n = 11
    for k in range(n):
        t = math.pi * (k + 0.5) / n
        radiale = a.vec(0, math.cos(t), math.sin(t))
        rc = ARCO_R + 13.0
        centro = (xf0 - 2.0, ym + math.cos(t) * rc, ARCO_Z + math.sin(t) * rc)
        r = unreal.MathLibrary.make_rot_from_xz(a.vec(1, 0, 0), radiale)
        forma("Concio_%d" % k, c + "/Arco", CUBO, centro, (0.2, (math.pi * rc / n) / 100.0 * 1.04, 0.26), r,
              "Mattoni")
    # il corpo sale e si restringe a gradoni fino al tetto
    for i, (dx, dy, z0, z1) in enumerate(((15, 20, MENSOLA_Z, 260), (30, 40, 260, 330), (40, 55, 330, COLMO + 110))):
        box("Canna_%d" % i, c, xf0 + dx, L, cy0 + dy, cy1 - dy, z0, z1, "Mattoni")
    # focolare a semicerchio davanti alla bocca
    cilindro("Focolare", c, xf0, ym, 0, 95, 5, "Mattoni")
    for k in range(9):
        t = math.pi / 2.0 + math.pi * (k + 0.5) / 9
        cx, cy = xf0 + math.cos(t) * 95, ym + math.sin(t) * 95
        forma("Cordolo_%d" % k, c + "/Focolare", CUBO, (cx, cy, 7), (0.28, 0.34, 0.14),
              a.rot(yaw=math.degrees(t)), "Mattoni")

    # il fuoco: ciocchi, braci, fiamme
    xf = L - 55
    tronco("Ciocco_1", c + "/Fuoco", (xf, ym, 18), 85, 8, yaw=90)
    tronco("Ciocco_2", c + "/Fuoco", (xf - 12, ym - 8, 30), 70, 7, yaw=70)
    tronco("Ciocco_3", c + "/Fuoco", (xf + 10, ym + 10, 30), 70, 7, yaw=110)
    forma("Braci", c + "/Fuoco", SFERA, (xf, ym, 11), (0.75, 1.0, 0.1), a.rot(), "Braci", ombre=False,
          collisione=False)
    for i, (dy, h) in enumerate(((-22, 0.55), (0, 0.75), (20, 0.5))):
        forma("Fiamma_%d" % i, c + "/Fuoco", CONO, (xf, ym + dy, 34 + h * 50), (0.22, 0.26, h), a.rot(), "Fiamma",
              ombre=False, collisione=False)

    # legna accatastata accanto al camino
    caos = random.Random(SEME)
    i = 0
    for fila, z in enumerate((9, 25, 41)):
        for k in range(4 - fila):
            tronco("Legna_%d" % i, c + "/Legna", (L - 45 + caos.uniform(-3, 3), cy1 + 55 + k * 17 + fila * 8, z),
                   60 + caos.uniform(-6, 6), 8, yaw=caos.uniform(-4, 4))
            i += 1

    # sul gradino del camino: due candele; sulla canna un quadro appoggiato
    for nome, y in (("Candela_S", cy0 + 30), ("Candela_N", cy1 - 30)):
        cilindro(nome, c + "/Mensola", xf0 + 7, y, MENSOLA_Z, 3.5, 20, "IntonacoChiaro")
        forma(nome + "_Fiamma", c + "/Mensola", SFERA, (xf0 + 7, y, MENSOLA_Z + 23), (0.025, 0.025, 0.05), a.rot(),
              "Fiamma", ombre=False, collisione=False)
    d = dimensioni(SCANLABS + "Decor/SM_Picture_B")
    if d is not None:
        yaw = 0.0 if d[0] < d[1] else 180.0 - FRONTE_SCANLABS
        prop(SCANLABS + "Decor/SM_Picture_B", "Quadro_Camino", c, xmax=xf0 + 29.5, y=ym, z=263.0, yaw=yaw,
             adatta=(10, 110, 64))

    if not BLOCKOUT:
        a.decal("Fuliggine_Fondo", M["Fuliggine"], (L - 10, ym, 70), (12, 55, 60), pitch=0.0, yaw=0.0)
        a.decal("Fuliggine_Arco", M["Fuliggine"], (xf0 - 4, ym, ARCO_Z + ARCO_R), (8, 50, 25), pitch=0.0, yaw=0.0)
        a.decal("Cenere_Focolare", M["Fuliggine"], (xf0 - 40, ym, 8), (10, 35, 70))


def costruisci_mensole():
    """Mensole sul muro del camino, ai due lati, cariche di oggetti."""
    c = "Mensole"
    L = LUNGO
    oggetti = {
        "S": [("Books/SM_Books_Mix_2", 0.35), ("Decor/SM_Box_B", 0.7), ("Decor/SM_Glas_Wine_B", 0.85),
              ("Decor/SM_Vase_Flower", 0.4), ("Decor/SM_Photo", 0.75)],
        "N": [("Books/SM_Books_Mix_5", 0.6), ("Decor/SM_Deco_Bean", 0.25), ("Decor/SM_Box_C", 0.35),
              ("Fruits/SM_Apple_02", 0.8), ("Decor/SM_Glas_Water", 0.65)],
    }
    for lato, y0, y1 in (("S", 25.0, 150.0), ("N", 450.0, 575.0)):
        for j, z in enumerate((100.0, 150.0)):
            box("Mensola_%s_%d" % (lato, j), c, L - 30, L, y0, y1, z - 4, z, "LegnoScuro")
            for k, y in enumerate((y0 + 12, y1 - 12)):
                box("Mensola_%s_%d_Staffa_%d" % (lato, j, k), c, L - 18, L, y - 2, y + 2, z - 22, z - 4, "LegnoScuro")
            for k in range(2):
                rel, fy = oggetti[lato][(j * 2 + k) % len(oggetti[lato])]
                prop(SCANLABS + rel, "Mensola_%s_%d_Ogg_%d" % (lato, j, k), c, x=L - 15, y=y0 + (y1 - y0) * fy,
                     z=z, yaw=-90.0 - FRONTE_SCANLABS + 10.0 * k, adatta=(26, 60, 40))


def costruisci_soggiorno():
    """Il tappeto davanti al fuoco, due poltrone, il pouf, la lampada da terra."""
    c = "Soggiorno"
    fuoco = (LUNGO - 55, LARGO / 2.0)
    # tappeto rotondo a cerchi concentrici
    for i, (rr, chiave) in enumerate(((140, "TessutoVerde"), (110, "TessutoGiallo"), (80, "TessutoVerde"),
                                      (50, "TessutoGiallo"))):
        cilindro("Tappeto_%d" % i, c, 490, 300, i * 0.5, rr, 1.2, chiave, collisione=False)
    cilindro("Pouf", c, 470, 300, 0, 28, 40, "TessutoRosso")

    for nome, pos in (("Poltrona_Nord", (540, 470)), ("Poltrona_Sud", (540, 130))):
        yaw = verso(pos, fuoco) - FRONTE_SCANLABS
        prop(SCANLABS + "Upholstery/SM_Couch_01_Single", nome, c, x=pos[0], y=pos[1], yaw=yaw)

    # tavolino di rattan accanto alla poltrona nord, con bicchiere e libro
    cilindro("Tavolino_Gamba", c, 470, 545, 0, 5, 52, "Ottone" if not BLOCKOUT else "LegnoScuro")
    cilindro("Tavolino_Piano", c, 470, 545, 52, 26, 3, "Rattan" if not BLOCKOUT else "LegnoChiaro")
    prop(SCANLABS + "Decor/SM_Glas_Wine_B", "Bicchiere_Vino", c, x=462, y=535, z=55)
    prop(SCANLABS + "Books/SM_Book_012", "Libro_Aperto", c, x=480, y=556, z=55, yaw=25, adatta=(30, 30, 10))

    # lampada da terra dietro la poltrona: il paralume NON fa ombra alla sua luce
    prop(SCANLABS + "Decor/SM_Lamp_Pear_Stand", "Lampada_Terra", c, x=650, y=560, yaw=200, ombre=False)
    prop(SCANLABS + "Decor/SM_Groundplant_12", "Pianta", c, x=40, y=572, adatta=(60, 56, 150))


def costruisci_cucina():
    """Il lato sud: mobili dipinti, mensola, tavolo da pranzo con panca."""
    c = "Cucina"
    box("Mobile_Base", c, 40, 340, 0, 60, 0, 85, "LegnoViola")
    box("Piano_Lavoro", c, 35, 345, 0, 66, 85, 90, "LegnoChiaro")
    for i, x in enumerate((115, 190, 265)):
        box("Anta_Fuga_%d" % i, c, x - 1, x + 1, 60, 61, 8, 80, "LegnoScuro")
    for i, x in enumerate((95, 135, 170, 210, 245, 285, 320)):
        if not BLOCKOUT:
            forma("Pomello_%d" % i, c, SFERA, (x, 63, 66), (0.04, 0.04, 0.04), a.rot(), "Ottone")
    box("Mensola", c, 60, 320, 0, 26, 135, 139, "LegnoScuro")
    # alzatina di mattoni tra il piano e la mensola, pentole appese
    box("Alzatina", c, 40, 340, 0, 3, 90, 135, "Mattoni")
    box("Barra_Pentole", c, 70, 200, 20, 23, 128, 131, "Ferro" if not BLOCKOUT else "LegnoScuro")
    for i, (x, r, h) in enumerate(((95, 11, 13), (135, 9, 11), (172, 7, 9))):
        cilindro("Pentola_%d" % i, c, x, 22, 112 - h, r, h, "Ferro" if not BLOCKOUT else "LegnoScuro",
                 collisione=False)
        box("Gancio_%d" % i, c, x - 0.5, x + 0.5, 21, 23, 112, 128, "Ferro" if not BLOCKOUT else "LegnoScuro",
            collisione=False)
    prop(SCANLABS + "Fruits/SM_Fruitplate", "Piatto_Frutta", c, x=110, y=35, z=90, adatta=(40, 40, 15))
    prop(SCANLABS + "Decor/SM_Glas_Water", "Bicchiere_Acqua", c, x=205, y=30, z=90)
    prop(SCANLABS + "Decor/SM_Box_D", "Scatola_Pane", c, x=290, y=32, z=90, adatta=(50, 45, 35))
    prop(SCANLABS + "Decor/SM_Glas_Wine_A", "Calice_1", c, x=110, y=13, z=139)
    prop(SCANLABS + "Decor/SM_Glas_Wine_A", "Calice_2", c, x=128, y=14, z=139, yaw=40)
    prop(SCANLABS + "Books/SM_Books_Mix_7", "Ricettari", c, x=240, y=13, z=139, adatta=(90, 24, 30))

    # tavolo da pranzo
    tx0, tx1, ty0, ty1, h = 120, 280, 260, 400, 76
    box("Tavolo_Piano", c, tx0, tx1, ty0, ty1, h - 5, h, "LegnoScuro")
    for i, (x, y) in enumerate(((tx0 + 8, ty0 + 8), (tx1 - 8, ty0 + 8), (tx0 + 8, ty1 - 8), (tx1 - 8, ty1 - 8))):
        box("Tavolo_Gamba_%d" % i, c, x - 4, x + 4, y - 4, y + 4, 0, h - 5, "LegnoScuro")
    box("Panca_Piano", c, 130, 270, 205, 240, 40, 45, "LegnoChiaro")
    box("Panca_Gamba_1", c, 140, 146, 207, 238, 0, 40, "LegnoChiaro")
    box("Panca_Gamba_2", c, 254, 260, 207, 238, 0, 40, "LegnoChiaro")
    cilindro("Sgabello", c, 320, 340, 0, 18, 46, "LegnoChiaro")
    prop(SCANLABS + "Fruits/SM_FruitsOnPlate", "Frutta", c, x=195, y=335, z=h, yaw=15, adatta=(45, 45, 25))
    prop(SCANLABS + "Decor/SM_Glas_Wine_A", "Calice_Tavolo", c, x=245, y=300, z=h)
    prop(SCANLABS + "Decor/SM_Glas_Wine_B", "Calice_Tavolo_2", c, x=150, y=370, z=h, yaw=60)
    # lampada a sospensione appesa al colmo
    prop(SCANLABS + "Decor/SM_Lamp_Pending", "Lampada_Sospensione", c, x=200, y=330, zmax=COLMO - 24, ombre=False)
    if (fondo("Lampada_Sospensione") or 0) < h + 60:
        # se la mesh ha un cavo lunghissimo, la si accorcia scalandola
        att = _creati.get("Lampada_Sospensione")
        if att is not None:
            att.destroy_actor()
        prop(SCANLABS + "Decor/SM_Lamp_Pending", "Lampada_Sospensione", c, x=200, y=330, zmax=COLMO - 24,
             ombre=False, adatta=(80, 80, COLMO - 24 - (h + 90)))


def costruisci_libreria():
    """Libreria bassa sotto la falda nord."""
    c = "Libreria"
    x0, x1, y0, y1 = 300.0, 440.0, LARGO - 38, LARGO
    ripiani = (0.0, 50.0, 100.0, 148.0)
    for i, z in enumerate(ripiani):
        box("Ripiano_%d" % i, c, x0, x1, y0, y1, z, z + 4, "LegnoScuro")
    xm = (x0 + x1) / 2.0
    for i, x in enumerate((x0, xm - 2, x1 - 4)):
        box("Fianco_%d" % i, c, x, x + 4, y0, y1, 0, ripiani[-1] + 4, "LegnoScuro")
    k = 1
    for z in ripiani[:-1]:
        for xc in ((x0 + xm) / 2.0, (xm + x1) / 2.0):
            prop(SCANLABS + "Books/SM_Books_Mix_%d" % k, "Libri_%d" % k, c, x=xc, y=(y0 + y1) / 2.0, z=z + 4,
                 adatta=((x1 - x0) / 2.0 - 10.0, 34, 44), yaw=0.0 if _lungo_x("Books/SM_Books_Mix_%d" % k) else 90.0)
            k = k % 8 + 1
    zt = ripiani[-1] + 4
    prop(SCANLABS + "Decor/SM_Buddha", "Statuetta", c, x=325, y=(y0 + y1) / 2.0, z=zt, adatta=(30, 30, 35),
         yaw=-90.0 - FRONTE_SCANLABS)
    prop(SCANLABS + "Decor/SM_Box_A", "Scatola_Ricordi", c, x=405, y=(y0 + y1) / 2.0, z=zt, yaw=-8, adatta=(60, 34, 30))

    # quadri ai lati della canna fumaria (sul timpano est) e accanto alla finestra
    for nome, mesh, y, z in (("Quadro_A", "Decor/SM_Picture_A", 100, 175), ("Quadro_C", "Decor/SM_Picture_C", 500, 165)):
        _quadro_su_muro_est(SCANLABS + mesh, nome, y, z)
    # scarpe bagnate vicino alla porta: qualcuno è appena rientrato
    prop(SCANLABS + "Decor/SM_Shoe_Boat", "Scarpa_1", "Racconto", x=40, y=410, z=0, yaw=80)
    prop(SCANLABS + "Decor/SM_Shoe_Boat", "Scarpa_2", "Racconto", x=62, y=398, z=0, yaw=55)
    if not BLOCKOUT:
        a.decal("Orme_Bagnate", M["Pozza"], (70, 420, 4), (10, 35, 55), yaw=-20.0)


def costruisci_letto():
    """Il letto sotto la falda nord, con le colonnine tornite, e il comò."""
    c = "Letto"
    x0, x1, y0, y1 = 85.0, 290.0, LARGO - 118, LARGO - 16
    box("Letto_Telaio", c, x0, x1, y0, y1, 28, 42, "LegnoScuro")
    box("Letto_Materasso", c, x0 + 8, x1 - 6, y0 + 4, y1 - 4, 42, 62, "IntonacoChiaro")
    box("Letto_Coperta", c, x0 + 60, x1 - 2, y0 - 2, y1 - 2, 54, 66, "TessutoBlu")
    box("Letto_Risvolto", c, x0 + 55, x0 + 75, y0 - 1, y1 - 3, 55, 67, "IntonacoChiaro")
    forma("Cuscino", c, SFERA, (x0 + 30, (y0 + y1) / 2.0, 70), (0.4, 0.75, 0.16), a.rot(), "IntonacoChiaro",
          collisione=False)
    # testiera (alta) e pediera (bassa) con colonnine e pomi
    for lato, x, h in (("Testa", x0, 120.0), ("Piedi", x1 - 8, 80.0)):
        box("Letto_%s_Pannello" % lato, c, x, x + 8, y0 + 8, y1 - 8, 30, h - 15, "LegnoScuro")
        for k, y in enumerate((y0 + 5, y1 - 5)):
            cilindro("Letto_%s_Colonna_%d" % (lato, k), c, x + 4, y, 0, 6, h, "LegnoScuro")
            forma("Letto_%s_Pomo_%d" % (lato, k), c, SFERA, (x + 4, y, h + 6), (0.16, 0.16, 0.16), a.rot(),
                  "LegnoScuro")
    # il comò, sul lato sud tra la cucina e la poltrona
    box("Como", c, 425, 525, 0, 48, 0, 82, "LegnoScuro")
    box("Como_Piano", c, 421, 529, 0, 52, 82, 86, "LegnoChiaro")
    for k, z in enumerate((22.0, 55.0)):
        box("Como_Cassetto_%d" % k, c, 429, 521, 48, 50, z - 14, z + 14, "LegnoViola")
        if not BLOCKOUT:
            for j, x in enumerate((450.0, 500.0)):
                forma("Como_Pomello_%d_%d" % (k, j), c, SFERA, (x, 52, z), (0.04, 0.04, 0.04), a.rot(), "Ottone")
    prop(SCANLABS + "Decor/SM_Vase_Flower", "Vaso_Fiore", c, x=445, y=24, z=86, adatta=(30, 30, 70))
    prop(SCANLABS + "Decor/SM_Glas_Water", "Boccia", c, x=505, y=24, z=86, adatta=(25, 25, 25))


def _lungo_x(rel):
    d = dimensioni(SCANLABS + rel)
    return d is None or d[0] >= d[1]


def _quadro_su_muro_est(percorso, nome, y, z):
    d = dimensioni(percorso)
    if d is None:
        return
    sottile_su_x = d[0] < d[1]
    # sottile su Y: il davanti guarda verso FRONTE_SCANLABS; lo giro verso -X (la stanza)
    yaw = 0.0 if sottile_su_x else 180.0 - FRONTE_SCANLABS
    prop(percorso, nome, "Quadri", xmax=LUNGO - 0.5, y=y, z=z - d[2] / 2.0, yaw=yaw, adatta=(10, 90, 90))


def costruisci_esterno():
    """Quello che si vede dalla finestra e dalla porta."""
    if BLOCKOUT:
        return
    prop(SCANLABS + "Tree/SM_Chestnut_4", "Castagno", "Esterno", x=-700, y=160, z=-25, yaw=30)
    prop(SCANLABS + "Room/SM_Treeline", "Bosco", "Esterno", x=-2600, y=300, z=-25, yaw=90)
    for i in range(6):
        a.mesh(SCANLABS + "Decor/SM_Groundplant_12", "Cespuglio_%d" % i, "Esterno",
               (-250 - i * 90, 380 + (i % 2) * 120, -25), yaw=i * 50.0, scala=1.2, collisione=False)


# =====================================================================
#  LE LUCI
# =====================================================================

def costruisci_luci():
    L, W = LUNGO, LARGO
    fuoco = (L - 55, W / 2.0)

    # 1) IL FUOCO - luce chiave e punto focale. Calda (1850 K), con ombre,
    #    tremola con un rumore di Perlin (AFlickeringLight, C++): la stanza
    #    "respira". Fa tremolare anche il materiale emissivo delle braci.
    luce = a.spawn_cpp("FlickeringLight", "Luce_Fuoco_Chiave", "Luci/Fuoco", (fuoco[0] - 10, fuoco[1], 55),
                       base_intensity=FUOCO_CD, temperature=FUOCO_K, flicker_amount=0.28, flicker_speed=6.0,
                       position_jitter=5.0, attenuation_radius=1500.0, source_radius=18.0, cast_shadows=True)
    a.collega(luce, "glow_actors", [_creati.get("Braci")])
    # un rimbalzo caldo e morbido fuori dalla bocca del camino, senza ombre:
    # simula la luce che esce dal focolare e scalda il tappeto
    a.luce_punto("Fuoco_Rimbalzo_SenzaOmbre", (L - 170, fuoco[1], 40), intensita=FUOCO_CD * 0.18,
                 temperatura=FUOCO_K + 200, raggio=650.0, sorgente=60.0, ombre=False, cartella="Luci/Fuoco")

    # 2) IL SOLE (pomeriggio) o LA LUNA (ora blu) - entra dalla finestra.
    #    Pomeriggio: un sole basso e caldo disegna la finestra (con la croce
    #    del telaio) sulle assi del pavimento e, con la nebbia volumetrica,
    #    i raggi nell'aria: è la "lama di luce" della foto di riferimento.
    #    Ora blu: la stessa finestra diventa un controluce freddo.
    pomeriggio = ORA in ("pomeriggio", "tramonto")
    if ORA == "tramonto":
        # il sole è quasi all'orizzonte: rosso, radente, entra in profondità
        esterna = a.luce_esterna(sole_pitch=TRAMONTO_PITCH, sole_yaw=LUNA_YAW, sole_lux=TRAMONTO_LUX,
                                 temperatura=TRAMONTO_K, cielo_intensita=0.45, nebbia_densita=0.02,
                                 nebbia_volumetrica=True)
    elif pomeriggio:
        esterna = a.luce_esterna(sole_pitch=SOLE_PITCH, sole_yaw=LUNA_YAW, sole_lux=SOLE_LUX,
                                 temperatura=SOLE_K, cielo_intensita=1.0, nebbia_densita=0.02,
                                 nebbia_volumetrica=True)
    else:
        esterna = a.luce_esterna(sole_pitch=LUNA_PITCH, sole_yaw=LUNA_YAW, sole_lux=LUNA_LUX,
                                 temperatura=LUNA_K, cielo_intensita=CIELO_INTENSITA, nebbia_densita=0.02,
                                 nebbia_volumetrica=True, luna=True)
    if esterna is not None:
        c = esterna.get_component_by_class(unreal.DirectionalLightComponent)
        a.imposta(c, "volumetric_scattering_intensity", 2.0 if pomeriggio else 3.0, silenzioso=True)
    # 3) IL CIELO (Sky Light, dentro luce_esterna): il colore delle ombre.
    #    Di pomeriggio schiarisce le ombre (la stanza resta leggibile e
    #    colorata); all'ora blu è bassa e fredda: dove il fuoco non arriva la
    #    stanza resta blu, non nera.

    # 4) LA PORTA SOCCHIUSA - una rect light fuori dalla porta: una lama di
    #    luce entra sul pavimento e porta l'occhio verso l'uscita. Di giorno
    #    è luce diurna (5500 K), all'ora blu è fredda.
    py0, py1 = PORTA_Y
    a.luce_rect("Porta_LuceEsterna", (-90, (py0 + py1) / 2.0, 110), 0.0, 0.0, 90, 200,
                intensita=PORTA_CD * (2.0 if pomeriggio else 1.0),
                temperatura=(3200.0 if ORA == "tramonto" else 5500.0) if pomeriggio else PORTA_K, raggio=800.0, porte=40.0,
                cartella="Luci/Esterno")

    # 5) LAMPADA DA TERRA - luce pratica dell'angolo lettura. Motivata: la
    #    sorgente si vede. Sta sotto/davanti al paralume (che non fa ombra).
    _luce_lampada("Lampada_Terra", "Luce_Lampada_Terra", LAMPADA_TERRA_CD, verso_basso=True)

    # 6) LAMPADA A SOSPENSIONE - un cono di luce sul tavolo: separa la zona
    #    pranzo dal resto e illumina la frutta (secondo punto d'interesse).
    _luce_lampada("Lampada_Sospensione", "Luce_Sospensione", SOSPENSIONE_CD, verso_basso=True, sotto=True)

    # 7) CANDELE - piccoli accenti caldi sulla mensola (foto, vaso), senza ombre.
    for nome, y in (("Candela_S", CAMINO_Y[0] + 30), ("Candela_N", CAMINO_Y[1] - 30)):
        a.luce_punto("Luce_" + nome, (CAMINO_FRONTE + 7, y, MENSOLA_Z + 26), intensita=CANDELA_CD, temperatura=2000.0,
                     raggio=260.0, sorgente=1.0, ombre=False, cartella="Luci/Candele")

    # 8) RIEMPIMENTO SULLA LIBRERIA - spot debole senza ombre: rende leggibili
    #    i dorsi dei libri, che il fuoco prende di taglio.
    a.luce_spot("Libreria_Riempimento_SenzaOmbre", (370, W - 230, 330), -40.0, 90.0, intensita=LIBRI_CD,
                         temperatura=3200.0, raggio=500.0, cono_esterno=45.0, cono_interno=25.0, sorgente=40.0,
                         ombre=False, cartella="Luci/Riempimento")

    # post process: esposizione stabile per l'interno, bloom per le fiamme,
    # vignetta per chiudere l'inquadratura, leggera grana
    a.post_process("PostProcess_Stanza", auto_exposure_min_brightness=0.8, auto_exposure_max_brightness=2.5,
                   auto_exposure_bias={"pomeriggio": 0.0, "tramonto": 0.2}.get(ORA, 0.3), bloom_intensity=0.7,
                   vignette_intensity=0.45, film_grain_intensity=0.15, white_temp=6300.0)


def _luce_lampada(nome_mesh, nome_luce, intensita, verso_basso=False, sotto=False):
    att = _creati.get(nome_mesh)
    if att is None:
        return
    o, e = att.get_actor_bounds(False)
    if sotto:
        pos = (o.x, o.y, o.z - e.z - 8.0)
    else:
        pos = (o.x, o.y, o.z + e.z - 22.0)
    a.luce_punto(nome_luce, pos, intensita=intensita, temperatura=LAMPADE_K, raggio=550.0, sorgente=8.0,
                 ombre=True, cartella="Luci/Lampade")


def costruisci_telecamere():
    for t in TELECAMERE:
        r = unreal.MathLibrary.find_look_at_rotation(a.vec(*t["pos"]), a.vec(*t["guarda"]))
        cam = a.camera(t["nome"], t["pos"], r.pitch, r.yaw, focale=t["focale"], apertura=4.0)
        if cam is not None:
            fs = cam.get_cine_camera_component().get_editor_property("focus_settings")
            a.imposta(fs, "focus_method", unreal.CameraFocusMethod.DISABLE, silenzioso=True)
            cam.get_cine_camera_component().set_editor_property("focus_settings", fs)


# =====================================================================
#  PUNTI D'INGRESSO
# =====================================================================

def costruisci(blockout=False):
    """Crea il livello (finito, o il blockout), lo riempie e lo salva."""
    global BLOCKOUT
    BLOCKOUT = blockout
    percorso = LIVELLO_BLOCKOUT if blockout else LIVELLO
    log("creo il livello " + percorso)
    M.clear()
    M.update(mat.tutti())
    if not a.nuovo_livello(percorso):
        return
    _creati.clear()
    del a.ERRORI[:]
    # ogni parte è "al sicuro": se una si ferma per un errore (scritto nel
    # log), le altre vengono costruite lo stesso e il livello viene salvato
    for parte in (costruisci_involucro, costruisci_travi, costruisci_finestra_e_porta, costruisci_camino,
                  costruisci_mensole, costruisci_soggiorno, costruisci_cucina, costruisci_libreria,
                  costruisci_letto,
                  costruisci_esterno):
        a.sicuro(parte)
    if blockout:
        # nel blockout bastano una luce per il fuoco e il cielo, per leggere i volumi
        a.luce_esterna(sole_pitch=LUNA_PITCH, sole_yaw=LUNA_YAW, sole_lux=4.0)
        a.luce_punto("Fuoco_Blockout", (LUNGO - 65, LARGO / 2.0, 55), intensita=FUOCO_CD, temperatura=FUOCO_K,
                     raggio=1500.0)
    else:
        a.sicuro(costruisci_luci)
    a.sicuro(costruisci_telecamere)
    a.rendi_giocabile(PARTENZA_GIOCATORE, PARTENZA_YAW)
    a.salva()
    if a.ERRORI:
        avviso("ATTENZIONE: queste parti si sono fermate per un errore (vedi le righe rosse sopra): "
               + ", ".join(a.ERRORI))
    log("fatto: %d oggetti. Livello salvato in %s" % (len(_creati), percorso))


def rendi_giocabile():
    """Rende giocabile il livello GIA' ESISTENTE, senza ricostruirlo."""
    if not a.livelli.load_level(LIVELLO):
        avviso("non riesco ad aprire " + LIVELLO)
        return
    for att in attori.get_all_level_actors():
        if isinstance(att, unreal.PlayerStart):
            avviso("c'è già un PlayerStart")
            return
    a.rendi_giocabile(PARTENZA_GIOCATORE, PARTENZA_YAW)
    a.salva()


if __name__ == "__main__":
    costruisci()
