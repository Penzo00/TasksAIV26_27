# -*- coding: utf-8 -*-
"""
GD13 - Task di Game Programming: platformer 3D in tre livelli, con Bot2.
Riscritto per contenere quello che abbiamo visto nelle lezioni di
programmazione (UE2-UE16, GD7, GD12, GD13, GD16).

Costruisce tre livelli in /Game/Maps:

  L_GD13_Livello1  "Si impara" - mattina, isola sull'acqua
      gradini, monete (Game Instance + Save Game), mulino che ti guarda
      (rotator e clamp), casse fisiche spawnate con un doppio ciclo for e un
      trace, staccionata su spline "per lunghezza", alberi sparsi (foliage a
      mano), pozzanghera con il materiale fisico dell'acqua (i passi cambiano
      suono), pedana a pressione che apre una porta doppia e accende una luce
      (event dispatcher), piattaforma mobile con suono e pitch, zona velocità,
      L'OGGETTO DEL DOPPIO SALTO e il tratto che lo richiede: una parete
      troppo alta per un salto solo e salti dove, se sbagli, ti correggi in
      aria col secondo salto.

  L_GD13_Livello2  "Ostacoli e congegni" - pomeriggio
      pedana di lancio (Launch Character), torretta che mira (con ripari e
      una cura), piattaforme che crollano e ricompaiono, leva con "premi E"
      (interfaccia di interazione) che chiama un ascensore, LA SEZIONE DI
      WALL RUN E WALL JUMP fra due muri blu (con una pedana sotto che ti
      riporta su se cadi), corridoio di lava che toglie vita (con le decal
      delle bruciature), piazza con la sbarra che spinge, ponte di ghiaccio
      (attrito) e un "negozio": paghi 5 monete e sale un ponte sicuro.

  L_GD13_Livello3  "Di notte, a tempo" - 90 secondi
      timer, piattaforma laterale, portale (teletrasporto), gravità ridotta,
      sbarra che uccide, un wall run veloce, salti da doppio salto, torretta
      che anticipa il movimento, ascensore; bracieri con luce che tremola.
      Il traguardo riporta al livello 1 e azzera le morti.

I pezzi di gioco sono classi C++ del progetto (Source/AIV_UE5Intro/Platformer).
Questo script li piazza e ne imposta i parametri; dopo si possono spostare e
modificare a mano nei Details, come qualunque actor. I blocchi del percorso
sono Blockout_Box del plugin BlockOut Tools: colore = significato
(grigio percorso, arancio salto/interazione, blu wall run, rosso pericolo,
verde traguardo).

COME SI LANCIA
  0. Compilare il progetto (le classi C++ devono esistere) e riavviare
     l'editor una volta (redirect di Scan Labs in DefaultEngine.ini).
  1. Tools > Execute Python Script... > questo file
     (oppure nel Output Log, Python: import gd13_platformer; gd13_platformer.costruisci_tutto())
  2. Ogni livello viene cancellato e ricreato: se lo avete modificato a mano,
     salvatelo prima con un altro nome.

MISURE (UE14, il gym): con il nostro componente delle abilità
  salto: 650 cm/s verso l'alto -> ~2,15 m di altezza
  salto camminando (300 cm/s): ~4 m; correndo (500 cm/s): ~6,5 m
  doppio salto: +~1,8 m di altezza, e la direzione si corregge in aria
Le distanze sotto sono pensate così: salti normali <= 3 m (bordo-bordo),
"prendi la rincorsa" 4-5 m, gradini <= 1 m, parete del doppio salto 3,3 m.
Unità: cm. Il percorso va lungo +X.
"""

import unreal

import aiv_livelli as a
import aiv_materiali as mat

LIV1 = "/Game/Maps/L_GD13_Livello1"
LIV2 = "/Game/Maps/L_GD13_Livello2"
LIV3 = "/Game/Maps/L_GD13_Livello3"

ALBERO = "/Game/Meshes/Tree/SM_Chestnut_4"
CESPUGLIO = "/Game/Meshes/Decor/SM_Groundplant_12"

M = {}


