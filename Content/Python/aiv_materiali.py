# -*- coding: utf-8 -*-
"""
I materiali dei livelli dei task, costruiti da script (UE3: Material
Expression, parametri, Material Instance, World Aligned, Panner/Noise, la
lava; UE16: le decal).

Perché da script: i cubi dell'engine scalati "stirano" la texture (lo
stretch visto in UE3). Il materiale M_AIV_Triplanare usa invece la posizione
nel mondo (World Position) per le UV, proiettata dai tre lati e mescolata
con la normale: la texture ha la stessa dimensione su ogni faccia, comunque
si scali il cubo. È la stessa idea dei materiali World Aligned della lezione,
fatta con i nodi base.

Tutti i materiali finiscono in /Game/AIV/Materiali. Si possono aprire e
modificare come quelli fatti a mano. Rilanciare lo script li ricostruisce.

Uso (dagli altri script):
    import aiv_materiali as mat
    m = mat.tutti()          # dizionario nome -> MaterialInstance
    m["Legno"], m["Intonaco"], m["Lava"], ...
"""

import unreal

CARTELLA = "/Game/AIV/Materiali"

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()


def _log(msg):
    unreal.log("[AIV materiali] " + msg)


def _avviso(msg):
    unreal.log_warning("[AIV materiali] " + msg)


def _carica(percorsi):
    lista = percorsi if isinstance(percorsi, (list, tuple)) else [percorsi]
    for p in lista:
        if eal.does_asset_exist(p):
            return eal.load_asset(p)
    _avviso("asset mancante: %s" % (percorsi,))
    return None


# Texture (i file sono nella radice di Content; il redirect in DefaultEngine.ini
# serve solo ai riferimenti interni del pacchetto Scan Labs).
TEX = {
    "legno": "/Game/Textures/T_Wood_A_diffuse",
    "intonaco": "/Game/Textures/T_Plaster_01_Diffuse",
    "tessuto": "/Game/Textures/T_Fabrics_001_Diffuse",
    "erba": "/Game/Textures/T_Grass_001_4096_basecolor",
    "quarzo": "/Game/Textures/T_Quartz_01_4096_Diffuse",
    "rattan": "/Game/Textures/T_Ratan_Table_Ratan_diffuse",
    "corteccia": "/Game/Textures/ChestNut/T_Chestnut_Bark_4096_basecolor",
    "piastrelle": "/Game/Tile_009/Texture/Tile_009/T_Tile_009_BaseColor",
    "griglia": "/BlockoutToolsPlugin/Textures/T_Blockout_Grid",
}

PM_ROCCIA = "/Game/PhysicalMaterial/PM_Rock"
PM_ACQUA = "/Game/PhysicalMaterial/PM_Water"


# ---------------------------------------------------------------------
# attrezzi per i nodi
# ---------------------------------------------------------------------

def _nuovo_asset(nome, classe, factory):
    percorso = CARTELLA + "/" + nome
    if eal.does_asset_exist(percorso):
        a = eal.load_asset(percorso)
        if isinstance(a, classe):
            return a, False
        eal.delete_asset(percorso)
    return tools.create_asset(nome, CARTELLA, classe, factory), True


def _materiale(nome):
    m, _ = _nuovo_asset(nome, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(m)
    return m


def _nodo(m, classe, x, y, **prop):
    e = mel.create_material_expression(m, classe, x, y)
    for k, v in prop.items():
        try:
            e.set_editor_property(k, v)
        except Exception as ex:  # noqa: BLE001
            _avviso("%s.%s: %s" % (classe.__name__, k, ex))
    return e


def _scalare(m, nome, valore, x, y):
    return _nodo(m, unreal.MaterialExpressionScalarParameter, x, y,
                 parameter_name=nome, default_value=float(valore))


def _vettore(m, nome, rgb, x, y):
    return _nodo(m, unreal.MaterialExpressionVectorParameter, x, y,
                 parameter_name=nome, default_value=unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))


def _collega(a, uscita, b, ingresso):
    if not mel.connect_material_expressions(a, uscita, b, ingresso):
        _avviso("collegamento fallito: %s.%s -> %s.%s" % (a.get_class().get_name(), uscita,
                                                          b.get_class().get_name(), ingresso))


def _uscita(e, uscita, proprieta):
    if not mel.connect_material_property(e, uscita, proprieta):
        _avviso("non riesco a collegare %s" % proprieta)


