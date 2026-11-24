# -*- coding: utf-8 -*-
"""
Attrezzi comuni agli script che costruiscono i livelli dei task
(gd13_platformer.py, gd14_stanza_luci.py).

Non si lancia da solo: lo importano gli altri script.

Novità rispetto alla prima versione:
  - i blocchi del percorso sono attori del plugin BlockOut Tools
    (Blockout_Box), con il loro materiale a griglia colorato o con un nostro
    materiale (Custom Material). Se il plugin non c'è, si torna al cubo
    dell'engine: lo script non si blocca.
  - luci sempre Movable (il progetto ha r.AllowStaticLighting=False: Lumen)
  - post process, decal, suoni, spline, tag per il wall run
"""

import unreal

MODULO_CPP = "/Script/AIV_UE5Intro."
GAME_MODE = MODULO_CPP + "PlatformerGameMode"
CUBO = "/Engine/BasicShapes/Cube"
CILINDRO = "/Engine/BasicShapes/Cylinder"
SFERA = "/Engine/BasicShapes/Sphere"
PIANO = "/Engine/BasicShapes/Plane"

BLOCKOUT_BOX = "/BlockoutToolsPlugin/Blueprints/Blockout_Box.Blockout_Box_C"
USA_BLOCKOUT = True          # False = solo cubi dell'engine

TAG_WALLRUN = "WallRun"      # deve essere uguale a WallTag nel componente delle abilità

# Colori della "grammatica" del gym (UE14): ogni colore ha un significato.
COLORI = {
    "percorso": ((0.55, 0.56, 0.58), (0.80, 0.80, 0.82)),   # (lati, faccia sopra)
    "percorso2": ((0.30, 0.31, 0.34), (0.55, 0.56, 0.60)),
    "salto": ((1.00, 0.45, 0.08), (1.00, 0.62, 0.25)),      # arancio: si interagisce / si salta
    "muro": ((0.15, 0.40, 1.00), (0.30, 0.55, 1.00)),       # blu: wall run
    "pericolo": ((0.90, 0.12, 0.10), (1.00, 0.30, 0.25)),
    "traguardo": ((0.15, 0.75, 0.30), (0.35, 0.95, 0.45)),
    "riparo": ((0.45, 0.40, 0.35), (0.60, 0.55, 0.50)),
}

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


def imposta(oggetto, proprieta, valore, silenzioso=False):
    """set_editor_property che non blocca lo script se qualcosa non va."""
    try:
        oggetto.set_editor_property(proprieta, valore)
        return True
    except Exception as e:  # noqa: BLE001
        if not silenzioso:
            avviso("non riesco a impostare %s = %r (%s)" % (proprieta, valore, e))
        return False


def rot(pitch=0.0, yaw=0.0, roll=0.0):
    return unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw)


def vec(x, y, z):
    return unreal.Vector(float(x), float(y), float(z))


def colore(rgb, a=1.0):
    return unreal.LinearColor(float(rgb[0]), float(rgb[1]), float(rgb[2]), float(a))


def nomina(attore, nome, cartella):
    if attore is None:
        return None
    attore.set_actor_label(nome)
    attore.set_folder_path(cartella)
    return attore