def _abilita(nome):
    """Valore dell'enum EPlatformerAbility (DOUBLE_JUMP / WALL_RUN)."""
    enum = getattr(unreal, "PlatformerAbility", None)
    if enum is None:
        return None
    return getattr(enum, nome, None)


# ---------------------------------------------------------------------
# pezzi ricorrenti
# ---------------------------------------------------------------------

def trappola_sotto(centro_x, lunghezza, larghezza=8000.0, z=-500.0):
    """Un grande volume invisibile sotto il livello: chi cade, riparte."""
    a.spawn_cpp("KillZone", "Trappola_Caduta", "Gioco/Trappole",
                (centro_x, 0, z), zone_extent=a.vec(lunghezza / 2, larghezza / 2, 150),
                show_visual=False, restart_delay=0.2)


def traguardo(x, y, z_sopra, prossimo, messaggio="Livello completato!", azzera=False):
    a.spawn_cpp("LevelGoal", "Traguardo", "Gioco", (x, y, z_sopra + 100),
                zone_extent=a.vec(150, 150, 100), next_level=prossimo, message=messaggio,
                delay=1.2, reset_death_count=azzera, label="TRAGUARDO",
                visual_material=M["Traguardo"])
    a.luce_punto("Luce_Traguardo", (x, y, z_sopra + 260), intensita=30.0, rgb=(0.4, 1.0, 0.5),
                 raggio=700.0, ombre=False)


def moneta(nome, x, y, z_sopra, valore=1):
    return a.spawn_cpp("CoinPickup", nome, "Gioco/Monete", (x, y, z_sopra + 90),
                       value=valore, material=M["Moneta"], glow_intensity=4.0,
                       glow_color=a.colore((1.0, 0.75, 0.3)))


def fila_monete(prefisso, x0, y0, x1, y1, z_sopra, n):
    for i in range(n):
        t = i / float(max(1, n - 1))
        moneta("%s_%d" % (prefisso, i + 1), x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, z_sopra)


def cura(nome, x, y, z_sopra, quanto=40.0):
    return a.spawn_cpp("HealthPickup", nome, "Gioco/Cure", (x, y, z_sopra + 90),
                       heal_amount=quanto, material=M["Vita"], glow_color=a.colore((0.3, 1.0, 0.4)),
                       label="+VITA")


def piattaforma_mobile(nome, x, y, z_sopra, spostamento, tempo=2.5, pausa=0.6,
                       ritardo=0.0, dim=(250, 250, 30)):
    return a.spawn_cpp("MovingPlatform", nome, "Gioco/PiattaformeMobili",
                       (x, y, z_sopra - dim[2] / 2.0),
                       end_offset=a.vec(*spostamento), travel_time=tempo, pause_time=pausa,
                       start_delay=ritardo, platform_size=a.vec(*dim),
                       platform_material=M["Salto"], move_sound=a.suono("SfxPlatformMoving_Cue"))


def muro_wallrun(nome, x_inizio, x_fine, y_faccia, lato, z_basso, z_alto, spessore=100.0):
    """Un muro blu su cui si può correre. y_faccia è la faccia percorribile,
    lato = +1 se il muro sta dalla parte delle Y positive, -1 se negative."""
    cx = (x_inizio + x_fine) / 2.0
    cy = y_faccia + lato * spessore / 2.0
    cz = (z_basso + z_alto) / 2.0
    return a.blocco(nome, "Percorso/WallRun", (cx, cy, cz), (x_fine - x_inizio, spessore, z_alto - z_basso),
                    tinta="muro", tag=a.TAG_WALLRUN)


def braciere(nome, x, y, z_sopra, intensita=80.0):
    """Un braciere: base, braci emissive e una luce che tremola (AFlickeringLight)."""
    a.blocco(nome + "_Base", "Arredo/Bracieri", (x, y, z_sopra + 45), (50, 50, 90), M["Ferro"])
    braci = a.mesh(a.SFERA, nome + "_Braci", "Arredo/Bracieri", (x, y, z_sopra + 95),
                   scala=(0.45, 0.45, 0.22), materiale=M["Braci"], ombre=False, collisione=False)
    luce = a.spawn_cpp("FlickeringLight", nome + "_Luce", "Luci/Fuochi", (x, y, z_sopra + 150),
                       base_intensity=intensita, flicker_amount=0.3, flicker_speed=7.0,
                       temperature=1800.0, attenuation_radius=1400.0, source_radius=10.0,
                       position_jitter=5.0)
    a.collega(luce, "glow_actors", [braci])
    return luce


