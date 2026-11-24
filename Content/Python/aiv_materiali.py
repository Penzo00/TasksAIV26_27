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


def crea_mattoni():
    """Mattoni PROCEDURALI (nel progetto non c'è una texture di mattoni):
    righe sfalsate di mezzo mattone, fuga di malta, ogni mattone con un colore
    un po' diverso (un numero "a caso" calcolato dalla sua riga e colonna).
    Le coordinate vengono dal mondo, come nei materiali World Aligned."""
    m = _materiale("M_AIV_Mattoni")
    M_ = unreal
    wp = _nodo(m, M_.MaterialExpressionWorldPosition, -2600, 0)
    n = _nodo(m, M_.MaterialExpressionVertexNormalWS, -2600, 300)
    an = _nodo(m, M_.MaterialExpressionAbs, -2450, 300)
    _collega(n, "", an, "")
    px = _maschera(m, wp, True, False, False, -2300, -100)
    py = _maschera(m, wp, False, True, False, -2300, 0)
    pz = _maschera(m, wp, False, False, True, -2300, 100)
    nx = _maschera(m, an, True, False, False, -2300, 250)
    ny = _maschera(m, an, False, True, False, -2300, 350)
    nz = _maschera(m, an, False, False, True, -2300, 450)
    # orizzontale: Y sui muri che guardano X, X sui muri che guardano Y (e sopra)
    u1 = _op(m, M_.MaterialExpressionMultiply, py, nx, -2100, -50)
    u2 = _op(m, M_.MaterialExpressionMultiply, px, ny, -2100, 50)
    u3 = _op(m, M_.MaterialExpressionMultiply, px, nz, -2100, 150)
    ub = _op(m, M_.MaterialExpressionAdd, _op(m, M_.MaterialExpressionAdd, u1, u2, -1950, 0), u3, -1800, 50)
    # verticale: Z sui muri, Y sulle facce orizzontali
    unz = _nodo(m, M_.MaterialExpressionOneMinus, -2100, 300)
    _collega(nz, "", unz, "")
    v1 = _op(m, M_.MaterialExpressionMultiply, pz, unz, -1950, 300)
    v2 = _op(m, M_.MaterialExpressionMultiply, py, nz, -1950, 400)
    vb = _op(m, M_.MaterialExpressionAdd, v1, v2, -1800, 350)

    bw = _scalare(m, "Larghezza", 24.0, -1800, -150)
    bh = _scalare(m, "Altezza", 8.0, -1800, 500)
    u = _op(m, M_.MaterialExpressionDivide, ub, bw, -1650, 0)
    v = _op(m, M_.MaterialExpressionDivide, vb, bh, -1650, 350)
    riga = _nodo(m, M_.MaterialExpressionFloor, -1500, 350)
    _collega(v, "", riga, "")
    meta = _nodo(m, M_.MaterialExpressionMultiply, -1350, 250, const_b=0.5)
    _collega(riga, "", meta, "A")
    sfalsa = _nodo(m, M_.MaterialExpressionFrac, -1200, 250)          # 0 o 0.5
    _collega(meta, "", sfalsa, "")
    u_s = _op(m, M_.MaterialExpressionAdd, u, sfalsa, -1050, 0)
    colonna = _nodo(m, M_.MaterialExpressionFloor, -900, -150)
    _collega(u_s, "", colonna, "")
    fu = _nodo(m, M_.MaterialExpressionFrac, -900, 0)
    _collega(u_s, "", fu, "")
    fv = _nodo(m, M_.MaterialExpressionFrac, -1350, 450)
    _collega(v, "", fv, "")

    def bordo(f, dim, x, y):
        inv = _nodo(m, M_.MaterialExpressionOneMinus, x, y)
        _collega(f, "", inv, "")
        mn = _op(m, M_.MaterialExpressionMin, f, inv, x + 150, y)
        return _op(m, M_.MaterialExpressionMultiply, mn, dim, x + 300, y)   # distanza dal bordo in cm

    eu = bordo(fu, bw, -750, 0)
    ev = bordo(fv, bh, -1200, 450)
    e = _op(m, M_.MaterialExpressionMin, eu, ev, -400, 200)
    malta = _scalare(m, "Malta", 0.9, -400, 350)
    maschera = _nodo(m, M_.MaterialExpressionSaturate, -100, 250)
    _collega(_op(m, M_.MaterialExpressionDivide, e, malta, -250, 250), "", maschera, "")

    # un numero "a caso" per mattone: frac(sin(colonna*12.99 + riga*78.23) * 43758)
    hc = _nodo(m, M_.MaterialExpressionMultiply, -750, -250, const_b=12.9898)
    _collega(colonna, "", hc, "A")
    hr = _nodo(m, M_.MaterialExpressionMultiply, -750, -350, const_b=78.233)
    _collega(riga, "", hr, "A")
    hs = _nodo(m, M_.MaterialExpressionSine, -450, -300, period=6.2831853)
    _collega(_op(m, M_.MaterialExpressionAdd, hc, hr, -600, -300), "", hs, "")
    hm = _nodo(m, M_.MaterialExpressionMultiply, -300, -300, const_b=43758.5453)
    _collega(hs, "", hm, "A")
    caso = _nodo(m, M_.MaterialExpressionFrac, -150, -300)
    _collega(hm, "", caso, "")

    ca = _vettore(m, "ColoreA", (0.55, 0.16, 0.07), -150, -550)
    cb = _vettore(m, "ColoreB", (0.85, 0.42, 0.18), -150, -450)
    mattone = _nodo(m, M_.MaterialExpressionLinearInterpolate, 50, -400)
    _collega(ca, "", mattone, "A")
    _collega(cb, "", mattone, "B")
    _collega(caso, "", mattone, "Alpha")
    cm = _vettore(m, "ColoreMalta", (0.55, 0.45, 0.35), 50, -150)
    finale = _nodo(m, M_.MaterialExpressionLinearInterpolate, 250, 0)
    _collega(cm, "", finale, "A")
    _collega(mattone, "", finale, "B")
    _collega(maschera, "", finale, "Alpha")
    _uscita(finale, "", M_.MaterialProperty.MP_BASE_COLOR)
    # la malta è più ruvida e "incassata" (meno speculare)
    rug = _nodo(m, M_.MaterialExpressionLinearInterpolate, 250, 200, const_a=0.95, const_b=0.8)
    _collega(maschera, "", rug, "Alpha")
    _uscita(rug, "", M_.MaterialProperty.MP_ROUGHNESS)
    em = _scalare(m, "Intensita", 0.0, 250, 400)
    _uscita(_op(m, M_.MaterialExpressionMultiply, finale, em, 450, 400), "", M_.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m)
    _salva(m)
    return m