def nuovo_livello(percorso):
    """Crea un livello vuoto in percorso. Il livello vecchio NON viene
    cancellato: viene rinominato in <nome>_Backup (quello di prima ancora
    viene sostituito). Se qualcosa va storto, il backup resta lì."""
    eal = unreal.EditorAssetLibrary
    # prima si apre una mappa vuota e temporanea: il livello da sostituire
    # non deve essere aperto mentre lo si rinomina
    try:
        unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    except Exception as e:  # noqa: BLE001
        avviso("non riesco ad aprire una mappa vuota: %s" % e)
    backup = percorso + "_Backup"
    if eal.does_asset_exist(percorso):
        if eal.does_asset_exist(backup) and not eal.delete_asset(backup):
            avviso("non riesco a togliere il vecchio backup %s: lo script si ferma qui." % backup)
            return False
        if not eal.rename_asset(percorso, backup):
            avviso("non riesco a mettere da parte %s: lo script si ferma qui, il livello è intatto." % percorso)
            return False
        log("il livello di prima ora è in " + backup)
    if livelli.new_level(percorso):
        log("creato il livello " + percorso)
        return True
    # piano B: una mappa vuota salvata con il nome giusto
    avviso("new_level ha fallito per %s: provo con una mappa vuota salvata con quel nome" % percorso)
    try:
        mondo = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
        if mondo is not None and unreal.EditorLoadingAndSavingUtils.save_map(mondo, percorso):
            log("creato il livello " + percorso + " (mappa vuota)")
            return True
    except Exception as e:  # noqa: BLE001
        avviso("anche il piano B ha fallito: %s" % e)
    if eal.does_asset_exist(backup) and not eal.does_asset_exist(percorso):
        eal.rename_asset(backup, percorso)
    unreal.log_error("[AIV] non riesco a creare il livello %s: lo script si ferma qui." % percorso)
    return False


ERRORI = []   # le parti che si sono fermate per un errore (vedi sicuro)


def sicuro(funzione, *args, **kwargs):
    """Esegue una funzione che costruisce un livello. Se si ferma per un
    errore, scrive il motivo nel log e salva comunque quello che ha fatto."""
    import traceback
    try:
        return funzione(*args, **kwargs)
    except Exception:  # noqa: BLE001
        ERRORI.append(funzione.__name__)
        unreal.log_error("[AIV] errore in %s:\n%s" % (funzione.__name__, traceback.format_exc()))
        try:
            salva()
            avviso("salvato il livello incompleto; quello di prima è nel _Backup accanto")
        except Exception:  # noqa: BLE001
            pass
        return None


def mobile(attore):
    """Tutte le luci Movable: il progetto usa Lumen, niente luce precalcolata."""
    radice = attore.get_editor_property("root_component")
    if radice is not None:
        radice.set_mobility(unreal.ComponentMobility.MOVABLE)
    return attore


# ---------------------------------------------------------------------
# blocchi (BlockOut Tools, oppure cubo dell'engine)
# ---------------------------------------------------------------------

_classe_blockout = None
_blockout_provato = False


def _blockout():
    global _classe_blockout, _blockout_provato
    if not _blockout_provato:
        _blockout_provato = True
        if USA_BLOCKOUT:
            _classe_blockout = unreal.load_class(None, BLOCKOUT_BOX)
            if _classe_blockout is None:
                avviso("BlockOut Tools non trovato: uso il cubo dell'engine")
    return _classe_blockout


def _tipo_materiale_blockout(personalizzato):
    enum = getattr(unreal, "BlockoutMaterialType", None)
    if enum is None:
        return None
    cerca = "CUSTOM" if personalizzato else "GRID"
    for nome in dir(enum):
        if cerca in nome.upper():
            return getattr(enum, nome)
    return None


def _cubo(centro, dimensioni, yaw, materiale):
    cubo = carica(CUBO)
    a = attori.spawn_actor_from_object(cubo, vec(*centro), rot(yaw=yaw))
    a.set_actor_scale3d(vec(dimensioni[0] / 100.0, dimensioni[1] / 100.0, dimensioni[2] / 100.0))
    if materiale is not None:
        a.static_mesh_component.set_material(0, materiale)
    return a