def isola_alberi(nome, x, y, ex, ey, n, seme, z_top=0.0):
    a.spawn_cpp("ScatterArea", nome, "Arredo/Natura", (x, y, z_top + 300),
                mesh=a.carica(ALBERO), area_extent=a.vec(ex, ey, 600), count=n, seed=seme,
                min_scale=0.55, max_scale=0.8, min_distance=600.0, align_to_ground=True,
                collide=True, cull_distance=0.0)


# ---------------------------------------------------------------------
# LIVELLO 1 - si impara
# ---------------------------------------------------------------------

def livello_1():
    if not a.nuovo_livello(LIV1):
        return
    a.luce_esterna(sole_pitch=-32.0, sole_yaw=-35.0, sole_lux=9.0, temperatura=5200.0, nebbia_densita=0.015)
    a.post_process(auto_exposure_bias=0.3, bloom_intensity=0.5, vignette_intensity=0.25)
    a.rendi_giocabile((-500, 0, 120))

    # acqua tutto intorno: chi cade, riparte
    # (la trappola sta SOPRA l'acqua, che ha collisione: si "affoga" prima di toccarla)
    a.blocco("Acqua", "Ambiente", (4000, 0, -310), (16000, 9000, 20), M["Acqua"])
    trappola_sotto(4000, 16000, larghezza=9000.0, z=-200)

    # --- isola di partenza ------------------------------------------------
    a.pedana("Isola_Partenza", "Percorso/Isola", 0, 0, 0, 1600, 1600, spessore=300, materiale=M["Erba"])
    a.scritta("LIVELLO 1\nsalta, raccogli, scopri", (-150, 0, 330), yaw=180, dimensione=55)
    fila_monete("Moneta_Isola", -300, -300, 200, -300, 0, 3)

    # pozzanghera: il materiale fisico dell'acqua cambia il suono dei passi
    a.blocco("Pozzanghera", "Arredo", (-250, 350, 1), (360, 260, 2), M["Acqua"])
    a.decal("Pozza_Bordo", M["Pozza"], (-250, 350, 10), (30, 140, 190))

    # mulino: le pale ruotano, la testa ti segue (con un limite all'angolo)
    a.spawn_cpp("Windmill", "Mulino", "Arredo", (-450, -650, 0), yaw=60, blade_speed=50.0,
                look_at_player=True, yaw_limit=60.0, tower_height=900.0, blade_length=420.0,
                blades_collide=False, tower_material=M["IntonacoChiaro"], blade_material=M["LegnoChiaro"])

    # casse fisiche: spawn con due cicli for e un trace verso il basso
    a.spawn_cpp("CubeSpawner", "Casse_Fisiche", "Arredo", (250, 550, 300), rows=3, columns=3, cube_layers=2,
                cube_size=60.0, spacing=66.0, simulate_physics=True, cube_mass=6.0,
                materials=[M["LegnoChiaro"], M["LegnoScuro"]])

    # staccionata lungo una spline (un paletto ogni 200 cm)
    recinto = a.spawn_cpp("SplineFence", "Staccionata", "Arredo", (-780, -780, 0), spacing=200.0,
                          rails=2, material=M["LegnoScuro"], collide=True)
    if recinto:
        a.spline_punti(recinto, [(0, 0, 0), (0, 1560, 0), (1560, 1560, 0)])
    for i, (x, y) in enumerate(((-600, 620), (500, -700), (-700, 100))):
        a.mesh(CESPUGLIO, "Cespuglio_%d" % i, "Arredo/Natura", (x, y, 0), yaw=i * 70.0, scala=1.3,
               collisione=False)

    # alberi sulle isolette intorno (foliage "a mano")
    a.pedana("Isoletta_Nord", "Ambiente", 1500, 2600, 0, 2400, 1400, spessore=300, materiale=M["Erba"])
    a.pedana("Isoletta_Sud", "Ambiente", 4500, -2900, 0, 3200, 1600, spessore=300, materiale=M["Erba"])
    isola_alberi("Alberi_Nord", 1500, 2600, 1100, 600, 5, 11)
    isola_alberi("Alberi_Sud", 4500, -2900, 1500, 700, 7, 23)

    # --- cancello: pedana a pressione -> porta doppia + luce ----------------
    # muro con un'apertura al centro
    a.blocco("Cancello_Muro_S", "Percorso/Cancello", (780, -460, 200), (60, 680, 400), M["Pietra"])
    a.blocco("Cancello_Muro_N", "Percorso/Cancello", (780, 460, 200), (60, 680, 400), M["Pietra"])
    a.blocco("Cancello_Architrave", "Percorso/Cancello", (780, 0, 350), (60, 240, 100), M["Pietra"])
    porta = a.spawn_cpp("ActivableDoor", "Porta_Doppia", "Gioco/Congegni", (780, 0, 0),
                        door_width=240.0, door_height=300.0, door_thickness=16.0, sliding=False,
                        open_angle=100.0, transition_time=1.2, door_material=M["LegnoDipinto"])
    piastra = a.spawn_cpp("PressurePlate", "Pedana_Pressione", "Gioco/Congegni", (450, -300, 0),
                          stay_pressed=True, on_material=M["Acceso"], off_material=M["Spento"])
    a.collega(piastra, "targets", [porta])           # riferimento diretto
    lampada = a.spawn_cpp("ActivableLight", "Lampada_Cancello", "Gioco/Congegni", (720, 0, 360),
                          transition_time=0.6)
    a.collega(lampada, "listen_to", [piastra])       # ascolta l'event dispatcher della pedana
    a.scritta("sali sulla pedana", (450, -300, 140), yaw=180, dimensione=35)

    # --- gradini -----------------------------------------------------------
    a.pedana("P1", "Percorso", 1100, 0, 0, 400, 400)
    a.pedana("P2_Gradino", "Percorso", 1550, 0, 60, 300, 300, tinta="percorso2")
    a.pedana("P3_Gradino", "Percorso", 1900, 0, 120, 300, 300)
    a.pedana("P4_Gradino", "Percorso", 2300, 0, 180, 300, 300, tinta="percorso2")
    moneta("Moneta_P2", 1550, 0, 60)
    moneta("Moneta_P3", 1900, 0, 120)
    moneta("Moneta_P4", 2300, 0, 180)

    # --- piattaforma mobile (suono con pitch diverso all'andata e al ritorno)
    piattaforma_mobile("Mobile_1", 2650, 0, 180, (600, 0, 0), tempo=2.5, pausa=0.8)
    a.scritta("aspetta la piattaforma", (2450, 0, 420), yaw=180, dimensione=35)

    # --- corridoio con la zona velocità e salto lungo ------------------------
    a.pedana("P5_Corridoio", "Percorso", 3900, 0, 180, 900, 400)
    a.spawn_cpp("ModifierZone", "Zona_Velocita", "Gioco/Modificatori", (3650, 0, 180 + 100),
                zone_extent=a.vec(180, 180, 100), walk_speed_multiplier=1.8, duration_after_exit=4.0,
                label="VELOCITA x1.8", visual_material=M["Salto"])
    a.scritta("prendi la rincorsa!", (4250, 0, 420), yaw=180, dimensione=40)

    # --- IL DOPPIO SALTO ------------------------------------------------------
    a.pedana("P6_DoppioSalto", "Percorso", 5000, 0, 180, 500, 500, tinta="salto")
    a.spawn_cpp("AbilityPickup", "Oggetto_DoppioSalto", "Gioco/Abilita", (5000, 0, 180 + 110),
                ability=_abilita("DOUBLE_JUMP"), material=M["Abilita"], glow_intensity=20.0,
                glow_color=a.colore((0.3, 0.7, 1.0)), pickup_radius=80.0, label="DOPPIO SALTO",
                message="Doppio salto! Premi salto di nuovo in aria: puoi anche cambiare direzione")
    a.luce_spot("Luce_DoppioSalto", (5000, 0, 900), -90.0, 0.0, intensita=400.0, rgb=(0.5, 0.75, 1.0),
                raggio=1200.0, cono_esterno=22.0, cono_interno=12.0)

    # parete di 3,3 m: con un salto solo (2,15 m) non ci si arriva
    a.pedana("P7_Parete", "Percorso", 5550, 0, 510, 500, 500, spessore=330, tinta="percorso2")
    a.scritta("troppo alta? salta due volte", (5250, 0, 560), yaw=180, dimensione=35)

    # salti in cui, se sbagli, ti correggi in aria con il secondo salto
    a.pedana("P8", "Percorso", 6250, 0, 510, 300, 300)
    a.pedana("P9", "Percorso", 6900, 200, 600, 300, 300, tinta="percorso2")
    a.pedana("P10", "Percorso", 7550, -100, 510, 300, 300)
    moneta("Moneta_P8", 6250, 0, 510)
    moneta("Moneta_P9", 6900, 200, 600)
    moneta("Moneta_P10", 7550, -100, 510)
    a.scritta("se manchi il salto, risalta in aria\ne correggi la direzione", (6250, 0, 780), yaw=180,
              dimensione=32)

    # --- traguardo -------------------------------------------------------------
    a.pedana("P11_Arrivo", "Percorso", 8250, 0, 510, 600, 600, tinta="traguardo")
    traguardo(8300, 0, 510, LIV2, messaggio="Livello 1 completato! Le monete sono al sicuro")
    a.salva()
    a.log("Livello 1 creato")