def crea_carta():
    """Carta da parati PROCEDURALE: un motivo a rombi su una griglia sfalsata,
    proiettato dal mondo come i mattoni (stessa misura su ogni muro)."""
    m = _materiale("M_AIV_CartaParati")
    M_ = unreal
    wp = _nodo(m, M_.MaterialExpressionWorldPosition, -2200, 0)
    n = _nodo(m, M_.MaterialExpressionVertexNormalWS, -2200, 300)
    an = _nodo(m, M_.MaterialExpressionAbs, -2050, 300)
    _collega(n, "", an, "")
    px = _maschera(m, wp, True, False, False, -1900, -100)
    py = _maschera(m, wp, False, True, False, -1900, 0)
    pz = _maschera(m, wp, False, False, True, -1900, 100)
    nx = _maschera(m, an, True, False, False, -1900, 250)
    ny = _maschera(m, an, False, True, False, -1900, 350)
    # orizzontale: Y sui muri che guardano X, X su quelli che guardano Y
    ub = _op(m, M_.MaterialExpressionAdd, _op(m, M_.MaterialExpressionMultiply, py, nx, -1750, -50),
             _op(m, M_.MaterialExpressionMultiply, px, ny, -1750, 50), -1600, 0)
    passo = _scalare(m, "Passo", 32.0, -1600, 200)
    u = _op(m, M_.MaterialExpressionDivide, ub, passo, -1450, 0)
    v = _op(m, M_.MaterialExpressionDivide, pz, passo, -1450, 150)
    riga = _nodo(m, M_.MaterialExpressionFloor, -1300, 250)
    _collega(v, "", riga, "")
    meta = _nodo(m, M_.MaterialExpressionMultiply, -1150, 250, const_b=0.5)
    _collega(riga, "", meta, "A")
    sfalsa = _nodo(m, M_.MaterialExpressionFrac, -1000, 250)
    _collega(meta, "", sfalsa, "")
    u_s = _op(m, M_.MaterialExpressionAdd, u, sfalsa, -850, 0)

    def centrato(f_in, x, y):
        f = _nodo(m, M_.MaterialExpressionFrac, x, y)
        _collega(f_in, "", f, "")
        d = _nodo(m, M_.MaterialExpressionSubtract, x + 150, y, const_b=0.5)
        _collega(f, "", d, "A")
        ab = _nodo(m, M_.MaterialExpressionAbs, x + 300, y)
        _collega(d, "", ab, "")
        return ab

    du = centrato(u_s, -700, 0)
    dv = centrato(v, -700, 150)
    rombo = _op(m, M_.MaterialExpressionAdd, du, dv, -250, 80)        # 0 al centro, 1 negli angoli
    dim = _scalare(m, "Motivo", 0.22, -250, 250)
    diff = _op(m, M_.MaterialExpressionSubtract, rombo, dim, -100, 120)
    nitido = _nodo(m, M_.MaterialExpressionMultiply, 50, 120, const_b=30.0)
    _collega(diff, "", nitido, "A")
    maschera = _nodo(m, M_.MaterialExpressionSaturate, 200, 120)      # 0 = motivo, 1 = fondo
    _collega(nitido, "", maschera, "")
    fondo = _vettore(m, "ColoreFondo", (0.85, 0.26, 0.07), 200, -200)
    motivo = _vettore(m, "ColoreMotivo", (1.0, 0.62, 0.18), 200, -100)
    finale = _nodo(m, M_.MaterialExpressionLinearInterpolate, 400, 0)
    _collega(motivo, "", finale, "A")
    _collega(fondo, "", finale, "B")
    _collega(maschera, "", finale, "Alpha")
    # un poco di "carta": la texture dell'intonaco scurisce appena il colore
    tex = _carica(TEX["intonaco"])
    if tex is not None:
        uv = _op(m, M_.MaterialExpressionAppendVector, u, v, -700, 400)
        campione = _nodo(m, M_.MaterialExpressionTextureSample, -500, 400, texture=tex)
        _collega(uv, "", campione, "UVs")
        grana = _nodo(m, M_.MaterialExpressionLinearInterpolate, 200, 400, const_a=0.8, const_b=1.0)
        _collega(campione, "R", grana, "Alpha")
        finale = _op(m, M_.MaterialExpressionMultiply, finale, grana, 550, 150)
    _uscita(finale, "", M_.MaterialProperty.MP_BASE_COLOR)
    _uscita(_nodo(m, M_.MaterialExpressionConstant, 550, 350, r=0.85), "", M_.MaterialProperty.MP_ROUGHNESS)
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
    mat_mattoni = crea_mattoni()
    mat_carta = crea_carta()

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
    # --- la baita colorata (stanza GD14, ispirata alla foto di riferimento) ---
    m["IntonacoArancio"] = tri_mi("IntonacoArancio", "intonaco", 260, (1.0, 0.52, 0.16), 1.15, 0.9)
    m["Perline"] = tri_mi("Perline", "legno", 140, (0.85, 0.55, 0.32), 0.85, 0.6)       # tavole del tetto
    m["LegnoViola"] = tri_mi("LegnoViola", "legno", 100, (0.32, 0.28, 0.75), 0.9, 0.55)  # credenze dipinte
    m["TessutoVerde"] = tri_mi("TessutoVerde", "tessuto", 60, (0.12, 0.62, 0.15), 1.1, 0.95)
    m["TessutoGiallo"] = tri_mi("TessutoGiallo", "tessuto", 60, (1.0, 0.82, 0.12), 1.1, 0.95)
    m["Mattoni"] = istanza("MI_AIV_Mattoni", mat_mattoni, {"Larghezza": 24.0, "Altezza": 8.0, "Malta": 0.9})
    m["MattoniScuri"] = istanza("MI_AIV_MattoniScuri", mat_mattoni, {"Larghezza": 20.0, "Altezza": 7.0},
                                {"ColoreA": (0.12, 0.04, 0.02), "ColoreB": (0.3, 0.12, 0.06),
                                 "ColoreMalta": (0.06, 0.05, 0.04)})
    m["CartaParati"] = istanza("MI_AIV_CartaParati", mat_carta, {"Passo": 32.0, "Motivo": 0.22})
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