def _box_blockout(cls, centro, dimensioni, yaw, materiale, tinta):
    a = attori.spawn_actor_from_class(cls, vec(*centro), rot(yaw=yaw))
    if a is None:
        return None
    ok = imposta(a, "box_size", vec(*dimensioni), silenzioso=True)
    if materiale is not None:
        tipo = _tipo_materiale_blockout(True)
        if tipo is not None:
            imposta(a, "blockout_material_type", tipo, silenzioso=True)
        imposta(a, "blockout_custom_material", materiale, silenzioso=True)
    else:
        lati, sopra = COLORI.get(tinta, COLORI["percorso"])
        imposta(a, "blockout_material_color", colore(lati), silenzioso=True)
        imposta(a, "blockout_material_top_color", colore(sopra), silenzioso=True)
        imposta(a, "blockout_material_use_top_color", True, silenzioso=True)
        imposta(a, "blockout_world_aligned", True, silenzioso=True)   # griglia uguale su ogni faccia
    try:
        a.rerun_construction_script()
    except Exception:  # noqa: BLE001
        pass
    if materiale is not None:
        # anche direttamente sulle mesh: se l'enum del plugin avesse un altro
        # nome in Python, il materiale c'è lo stesso
        for c in a.get_components_by_class(unreal.StaticMeshComponent):
            c.set_material(0, materiale)
    # Il pivot del Blockout_Box non è documentato: lo misuro e correggo la
    # posizione così che il CENTRO del blocco sia dove lo voglio.
    origine, estensione = a.get_actor_bounds(True)   # solo le mesh con collisione (no billboard)
    attesa = [d / 2.0 for d in dimensioni]
    if yaw % 180.0 != 0.0:
        attesa = None   # ruotato: i bounds sono allineati agli assi, controllo solo Z
    giusto = ok and abs(estensione.z - dimensioni[2] / 2.0) < 2.0
    if attesa and giusto:
        giusto = abs(estensione.x - attesa[0]) < 2.0 and abs(estensione.y - attesa[1]) < 2.0
    if not giusto:
        a.destroy_actor()
        return None
    a.set_actor_location(a.get_actor_location() + (vec(*centro) - origine), False, False)
    return a


def blocco(nome, cartella, centro, dimensioni, materiale=None, yaw=0.0, tinta="percorso", tag=None):
    """Un parallelepipedo: centro (x, y, z) e dimensioni (dx, dy, dz) in cm.

    materiale: un nostro materiale (Custom Material del Blockout);
    tinta: altrimenti uno dei colori di COLORI (griglia del Blockout).
    tag: per esempio TAG_WALLRUN, per i muri su cui si corre.
    """
    a = None
    cls = _blockout()
    if cls is not None:
        a = _box_blockout(cls, centro, dimensioni, yaw, materiale, tinta)
    if a is None:
        if materiale is None and cls is None:
            materiale = carica("/Game/AIV/Materiali/MI_AIV_Percorso")
        a = _cubo(centro, dimensioni, yaw, materiale)
    if tag:
        aggiungi_tag(a, tag)
    return nomina(a, nome, cartella)


def pedana(nome, cartella, x, y, z_sopra, dx, dy, spessore=40.0, materiale=None, tinta="percorso", tag=None):
    """Una piattaforma fissa: (x, y) è il centro, z_sopra la quota su cui si cammina."""
    return blocco(nome, cartella, (x, y, z_sopra - spessore / 2.0), (dx, dy, spessore),
                  materiale, tinta=tinta, tag=tag)


def mesh(percorso, nome, cartella, pos, yaw=0.0, scala=1.0, materiale=None, pitch=0.0, roll=0.0,
         ombre=True, collisione=True):
    """Una static mesh qualunque (Scan Labs, Starter Content...)."""
    sm = carica(percorso)
    if sm is None:
        return None
    a = attori.spawn_actor_from_object(sm, vec(*pos), rot(pitch, yaw, roll))
    s = scala if isinstance(scala, (list, tuple)) else (scala, scala, scala)
    a.set_actor_scale3d(vec(*s))
    c = a.static_mesh_component
    if materiale is not None:
        for i in range(max(1, c.get_num_materials())):
            c.set_material(i, materiale)
    c.set_cast_shadow(ombre)
    if not collisione:
        c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    return nomina(a, nome, cartella)