def _maschera(m, sorgente, r, g, b, x, y):
    e = _nodo(m, unreal.MaterialExpressionComponentMask, x, y, r=r, g=g, b=b, a=False)
    _collega(sorgente, "", e, "")
    return e


def _op(m, classe, a, b, x, y, ia="A", ib="B"):
    e = _nodo(m, classe, x, y)
    _collega(a, "", e, ia)
    if b is not None:
        _collega(b, "", e, ib)
    return e


def _salva(asset):
    eal.save_loaded_asset(asset, False)


# ---------------------------------------------------------------------
# i materiali "padre"
# ---------------------------------------------------------------------

def crea_triplanare():
    """Texture proiettata dal mondo sui tre assi: niente stretch sui cubi scalati."""
    m = _materiale("M_AIV_Triplanare")
    tex_default = _carica(TEX["intonaco"])

    wp = _nodo(m, unreal.MaterialExpressionWorldPosition, -1600, 0)
    dim = _scalare(m, "Dimensione", 200.0, -1600, 150)
    uvw = _op(m, unreal.MaterialExpressionDivide, wp, dim, -1400, 0)

    proiezioni = []
    for i, (r, g, b) in enumerate(((False, True, True), (True, False, True), (True, True, False))):
        uv = _maschera(m, uvw, r, g, b, -1200, -300 + i * 300)
        t = _nodo(m, unreal.MaterialExpressionTextureSampleParameter2D, -1000, -300 + i * 300,
                  parameter_name="Diffuse", texture=tex_default)
        _collega(uv, "", t, "UVs")
        proiezioni.append(t)       # YZ (muri lungo X), XZ (muri lungo Y), XY (pavimenti)

    # pesi: |normale|^4, normalizzati (più il numero è alto, più netto il passaggio)
    n = _nodo(m, unreal.MaterialExpressionVertexNormalWS, -1400, 800)
    an = _nodo(m, unreal.MaterialExpressionAbs, -1250, 800)
    _collega(n, "", an, "")
    pw = _nodo(m, unreal.MaterialExpressionPower, -1100, 800, const_exponent=4.0)
    _collega(an, "", pw, "Base")
    uno = _nodo(m, unreal.MaterialExpressionConstant3Vector, -1100, 950,
                constant=unreal.LinearColor(1, 1, 1, 1))
    somma = _op(m, unreal.MaterialExpressionDotProduct, pw, uno, -950, 900)
    wn = _op(m, unreal.MaterialExpressionDivide, pw, somma, -800, 800)

    colore = None
    for i, (r, g, b) in enumerate(((True, False, False), (False, True, False), (False, False, True))):
        w = _maschera(m, wn, r, g, b, -650, 700 + i * 120)
        rgb = _nodo(m, unreal.MaterialExpressionComponentMask, -800, -300 + i * 300, r=True, g=True, b=True, a=False)
        _collega(proiezioni[i], "RGB", rgb, "")
        parte = _op(m, unreal.MaterialExpressionMultiply, rgb, w, -500, -300 + i * 300)
        colore = parte if colore is None else _op(m, unreal.MaterialExpressionAdd, colore, parte, -350, -150 + i * 150)

    tinta = _vettore(m, "Tinta", (1, 1, 1), -350, 300)
    luce = _scalare(m, "Luminosita", 1.0, -350, 450)
    c1 = _op(m, unreal.MaterialExpressionMultiply, colore, tinta, -150, 0)
    c2 = _op(m, unreal.MaterialExpressionMultiply, c1, luce, 0, 0)
    _uscita(c2, "", unreal.MaterialProperty.MP_BASE_COLOR)
    _uscita(_scalare(m, "Rugosita", 0.75, 0, 200), "", unreal.MaterialProperty.MP_ROUGHNESS)
    _uscita(_scalare(m, "Metallico", 0.0, 0, 300), "", unreal.MaterialProperty.MP_METALLIC)
    _uscita(_scalare(m, "Speculare", 0.4, 0, 400), "", unreal.MaterialProperty.MP_SPECULAR)
    mel.recompile_material(m)
    _salva(m)
    return m