# ---------------------------------------------------------------------
# LIVELLO 2 - ostacoli e congegni
# ---------------------------------------------------------------------

def livello_2():
    if not a.nuovo_livello(LIV2):
        return
    a.luce_esterna(sole_pitch=-38.0, sole_yaw=40.0, sole_lux=8.0, temperatura=5600.0)
    a.post_process(auto_exposure_bias=0.2, bloom_intensity=0.6, vignette_intensity=0.3)
    a.rendi_giocabile((-150, 0, 120))
    a.scritta("LIVELLO 2\nostacoli e congegni", (150, 0, 330), yaw=180, dimensione=55)
    trappola_sotto(6500, 16000, z=-600)

    a.pedana("P0_Partenza", "Percorso", 0, 0, 0, 800, 800)

    # --- pedana di lancio (Launch Character) -----------------------------------
    a.pedana("P1_Lancio", "Percorso", 800, 0, 0, 400, 400, tinta="percorso2")
    a.spawn_cpp("JumpPad", "Pedana_Lancio", "Gioco/Congegni", (800, 0, 10),
                launch_velocity=a.vec(350, 0, 1300), override_horizontal=True, pad_material=M["Salto"])
    a.pedana("P2_Alto", "Percorso", 1500, 0, 500, 500, 500, spessore=500)
    moneta("Moneta_P2", 1500, 0, 500)

    # --- la torretta (con ripari e una cura) ------------------------------------
    a.pedana("P3_Tiro", "Percorso", 2500, 0, 500, 1200, 600)
    a.blocco("Riparo_1", "Percorso/Ripari", (2150, -80, 600), (80, 300, 200), tinta="riparo")
    a.blocco("Riparo_2", "Percorso/Ripari", (2600, 120, 600), (80, 300, 200), tinta="riparo")
    a.pedana("Torretta_Base", "Gioco/Torrette", 2900, -650, 600, 300, 300, spessore=700, tinta="pericolo")
    a.spawn_cpp("Turret", "Torretta", "Gioco/Torrette", (2900, -650, 600), yaw=90, range=1800.0,
                fire_interval=1.6, turn_speed=3.0, projectile_speed=1400.0, projectile_damage=20.0,
                lead_target=False, body_material=M["Torretta"], projectile_material=M["Proiettile"])
    fila_monete("Moneta_P3", 2000, 150, 2400, 150, 500, 2)
    cura("Cura_P3", 3000, 200, 500)
    a.scritta("la torretta ti vede: usa i ripari", (2000, 0, 800), yaw=180, dimensione=35)

    # --- piattaforme che crollano ----------------------------------------------
    for i, x in enumerate((3400, 3800, 4200)):
        a.spawn_cpp("CrumblingPlatform", "Crolla_%d" % (i + 1), "Gioco/Crollano", (x, 0, 500 - 15),
                    platform_size=a.vec(250, 250, 30), shake_time=0.7, respawn_time=3.0,
                    platform_material=M["Salto"], warning_material=M["Pericolo"],
                    crumble_sound=a.suono("Rock_Cue"))
    moneta("Moneta_Crolla", 3800, 0, 500)

    # --- leva ("premi E") e ascensore ----------------------------------------------
    a.pedana("P4_Leva", "Percorso", 4750, 0, 500, 600, 600, tinta="percorso2")
    ascensore = a.spawn_cpp("ActivableMover", "Ascensore", "Gioco/Congegni", (5250, 0, 500 - 15),
                            end_offset=a.vec(0, 0, 450), platform_size=a.vec(300, 300, 30),
                            transition_time=2.5)
    if ascensore:
        m = ascensore.get_component_by_class(unreal.StaticMeshComponent)
        if m:
            m.set_material(0, M["Salto"])
    leva = a.spawn_cpp("InteractLever", "Leva_Ascensore", "Gioco/Congegni", (4900, -220, 500), yaw=180,
                       prompt="Chiama l'ascensore", toggle=True, label="LEVA",
                       on_material=M["Acceso"], off_material=M["Spento"])
    a.collega(leva, "targets", [ascensore])

    # --- WALL RUN e WALL JUMP --------------------------------------------------------
    a.pedana("P5_WallRun", "Percorso", 5700, 0, 950, 500, 500)
    a.scritta("WALL RUN: corri e salta verso il muro blu\nsalta di nuovo per passare all'altro muro",
              (5650, 0, 1230), yaw=180, dimensione=32, colore_testo=(0.5, 0.8, 1.0))
    muro_wallrun("Muro_A", 6050, 6850, 300, +1, 750, 1450)
    muro_wallrun("Muro_B", 6900, 7700, -300, -1, 750, 1450)
    moneta("Moneta_MuroA", 6500, 220, 1050)
    moneta("Moneta_MuroB", 7350, -220, 1050)
    # se cadi: un pavimento di sicurezza e una pedana che ti rilancia su P5
    a.pedana("Sotto_Sicurezza", "Percorso/WallRun", 6900, 0, 500, 2000, 1200, tinta="percorso2")
    a.spawn_cpp("JumpPad", "Pedana_Ritorno", "Gioco/Congegni", (6650, 0, 510), yaw=180,
                launch_velocity=a.vec(350, 0, 1300), override_horizontal=True, pad_material=M["Salto"])
    a.scritta("caduto? la pedana ti riporta su", (6650, 0, 760), yaw=180, dimensione=30)
    a.pedana("P6_DopoMuri", "Percorso", 8150, 0, 950, 800, 1000)

    # --- corridoio di lava (toglie vita) ---------------------------------------------
    a.pedana("P7_Lava", "Percorso", 9250, 0, 950, 1400, 600, tinta="percorso2")
    a.spawn_cpp("DamageZone", "Lava", "Gioco/Trappole", (9250, 0, 950 + 60),
                zone_extent=a.vec(400, 300, 60), damage_per_hit=15.0, hit_interval=0.4,
                label="LAVA! attraversa in fretta", visual_material=M["Lava"], visual_thickness=6.0)
    a.luce_punto("Luce_Lava", (9250, 0, 1080), intensita=120.0, rgb=(1.0, 0.35, 0.08), raggio=900.0,
                 ombre=False)
    for i, (x, y) in enumerate(((8780, 180), (8800, -200), (9720, 120), (9700, -170))):
        a.decal("Bruciatura_%d" % i, M["Bruciatura"], (x, y, 960), (30, 130, 130), yaw=i * 40.0)
    fila_monete("Moneta_Lava", 9050, 0, 9450, 0, 950, 2)

    # --- la sbarra rotante che spinge ----------------------------------------------
    a.pedana("P8_Sbarra", "Percorso", 10450, 0, 950, 800, 800)
    a.spawn_cpp("SpinningObstacle", "Sbarra_Rotante", "Gioco/Ostacoli", (10450, 0, 950),
                degrees_per_second=75.0, bar_size=a.vec(650, 30, 40), bar_height=60.0,
                push_strength=900.0, kill_on_touch=False)
    moneta("Moneta_Sbarra", 10450, 300, 950)

    # --- ghiaccio e negozio -------------------------------------------------------------
    a.pedana("P9_Piazza", "Percorso", 11400, 0, 950, 900, 1200, tinta="percorso2")
    a.blocco("Ponte_Ghiaccio", "Percorso", (12350, -250, 930), (1000, 300, 40), M["Ghiaccio"])
    a.spawn_cpp("ModifierZone", "Zona_Ghiaccio", "Gioco/Modificatori", (12075, -250, 950 + 100),
                zone_extent=a.vec(775, 150, 100), ground_friction_multiplier=0.08,
                braking_multiplier=0.05, duration_after_exit=0.5, label="GHIACCIO",
                show_visual=False)
    ponte = a.spawn_cpp("ActivableMover", "Ponte_Pagato", "Gioco/Congegni", (12350, 250, 950 - 15 - 400),
                        end_offset=a.vec(0, 0, 400), platform_size=a.vec(1000, 300, 30), transition_time=2.0)
    if ponte:
        m = ponte.get_component_by_class(unreal.StaticMeshComponent)
        if m:
            m.set_material(0, M["Moneta"])
    negozio = a.spawn_cpp("ShopZone", "Negozio", "Gioco/Congegni", (11600, 400, 950 + 100),
                          zone_extent=a.vec(120, 120, 100), cost=5, only_once=True,
                          label="5 MONETE: ponte sicuro", visual_material=M["Moneta"])
    a.collega(negozio, "targets", [ponte])
    a.scritta("ghiaccio gratis... o ponte sicuro a 5 monete", (11150, 0, 1250), yaw=180, dimensione=32)

    a.pedana("P10_Arrivo", "Percorso", 13250, 0, 950, 800, 1000, tinta="traguardo")
    traguardo(13300, 0, 950, LIV3, messaggio="Livello 2 completato! Ora di notte, a tempo")
    a.salva()
    a.log("Livello 2 creato")


