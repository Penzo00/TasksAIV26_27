# -*- coding: utf-8 -*-
"""
Attrezzi comuni agli script che costruiscono i livelli dei task
(gd13_platformer.py, gd14_stanza_luci.py).

Non si lancia da solo: lo importano gli altri script.
"""

import unreal

MODULO_CPP = "/Script/AIV_UE5Intro."
GAME_MODE = MODULO_CPP + "PlatformerGameMode"
CUBO = "/Engine/BasicShapes/Cube"
CILINDRO = "/Engine/BasicShapes/Cylinder"

attori = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
livelli = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def log(msg):
    unreal.log("[AIV] " + msg)


def avviso(msg):
    unreal.log_warning("[AIV] " + msg)


def carica(percorso):
    """Carica un asset (o il primo esistente di una lista). None se manca."""
    lista = percorso if isinstance(percorso, (list, tuple)) else [percorso]
    for p in lista:
        if unreal.EditorAssetLibrary.does_asset_exist(p):
            return unreal.EditorAssetLibrary.load_asset(p)
    avviso("asset mancante: %s" % (percorso,))
    return None


def classe_cpp(nome):
    """Una classe C++ del progetto, per esempio classe_cpp('KillZone')."""
    c = unreal.load_class(None, MODULO_CPP + nome)
    if c is None:
        avviso("classe C++ non trovata: %s (il progetto è compilato?)" % nome)
    return c


def imposta(oggetto, proprieta, valore):
    """set_editor_property che non blocca lo script se qualcosa non va."""
    try:
        oggetto.set_editor_property(proprieta, valore)
        return True
    except Exception as e:  # noqa: BLE001
        avviso("non riesco a impostare %s = %r (%s)" % (proprieta, valore, e))
        return False


def rot(pitch=0.0, yaw=0.0, roll=0.0):
    return unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw)


def vec(x, y, z):
    return unreal.Vector(float(x), float(y), float(z))


def nomina(attore, nome, cartella):
    attore.set_actor_label(nome)
    attore.set_folder_path(cartella)
    return attore


def nuovo_livello(percorso):
    """Crea un livello vuoto in percorso, cancellando quello vecchio se c'è."""
    if unreal.EditorAssetLibrary.does_asset_exist(percorso):
        if not unreal.EditorAssetLibrary.delete_asset(percorso):
            avviso("non riesco a cancellare %s: se è aperto, apri un altro livello e rilancia." % percorso)
            return False
    if not livelli.new_level(percorso):
        avviso("new_level ha fallito per " + percorso)
        return False
    return True


def blocco(nome, cartella, centro, dimensioni, materiale=None, yaw=0.0):
    """Un parallelepipedo: centro (x, y, z) e dimensioni (dx, dy, dz) in cm."""
    cubo = carica(CUBO)
    a = attori.spawn_actor_from_object(cubo, vec(*centro), rot(yaw=yaw))
    a.set_actor_scale3d(vec(dimensioni[0] / 100.0, dimensioni[1] / 100.0, dimensioni[2] / 100.0))
    if materiale is not None:
        a.static_mesh_component.set_material(0, materiale)
    return nomina(a, nome, cartella)


def pedana(nome, cartella, x, y, z_sopra, dx, dy, spessore=40.0, materiale=None):
    """Una piattaforma fissa: (x, y) è il centro, z_sopra la quota su cui si cammina."""
    return blocco(nome, cartella, (x, y, z_sopra - spessore / 2.0), (dx, dy, spessore), materiale)


def scritta(testo, pos, yaw=0.0, dimensione=60.0, colore=(1.0, 1.0, 1.0)):
    a = attori.spawn_actor_from_class(unreal.TextRenderActor, vec(*pos), rot(yaw=yaw))
    c = a.get_component_by_class(unreal.TextRenderComponent)
    c.set_text(unreal.Text(testo))
    c.set_world_size(dimensione)
    c.set_text_render_color(unreal.Color(int(colore[0] * 255), int(colore[1] * 255),
                                         int(colore[2] * 255), 255))
    imposta(c, "horizontal_alignment", unreal.HorizTextAligment.EHTA_CENTER)
    return nomina(a, "Scritta_" + testo[:20].replace(" ", "_"), "Scritte")


def spawn_cpp(nome_classe, nome, cartella, pos, yaw=0.0, **proprieta):
    """Piazza un actor C++ del progetto e ne imposta le proprietà (nomi in snake_case)."""
    c = classe_cpp(nome_classe)
    if c is None:
        return None
    a = attori.spawn_actor_from_class(c, vec(*pos), rot(yaw=yaw))
    for k, v in proprieta.items():
        if k == "label":
            v = unreal.Text(v)
        imposta(a, k, v)
    # riapplica le dimensioni ai componenti (OnConstruction)
    for metodo in ("apply_zone_settings", "apply_platform_size", "apply_bar_size"):
        if hasattr(a, metodo):
            getattr(a, metodo)()
    return nomina(a, nome, cartella)


def luce_esterna(sole_pitch=-40.0, sole_yaw=-30.0, sole_lux=8.0):
    """Sole + cielo + atmosfera + nebbia: il minimo per un livello all'aperto."""
    sole = attori.spawn_actor_from_class(unreal.DirectionalLight, vec(0, 0, 800),
                                         rot(pitch=sole_pitch, yaw=sole_yaw))
    c = sole.get_component_by_class(unreal.DirectionalLightComponent)
    c.set_intensity(sole_lux)
    imposta(c, "atmosphere_sun_light", True)
    nomina(sole, "Sole", "Luci")
    nomina(attori.spawn_actor_from_class(unreal.SkyAtmosphere, vec(0, 0, 0), rot()), "Atmosfera", "Luci")
    cielo = attori.spawn_actor_from_class(unreal.SkyLight, vec(0, 0, 900), rot())
    imposta(cielo.get_component_by_class(unreal.SkyLightComponent), "real_time_capture", True)
    nomina(cielo, "Cielo", "Luci")
    nomina(attori.spawn_actor_from_class(unreal.ExponentialHeightFog, vec(0, 0, -200), rot()),
           "Nebbia", "Luci")


def rendi_giocabile(pos_partenza, yaw_partenza=0.0):
    """PlayerStart + GameMode del progetto (che usa BP_Bot2) nel livello aperto."""
    ps = attori.spawn_actor_from_class(unreal.PlayerStart, vec(*pos_partenza), rot(yaw=yaw_partenza))
    nomina(ps, "Partenza", "Gioco")
    gm = classe_cpp("PlatformerGameMode")
    if gm is None:
        return
    mondo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    impostazioni = None
    try:
        impostazioni = mondo.get_world_settings()
    except Exception:  # noqa: BLE001
        trovati = unreal.GameplayStatics.get_all_actors_of_class(mondo, unreal.WorldSettings)
        impostazioni = trovati[0] if trovati else None
    if impostazioni is None or not imposta(impostazioni, "default_game_mode", gm):
        avviso("imposta a mano: World Settings > GameMode Override = PlatformerGameMode")


def salva():
    livelli.save_current_level()