def crea_colore():
    """Un colore pieno con rugosità, metallo ed emissivo (per i pezzi semplici)."""
    m = _materiale("M_AIV_Colore")
    col = _vettore(m, "Colore", (0.5, 0.5, 0.5), -500, 0)
    _uscita(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    _uscita(_scalare(m, "Rugosita", 0.6, -500, 150), "", unreal.MaterialProperty.MP_ROUGHNESS)
    _uscita(_scalare(m, "Metallico", 0.0, -500, 250), "", unreal.MaterialProperty.MP_METALLIC)
    em = _scalare(m, "Intensita", 0.0, -500, 350)
    emc = _op(m, unreal.MaterialExpressionMultiply, col, em, -250, 350)
    _uscita(emc, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m)
    _salva(m)
    return m


def crea_lava():
    """UE3, l'esercizio della lava: Noise sulla posizione nel mondo che scorre col Time
    (come un Panner), due colori mescolati con un Lerp, molto emissivo."""
    m = _materiale("M_AIV_Lava")
    wp = _nodo(m, unreal.MaterialExpressionWorldPosition, -1400, 0)
    t = _nodo(m, unreal.MaterialExpressionTime, -1400, 200)
    vel = _vettore(m, "Velocita", (20.0, 8.0, 0.0), -1400, 300)
    spost = _op(m, unreal.MaterialExpressionMultiply, t, vel, -1200, 250)
    pos = _op(m, unreal.MaterialExpressionAdd, wp, spost, -1050, 100)
    noise = _nodo(m, unreal.MaterialExpressionNoise, -850, 100, scale=0.004, levels=4,
                  output_min=0.0, output_max=1.0, turbulence=True)
    _collega(pos, "", noise, "Position")
    c1 = _vettore(m, "ColoreScuro", (0.25, 0.02, 0.0), -850, -200)
    c2 = _vettore(m, "ColoreChiaro", (1.0, 0.45, 0.05), -850, -50)
    lerp = _nodo(m, unreal.MaterialExpressionLinearInterpolate, -600, 0)
    _collega(c1, "", lerp, "A")
    _collega(c2, "", lerp, "B")
    _collega(noise, "", lerp, "Alpha")
    inten = _scalare(m, "Intensita", 12.0, -600, 200)
    em = _op(m, unreal.MaterialExpressionMultiply, lerp, inten, -400, 100)
    _uscita(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    _uscita(lerp, "", unreal.MaterialProperty.MP_BASE_COLOR)
    _uscita(_scalare(m, "Rugosita", 0.9, -400, 300), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(m)
    _salva(m)
    return m


def crea_decal():
    """UE16: un materiale da decal. Una macchia rotonda e sfumata (bruciatura,
    pozzanghera, alone): il bordo sfuma con la distanza dal centro delle UV."""
    m = _materiale("M_AIV_Decal")
    try:
        m.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
        m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    except Exception as ex:  # noqa: BLE001
        _avviso("decal: %s" % ex)
    uv = _nodo(m, unreal.MaterialExpressionTextureCoordinate, -900, 0)
    centro = _nodo(m, unreal.MaterialExpressionConstant2Vector, -900, 150, r=0.5, g=0.5)
    dist = _op(m, unreal.MaterialExpressionDistance, uv, centro, -700, 50)
    due = _nodo(m, unreal.MaterialExpressionMultiply, -550, 50, const_b=2.0)
    _collega(dist, "", due, "A")
    inv = _nodo(m, unreal.MaterialExpressionOneMinus, -400, 50)
    _collega(due, "", inv, "")
    sat = _nodo(m, unreal.MaterialExpressionSaturate, -300, 50)
    _collega(inv, "", sat, "")
    morbidezza = _scalare(m, "Durezza", 1.5, -300, 200)
    pw = _op(m, unreal.MaterialExpressionPower, sat, morbidezza, -150, 50, ia="Base", ib="Exponent")
    opac = _scalare(m, "Opacita", 0.85, -150, 200)
    op = _op(m, unreal.MaterialExpressionMultiply, pw, opac, 0, 100)
    _uscita(op, "", unreal.MaterialProperty.MP_OPACITY)
    col = _vettore(m, "Colore", (0.02, 0.015, 0.01), 0, -100)
    _uscita(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    _uscita(_scalare(m, "Rugosita", 0.9, 0, 300), "", unreal.MaterialProperty.MP_ROUGHNESS)
    em = _scalare(m, "Intensita", 0.0, 0, 400)
    emc = _op(m, unreal.MaterialExpressionMultiply, col, em, 200, 400)
    _uscita(emc, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m)
    _salva(m)
    return m


# ---------------------------------------------------------------------
# le istanze
# ---------------------------------------------------------------------

def istanza(nome, padre, scalari=None, vettori=None, texture=None, fisico=None):
    mi, _ = _nuovo_asset(nome, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, padre)
    for k, v in (scalari or {}).items():
        mel.set_material_instance_scalar_parameter_value(mi, k, float(v))
    for k, v in (vettori or {}).items():
        mel.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
    for k, v in (texture or {}).items():
        t = _carica(v)
        if t is not None:
            mel.set_material_instance_texture_parameter_value(mi, k, t)
    if fisico:
        pm = _carica(fisico)
        if pm is not None:
            try:
                mi.set_editor_property("phys_material", pm)   # UE10: i passi cambiano suono
            except Exception as ex:  # noqa: BLE001
                _avviso("phys_material: %s" % ex)
    mel.update_material_instance(mi)
    _salva(mi)
    return mi


_cache = {}


def tutti(ricostruisci=False):
    """Crea (o riusa) tutti i materiali. Restituisce nome -> istanza."""
    if _cache and not ricostruisci:
        return _cache
    tri = crea_triplanare()
    col = crea_colore()
    lava = crea_lava()
    dec = crea_decal()

    def tri_mi(nome, tex, dim, tinta=(1, 1, 1), lum=1.0, rug=0.75, fisico=PM_ROCCIA):
        return istanza("MI_AIV_" + nome, tri, {"Dimensione": dim, "Luminosita": lum, "Rugosita": rug},
                       {"Tinta": tinta}, {"Diffuse": TEX[tex]}, fisico)

    def col_mi(nome, rgb, rug=0.6, met=0.0, em=0.0):
        return istanza("MI_AIV_" + nome, col, {"Rugosita": rug, "Metallico": met, "Intensita": em},
                       {"Colore": rgb})

    m = _cache
    # --- per la stanza (GD14) ---
    m["Assi"] = tri_mi("Assi", "legno", 120, (0.95, 0.78, 0.62), 0.9, 0.6)           # pavimento di assi
    m["Travi"] = tri_mi("Travi", "corteccia", 160, (0.55, 0.40, 0.30), 0.8, 0.85)    # travi a vista
    m["LegnoScuro"] = tri_mi("LegnoScuro", "legno", 90, (0.50, 0.33, 0.22), 0.8, 0.55)
    m["Intonaco"] = tri_mi("Intonaco", "intonaco", 220, (1.0, 0.86, 0.62), 1.0, 0.9)  # muri caldi, ocra
    m["IntonacoChiaro"] = tri_mi("IntonacoChiaro", "intonaco", 220, (1.0, 0.95, 0.86), 1.0, 0.9)
    m["Pietra"] = tri_mi("Pietra", "quarzo", 70, (0.85, 0.72, 0.62), 0.9, 0.8)       # camino
    m["Piastrelle"] = tri_mi("Piastrelle", "piastrelle", 120, (1, 1, 1), 1.0, 0.7)
    m["TessutoRosso"] = tri_mi("TessutoRosso", "tessuto", 60, (0.75, 0.16, 0.10), 1.0, 0.95)
    m["TessutoOcra"] = tri_mi("TessutoOcra", "tessuto", 60, (0.95, 0.62, 0.18), 1.0, 0.95)
    m["TessutoBlu"] = tri_mi("TessutoBlu", "tessuto", 60, (0.20, 0.30, 0.60), 1.0, 0.95)
    m["LegnoDipinto"] = tri_mi("LegnoDipinto", "legno", 100, (0.32, 0.42, 0.50), 0.9, 0.6)  # credenza
    m["Rattan"] = tri_mi("Rattan", "rattan", 60, (1, 0.9, 0.75), 1.0, 0.8)
    m["Prato"] = tri_mi("Prato", "erba", 400, (0.55, 0.65, 0.5), 0.6, 0.95)
    m["Braci"] = istanza("MI_AIV_Braci", lava, {"Intensita": 8.0},
                         {"ColoreScuro": (0.15, 0.01, 0.0), "ColoreChiaro": (1.0, 0.35, 0.04),
                          "Velocita": (0.0, 0.0, 6.0)})
    m["Fiamma"] = istanza("MI_AIV_Fiamma", lava, {"Intensita": 25.0},
                          {"ColoreScuro": (0.9, 0.18, 0.0), "ColoreChiaro": (1.0, 0.75, 0.25),
                           "Velocita": (0.0, 0.0, 40.0)})
    m["Ferro"] = col_mi("Ferro", (0.03, 0.03, 0.03), 0.45, 1.0)
    m["Ottone"] = col_mi("Ottone", (0.75, 0.55, 0.25), 0.3, 1.0)
    m["LuceCalda"] = col_mi("LuceCalda", (1.0, 0.65, 0.3), 0.5, 0.0, 6.0)
    m["Fuliggine"] = istanza("MI_AIV_Fuliggine", dec, {"Opacita": 0.8, "Durezza": 1.2}, {"Colore": (0.01, 0.008, 0.006)})
    m["AloneFuoco"] = istanza("MI_AIV_AloneFuoco", dec, {"Opacita": 0.35, "Durezza": 2.0, "Intensita": 0.6},
                              {"Colore": (1.0, 0.45, 0.12)})

    # --- per il platformer (GD13) ---
    m["Percorso"] = tri_mi("Percorso", "griglia", 100, (0.62, 0.62, 0.64), 1.0, 0.7)
    m["PercorsoScuro"] = tri_mi("PercorsoScuro", "griglia", 100, (0.33, 0.34, 0.37), 1.0, 0.7)
    m["Salto"] = tri_mi("Salto", "griglia", 100, (1.0, 0.55, 0.12), 1.0, 0.6)        # arancio: interattivo
    m["Muro"] = tri_mi("Muro", "griglia", 100, (0.25, 0.55, 1.0), 1.0, 0.6)          # blu: wall run
    m["Pericolo"] = tri_mi("Pericolo", "griglia", 100, (0.95, 0.18, 0.15), 1.0, 0.6)
    m["Traguardo"] = tri_mi("Traguardo", "griglia", 100, (0.2, 0.85, 0.35), 1.0, 0.6)
    m["Ghiaccio"] = tri_mi("Ghiaccio", "quarzo", 120, (0.70, 0.88, 1.0), 1.1, 0.08, PM_ACQUA)
    m["Erba"] = tri_mi("Erba", "erba", 500, (0.8, 0.9, 0.75), 0.8, 0.95)
    m["LegnoChiaro"] = tri_mi("LegnoChiaro", "legno", 120, (1.0, 0.85, 0.7), 1.0, 0.6)
    m["Acqua"] = istanza("MI_AIV_Acqua", col, {"Rugosita": 0.05, "Metallico": 0.0, "Intensita": 0.0},
                         {"Colore": (0.02, 0.08, 0.12)}, fisico=PM_ACQUA)   # i passi "sguazzano" (UE10)
    m["Lava"] = istanza("MI_AIV_Lava", lava, {"Intensita": 14.0})
    m["Moneta"] = col_mi("Moneta", (1.0, 0.72, 0.15), 0.25, 1.0, 0.4)
    m["Abilita"] = col_mi("Abilita", (0.25, 0.65, 1.0), 0.3, 0.0, 4.0)
    m["Vita"] = col_mi("Vita", (0.2, 1.0, 0.35), 0.3, 0.0, 3.0)
    m["Torretta"] = col_mi("Torretta", (0.18, 0.18, 0.2), 0.35, 0.8)
    m["Proiettile"] = col_mi("Proiettile", (1.0, 0.25, 0.1), 0.4, 0.0, 20.0)
    m["Acceso"] = col_mi("Acceso", (0.2, 1.0, 0.35), 0.4, 0.0, 5.0)
    m["Spento"] = col_mi("Spento", (0.6, 0.15, 0.1), 0.4, 0.0, 1.0)
    m["Bruciatura"] = istanza("MI_AIV_Bruciatura", dec, {"Opacita": 0.9, "Durezza": 1.0}, {"Colore": (0.01, 0.008, 0.006)})
    m["Pozza"] = istanza("MI_AIV_Pozza", dec, {"Opacita": 0.7, "Durezza": 0.6}, {"Colore": (0.02, 0.03, 0.04)})
    _log("materiali pronti: %d" % len(m))
    return m


if __name__ == "__main__":
    tutti(True)