# ---------------------------------------------------------------------
# testo, actor C++, suoni
# ---------------------------------------------------------------------

def scritta(testo, pos, yaw=0.0, dimensione=60.0, colore_testo=(1.0, 1.0, 1.0), cartella="Scritte"):
    a = attori.spawn_actor_from_class(unreal.TextRenderActor, vec(*pos), rot(yaw=yaw))
    c = a.get_component_by_class(unreal.TextRenderComponent)
    c.set_text(unreal.Text(testo))
    c.set_world_size(dimensione)
    c.set_text_render_color(unreal.Color(r=int(colore_testo[0] * 255), g=int(colore_testo[1] * 255),
                                         b=int(colore_testo[2] * 255), a=255))
    imposta(c, "horizontal_alignment", unreal.HorizTextAligment.EHTA_CENTER, silenzioso=True)
    return nomina(a, "Scritta_" + testo[:20].replace(" ", "_").replace("\n", "_"), cartella)


# metodi "CallInEditor" degli actor C++ che ricostruiscono i componenti dopo
# aver cambiato le proprietà (fanno quello che fa OnConstruction)
_RICOSTRUISCI = ("apply_zone_settings", "apply_platform_size", "apply_bar_size", "apply_door_size",
                 "apply_pickup_settings", "apply_turret_settings", "apply_windmill_settings",
                 "rebuild_fence", "rebuild", "apply_light_settings")


def ricostruisci(a):
    for metodo in _RICOSTRUISCI:
        if hasattr(a, metodo):
            try:
                getattr(a, metodo)()
            except Exception as e:  # noqa: BLE001
                avviso("%s.%s: %s" % (a.get_actor_label(), metodo, e))


def spawn_cpp(nome_classe, nome, cartella, pos, yaw=0.0, **proprieta):
    """Piazza un actor C++ del progetto e ne imposta le proprietà (nomi in snake_case)."""
    c = classe_cpp(nome_classe)
    if c is None:
        return None
    a = attori.spawn_actor_from_class(c, vec(*pos), rot(yaw=yaw))
    for k, v in proprieta.items():
        if k in ("label", "prompt") and isinstance(v, str):
            v = unreal.Text(v)
        imposta(a, k, v)
    ricostruisci(a)
    return nomina(a, nome, cartella)


def collega(a, proprieta, lista):
    """Imposta un array di riferimenti ad altri actor (Targets, ListenTo...)."""
    lista = [x for x in lista if x is not None]
    if a is not None:
        imposta(a, proprieta, lista)


def suono(nome):
    return carica("/Game/Audio/" + nome)


def aggiungi_tag(attore, nome):
    if attore is not None:
        tags = list(attore.get_editor_property("tags")) + [unreal.Name(nome)]
        attore.set_editor_property("tags", tags)
    return attore


# ---------------------------------------------------------------------
# luci, cielo, post process, decal
# ---------------------------------------------------------------------

def luce_punto(nome, pos, intensita=20.0, temperatura=None, rgb=None, raggio=1000.0, sorgente=5.0,
               ombre=True, cartella="Luci", volumetrica=1.0):
    """Point light Movable in candele. temperatura in Kelvin, oppure rgb."""
    a = attori.spawn_actor_from_class(unreal.PointLight, vec(*pos), rot())
    mobile(a)
    c = a.get_component_by_class(unreal.PointLightComponent)
    imposta(c, "intensity_units", unreal.LightUnits.CANDELAS, silenzioso=True)
    c.set_intensity(intensita)
    c.set_attenuation_radius(raggio)
    c.set_source_radius(sorgente)
    c.set_cast_shadows(ombre)
    imposta(c, "volumetric_scattering_intensity", volumetrica, silenzioso=True)
    if temperatura:
        c.set_use_temperature(True)
        c.set_temperature(temperatura)
    if rgb:
        c.set_light_color(colore(rgb))
    return nomina(a, nome, cartella)