# ---------------------------------------------------------------------
# LIVELLO 3 - di notte, a tempo
# ---------------------------------------------------------------------

def livello_3():
    if not a.nuovo_livello(LIV3):
        return
    a.luce_esterna(sole_pitch=-35.0, sole_yaw=-120.0, sole_lux=0.4, temperatura=9000.0,
                   cielo_intensita=0.6, nebbia_densita=0.03, nebbia_volumetrica=True, luna=True)
    a.post_process(auto_exposure_bias=1.0, auto_exposure_min_brightness=0.4,
                   auto_exposure_max_brightness=1.5, bloom_intensity=0.9, vignette_intensity=0.45)
    a.rendi_giocabile((-150, 0, 120))
    trappola_sotto(5000, 13000, z=-600)

    # il timer: se scade, il livello riparte (lo mostra anche l'HUD)
    a.spawn_cpp("LevelTimer", "Timer", "Gioco", (300, 0, 450), yaw=180,
                time_limit=90.0, warning_threshold=15.0, show_on_screen=False, show_in_world=True)
    a.scritta("LIVELLO 3: 90 secondi", (300, 0, 300), yaw=180, dimensione=45)

    a.pedana("P0_Partenza", "Percorso", 0, 0, 0, 600, 600)
    braciere("Braciere_0", 200, 230, 0)

    # piattaforma laterale
    piattaforma_mobile("Mobile_Laterale", 550, -350, 0, (0, 700, 0), tempo=2.0, pausa=0.4)
    a.pedana("P1_Portale", "Percorso", 1000, 0, 0, 300, 300, tinta="salto")

    # portale: teletrasporto su un'isola alta
    a.spawn_cpp("TeleportZone", "Portale", "Gioco/Congegni", (1000, 0, 100),
                zone_extent=a.vec(100, 100, 100), destination=a.vec(1500, 0, 600), stop_velocity=True,
                label="PORTALE", visual_material=M["Abilita"])
    a.luce_punto("Luce_Portale", (1000, 0, 180), intensita=60.0, rgb=(0.35, 0.6, 1.0), raggio=600.0,
                 ombre=False)

    # gravità ridotta per il salto lungo
    a.pedana("P2_Luna", "Percorso", 2500, 0, 600, 500, 500)
    braciere("Braciere_2", 2350, -200, 600)
    a.spawn_cpp("ModifierZone", "Zona_GravitaRidotta", "Gioco/Modificatori", (2600, 0, 600 + 100),
                zone_extent=a.vec(150, 200, 100), walk_speed_multiplier=1.1, jump_multiplier=1.2,
                gravity_multiplier=0.45, duration_after_exit=3.0, label="GRAVITA RIDOTTA",
                visual_material=M["Salto"])
    a.pedana("P3", "Percorso", 3700, 0, 700, 400, 400, tinta="percorso2")
    moneta("Moneta_P3", 3700, 0, 700)

    # sbarra che uccide: va saltata
    a.pedana("P4_SbarraMortale", "Percorso", 4600, 0, 700, 800, 800)
    a.spawn_cpp("SpinningObstacle", "Sbarra_Mortale", "Gioco/Ostacoli", (4600, 0, 700),
                degrees_per_second=60.0, bar_size=a.vec(600, 30, 30), bar_height=50.0, kill_on_touch=True)
    braciere("Braciere_4", 4950, 350, 700)

    # wall run veloce (oppure un doppio salto ben fatto)
    muro_wallrun("Muro_Notte", 5050, 5950, 300, +1, 500, 1200)
    moneta("Moneta_Muro", 5500, 220, 800)
    a.pedana("P5", "Percorso", 6400, 0, 700, 800, 800)
    braciere("Braciere_5", 6700, -300, 700)
    cura("Cura_P5", 6300, 250, 700)

    # salti da doppio salto
    a.pedana("P6", "Percorso", 7400, 0, 900, 300, 300, tinta="salto")
    a.pedana("P7", "Percorso", 8200, 0, 1000, 300, 300, tinta="salto")
    moneta("Moneta_P6", 7400, 0, 900)
    moneta("Moneta_P7", 8200, 0, 1000)

    # torretta che anticipa il movimento
    a.pedana("P8", "Percorso", 9000, 0, 1000, 600, 600)
    a.blocco("Riparo_P8", "Percorso/Ripari", (9000, 120, 1100), (80, 260, 200), tinta="riparo")
    a.pedana("Torretta_Base", "Gioco/Torrette", 8800, 700, 1100, 250, 250, spessore=800, tinta="pericolo")
    a.spawn_cpp("Turret", "Torretta_Furba", "Gioco/Torrette", (8800, 700, 1100), yaw=-90, range=1600.0,
                fire_interval=1.8, projectile_speed=1600.0, projectile_damage=20.0, lead_target=True,
                body_material=M["Torretta"], projectile_material=M["Proiettile"])
    braciere("Braciere_8", 9200, -230, 1000)

    # ascensore e traguardo
    piattaforma_mobile("Ascensore", 9550, 0, 1000, (0, 0, 500), tempo=2.0, pausa=1.0)
    a.pedana("P9_Arrivo", "Percorso", 10000, 0, 1500, 500, 500, tinta="traguardo")
    braciere("Braciere_9", 10150, 180, 1500, intensita=100.0)
    traguardo(10000, 0, 1500, LIV1, messaggio="Hai finito tutti e tre i livelli!", azzera=True)
    a.salva()
    a.log("Livello 3 creato")


def costruisci_tutto():
    M.update(mat.tutti())
    for costruisci_livello in (livello_1, livello_2, livello_3):
        a.sicuro(costruisci_livello)
    a.log("Fatto. Apri L_GD13_Livello1 e premi Play.")


if __name__ == "__main__":
    costruisci_tutto()