def luce_spot(nome, pos, pitch, yaw, intensita=50.0, temperatura=None, rgb=None, raggio=1500.0,
              cono_esterno=40.0, cono_interno=20.0, sorgente=5.0, ombre=True, cartella="Luci",
              volumetrica=1.0):
    a = attori.spawn_actor_from_class(unreal.SpotLight, vec(*pos), rot(pitch=pitch, yaw=yaw))
    mobile(a)
    c = a.get_component_by_class(unreal.SpotLightComponent)
    imposta(c, "intensity_units", unreal.LightUnits.CANDELAS, silenzioso=True)
    c.set_intensity(intensita)
    c.set_attenuation_radius(raggio)
    c.set_outer_cone_angle(cono_esterno)
    c.set_inner_cone_angle(cono_interno)
    c.set_source_radius(sorgente)
    c.set_cast_shadows(ombre)
    imposta(c, "volumetric_scattering_intensity", volumetrica, silenzioso=True)
    if temperatura:
        c.set_use_temperature(True)
        c.set_temperature(temperatura)
    if rgb:
        c.set_light_color(colore(rgb))
    return nomina(a, nome, cartella)


def luce_rect(nome, pos, pitch, yaw, larghezza, altezza, intensita=10.0, temperatura=None, rgb=None,
              raggio=1500.0, porte=0.0, ombre=True, cartella="Luci", volumetrica=1.0):
    """Rect light: una superficie che emette (una finestra, un'apertura)."""
    a = attori.spawn_actor_from_class(unreal.RectLight, vec(*pos), rot(pitch=pitch, yaw=yaw))
    mobile(a)
    c = a.get_component_by_class(unreal.RectLightComponent)
    imposta(c, "intensity_units", unreal.LightUnits.CANDELAS, silenzioso=True)
    c.set_intensity(intensita)
    c.set_attenuation_radius(raggio)
    imposta(c, "source_width", float(larghezza), silenzioso=True)
    imposta(c, "source_height", float(altezza), silenzioso=True)
    imposta(c, "barn_door_angle", float(porte), silenzioso=True)
    c.set_cast_shadows(ombre)
    imposta(c, "volumetric_scattering_intensity", volumetrica, silenzioso=True)
    if temperatura:
        c.set_use_temperature(True)
        c.set_temperature(temperatura)
    if rgb:
        c.set_light_color(colore(rgb))
    return nomina(a, nome, cartella)


def luce_esterna(sole_pitch=-40.0, sole_yaw=-30.0, sole_lux=8.0, temperatura=5800.0, cielo_intensita=1.0,
                 nebbia_densita=0.02, nebbia_volumetrica=False, luna=False):
    """Sole (o luna) + cielo + atmosfera + nebbia: il minimo per un livello all'aperto."""
    sole = attori.spawn_actor_from_class(unreal.DirectionalLight, vec(0, 0, 800),
                                         rot(pitch=sole_pitch, yaw=sole_yaw))
    mobile(sole)
    c = sole.get_component_by_class(unreal.DirectionalLightComponent)
    c.set_intensity(sole_lux)
    c.set_use_temperature(True)
    c.set_temperature(temperatura)
    imposta(c, "atmosphere_sun_light", True, silenzioso=True)
    imposta(c, "light_source_angle", 0.5 if luna else 1.0, silenzioso=True)
    nomina(sole, "Luna" if luna else "Sole", "Luci")
    nomina(attori.spawn_actor_from_class(unreal.SkyAtmosphere, vec(0, 0, 0), rot()), "Atmosfera", "Luci")
    cielo = attori.spawn_actor_from_class(unreal.SkyLight, vec(0, 0, 900), rot())
    mobile(cielo)
    cc = cielo.get_component_by_class(unreal.SkyLightComponent)
    imposta(cc, "real_time_capture", True, silenzioso=True)
    cc.set_intensity(cielo_intensita)
    nomina(cielo, "Cielo", "Luci")
    nebbia = attori.spawn_actor_from_class(unreal.ExponentialHeightFog, vec(0, 0, -200), rot())
    nc = nebbia.get_component_by_class(unreal.ExponentialHeightFogComponent)
    imposta(nc, "fog_density", nebbia_densita, silenzioso=True)
    imposta(nc, "volumetric_fog", nebbia_volumetrica, silenzioso=True)
    nomina(nebbia, "Nebbia", "Luci")
    return sole


def post_process(nome="PostProcess", **valori):
    """Un Post Process Volume infinito (unbound). valori: nomi delle
    PostProcessSettings in snake_case, es. auto_exposure_bias=0.5, bloom_intensity=0.8.
    Per ogni valore attiva anche il relativo override_*."""
    a = attori.spawn_actor_from_class(unreal.PostProcessVolume, vec(0, 0, 0), rot())
    imposta(a, "unbound", True)
    s = a.get_editor_property("settings")
    for k, v in valori.items():
        imposta(s, "override_" + k, True, silenzioso=True)
        if isinstance(v, (tuple, list)) and len(v) in (3, 4):
            v = unreal.LinearColor(*[float(x) for x in v]) if len(v) == 4 else unreal.LinearColor(
                float(v[0]), float(v[1]), float(v[2]), 1.0)
        imposta(s, k, v)
    a.set_editor_property("settings", s)
    return nomina(a, nome, "Luci")


def decal(nome, materiale, pos, dimensioni=(64, 200, 200), pitch=-90.0, yaw=0.0, cartella="Decal"):
    """Una decal (UE16). Di default proietta verso il basso (pitch -90).
    dimensioni: come il DecalSize di Unreal, cioè METÀ di (profondità,
    larghezza, altezza) in cm: (20, 100, 100) copre 2 x 2 m."""
    a = attori.spawn_actor_from_class(unreal.DecalActor, vec(*pos), rot(pitch=pitch, yaw=yaw))
    c = a.get_component_by_class(unreal.DecalComponent)
    c.set_decal_material(materiale)
    imposta(c, "decal_size", vec(*dimensioni))
    return nomina(a, nome, cartella)


def spline_punti(attore, punti_locali, chiusa=False):
    """Imposta i punti della spline di un actor (coordinate locali, cm)."""
    s = attore.get_component_by_class(unreal.SplineComponent)
    if s is None:
        return
    s.set_spline_points([vec(*p) for p in punti_locali], unreal.SplineCoordinateSpace.LOCAL, True)
    s.set_closed_loop(chiusa, True)
    ricostruisci(attore)


def camera(nome, pos, pitch, yaw, focale=24.0, fuoco=None, apertura=2.8):
    """Una CineCamera per gli screenshot (Pilot / High Resolution Screenshot)."""
    a = attori.spawn_actor_from_class(unreal.CineCameraActor, vec(*pos), rot(pitch=pitch, yaw=yaw))
    c = a.get_cine_camera_component()
    imposta(c, "current_focal_length", focale, silenzioso=True)
    imposta(c, "current_aperture", apertura, silenzioso=True)
    if fuoco:
        fs = c.get_editor_property("focus_settings")
        imposta(fs, "focus_method", unreal.CameraFocusMethod.MANUAL, silenzioso=True)
        imposta(fs, "manual_focus_distance", float(fuoco), silenzioso=True)
        c.set_editor_property("focus_settings", fs)
    return nomina(a, nome, "Camere")


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
    """Salva il livello E i pacchetti degli attori: con "One File Per Actor"
    (attivo di default nei livelli nuovi di UE5) ogni attore è un file a sé
    in __ExternalActors__, e salvare solo la mappa li lascerebbe fuori."""
    livelli.save_current_level()
    try:
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    except Exception as e:  # noqa: BLE001
        avviso("salvataggio dei pacchetti: %s (usa File > Save All)" % e)
