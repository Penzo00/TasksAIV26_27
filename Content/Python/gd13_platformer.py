# -*- coding: utf-8 -*-
"""
GD13 - Task di Game Programming: platformer 3D in tre livelli, con Bot2.

Costruisce tre livelli in /Game/Maps:
  L_GD13_Livello1  si impara: salti, gradini, zona velocità, traguardo
  L_GD13_Livello2  ostacoli: pedana di salto, piattaforme mobili, zona che
                   toglie vita, sbarra rotante che spinge
  L_GD13_Livello3  a tempo: timer, gravità ridotta, sbarra che uccide,
                   ascensore; il traguardo riporta al livello 1

I "pezzi" di gioco sono classi C++ del progetto (Source/AIV_UE5Intro/Platformer):
KillZone, DamageZone, ModifierZone, LevelGoal, LevelTimer, MovingPlatform,
SpinningObstacle. Questo script li piazza e ne imposta i parametri; dopo si
possono spostare e modificare a mano nei Details, come qualunque actor.

COME SI LANCIA
  0. Compilare il progetto (Visual Studio / Rider, o Live Coding) così che le
     classi C++ esistano.
  1. Tools > Execute Python Script... > questo file
     (oppure: import gd13_platformer; gd13_platformer.costruisci_tutto())
  2. Ogni livello viene cancellato e ricreato: se lo avete modificato a mano,
     salvatelo prima con un altro nome.

DOVE MODIFICARE
  Le funzioni livello_1/2/3 qui sotto: ogni riga è un pezzo del percorso, con
  posizione e parametri scritti in chiaro. Unità: cm. Il percorso va lungo +X.
"""

import aiv_livelli as a

LIV1 = "/Game/Maps/L_GD13_Livello1"
LIV2 = "/Game/Maps/L_GD13_Livello2"
LIV3 = "/Game/Maps/L_GD13_Livello3"

# Materiali (template Level Prototyping + Scan Labs)
M_PAV = "/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray"
M_PAV2 = "/Game/LevelPrototyping/Materials/MI_PrototypeGrid_TopDark"
M_SALTO = "/Game/LevelPrototyping/Materials/MI_Solid_Blue"
M_VELOCITA = "/Game/Materials/MI_Deco_Bean_Gold"
M_DANNO = "/Game/Materials/M_LIGHT"
M_MOBILE = "/Game/Materials/MI_System_Wood"
M_TRAGUARDO = "/Game/LevelPrototyping/Materials/MI_Solid_Blue"


def _m(p):
    return a.carica(p)


def trappola_sotto(lunghezza=12000.0, larghezza=6000.0, centro_x=2500.0):
    """Un grande piano invisibile sotto il livello: chi cade, riparte."""
    a.spawn_cpp("KillZone", "Trappola_Caduta", "Gioco/Trappole",
                (centro_x, 0, -700), zone_extent=a.vec(lunghezza / 2, larghezza / 2, 150),
                show_visual=False, restart_delay=0.2)


def traguardo(x, y, z_sopra, prossimo, messaggio="Livello completato!", azzera=False):
    a.spawn_cpp("LevelGoal", "Traguardo", "Gioco", (x, y, z_sopra + 100),
                zone_extent=a.vec(150, 150, 100), next_level=prossimo, message=messaggio,
                delay=1.0, reset_death_count=azzera, label="TRAGUARDO",
                visual_material=_m(M_TRAGUARDO))


def piattaforma_mobile(nome, x, y, z_sopra, spostamento, tempo=2.5, pausa=0.6,
                       ritardo=0.0, dim=(250, 250, 30)):
    return a.spawn_cpp("MovingPlatform", nome, "Gioco/PiattaformeMobili",
                       (x, y, z_sopra - dim[2] / 2.0),
                       end_offset=a.vec(*spostamento), travel_time=tempo, pause_time=pausa,
                       start_delay=ritardo, platform_size=a.vec(*dim))


# ---------------------------------------------------------------------
def livello_1():
    if not a.nuovo_livello(LIV1):
        return
    pav, pav2 = _m(M_PAV), _m(M_PAV2)
    a.luce_esterna()
    a.rendi_giocabile((0, 0, 120))
    a.scritta("Livello 1: salta sulle piattaforme", (300, 0, 260), yaw=180)

    a.pedana("P0_Partenza", "Percorso", 0, 0, 0, 800, 800, materiale=pav)
    a.pedana("P1", "Percorso", 750, 0, 0, 300, 300, materiale=pav2)
    a.pedana("P2_Gradino", "Percorso", 1100, 0, 60, 300, 300, materiale=pav)
    a.pedana("P3_Gradino", "Percorso", 1450, 0, 120, 300, 300, materiale=pav2)

    # corridoio lungo: all'inizio la zona che fa correre più veloce
    a.pedana("P4_Corridoio", "Percorso", 2350, 0, 120, 1200, 400, materiale=pav)
    a.spawn_cpp("ModifierZone", "Zona_Velocita", "Gioco/Modificatori", (1950, 0, 120 + 100),
                zone_extent=a.vec(200, 200, 100), walk_speed_multiplier=1.8,
                jump_multiplier=1.0, gravity_multiplier=1.0, duration_after_exit=4.0,
                label="VELOCITA x1.8", visual_material=_m(M_VELOCITA))
    a.scritta("Prendi la rincorsa!", (2700, 0, 380), yaw=180)

    # salto lungo: si passa meglio con la velocità in più
    a.pedana("P5_Arrivo", "Percorso", 3550, 0, 120, 500, 500, materiale=pav2)
    traguardo(3600, 0, 120, LIV2)
    trappola_sotto(centro_x=1800)
    a.salva()
    a.log("Livello 1 creato")


# ---------------------------------------------------------------------
def livello_2():
    if not a.nuovo_livello(LIV2):
        return
    pav, pav2 = _m(M_PAV), _m(M_PAV2)
    a.luce_esterna(sole_pitch=-35.0, sole_yaw=40.0)
    a.rendi_giocabile((0, 0, 120))
    a.scritta("Livello 2: attenzione agli ostacoli", (250, 0, 260), yaw=180)

    a.pedana("P0_Partenza", "Percorso", 0, 0, 0, 600, 600, materiale=pav)

    # pedana di salto e sporgenza alta
    a.pedana("P1_PedanaSalto", "Percorso", 700, 0, 0, 500, 500, materiale=pav2)
    a.spawn_cpp("ModifierZone", "Zona_Salto", "Gioco/Modificatori", (700, 0, 100),
                zone_extent=a.vec(120, 120, 100), walk_speed_multiplier=1.0,
                jump_multiplier=1.7, gravity_multiplier=1.0, duration_after_exit=2.0,
                label="SALTO x1.7", visual_material=_m(M_SALTO))
    a.pedana("P2_Sporgenza", "Percorso", 1250, 0, 250, 400, 400, materiale=pav)

    # piattaforma mobile sopra il vuoto
    piattaforma_mobile("Mobile_1", 1650, 0, 250, (500, 0, 0), tempo=2.5, pausa=0.8)
    a.pedana("P3", "Percorso", 2600, 0, 250, 500, 500, materiale=pav2)

    # passerella con la zona che toglie vita
    a.pedana("P4_Passerella", "Percorso", 3350, 0, 250, 1000, 400, materiale=pav)
    a.spawn_cpp("DamageZone", "Zona_Danno", "Gioco/Trappole", (3350, 0, 250 + 60),
                zone_extent=a.vec(300, 200, 60), damage_per_hit=20.0, hit_interval=0.4,
                label="SCOTTA! Attraversa in fretta", visual_material=_m(M_DANNO))

    # piazza con la sbarra rotante che spinge via
    a.pedana("P5_Sbarra", "Percorso", 4250, 0, 250, 800, 800, materiale=pav2)
    a.spawn_cpp("SpinningObstacle", "Sbarra_Rotante", "Gioco/Ostacoli", (4250, 0, 250),
                degrees_per_second=75.0, bar_size=a.vec(650, 30, 40), bar_height=60.0,
                push_strength=900.0, kill_on_touch=False)

    a.pedana("P6_Arrivo", "Percorso", 4950, 0, 250, 400, 400, materiale=pav)
    traguardo(4950, 0, 250, LIV3)
    trappola_sotto(centro_x=2500)
    a.salva()
    a.log("Livello 2 creato")


# ---------------------------------------------------------------------
def livello_3():
    if not a.nuovo_livello(LIV3):
        return
    pav, pav2 = _m(M_PAV), _m(M_PAV2)
    a.luce_esterna(sole_pitch=-20.0, sole_yaw=-60.0, sole_lux=6.0)   # tramonto: si va di fretta
    a.rendi_giocabile((0, 0, 120))

    # il timer: se scade, il livello riparte
    a.spawn_cpp("LevelTimer", "Timer", "Gioco", (450, 0, 450), yaw=180,
                time_limit=60.0, warning_threshold=10.0, show_on_screen=True, show_in_world=True)
    a.scritta("Livello 3: hai 60 secondi", (300, 0, 300), yaw=180)

    a.pedana("P0_Partenza", "Percorso", 0, 0, 0, 600, 600, materiale=pav)
    piattaforma_mobile("Mobile_Laterale", 650, -300, 0, (0, 600, 0), tempo=2.0, pausa=0.4)
    a.pedana("P1", "Percorso", 1050, 0, 0, 300, 300, materiale=pav2)

    # gravità ridotta per il salto lungo
    a.pedana("P2_Luna", "Percorso", 1500, 0, 0, 400, 400, materiale=pav)
    a.spawn_cpp("ModifierZone", "Zona_GravitaRidotta", "Gioco/Modificatori", (1500, 0, 100),
                zone_extent=a.vec(180, 180, 100), walk_speed_multiplier=1.1,
                jump_multiplier=1.2, gravity_multiplier=0.45, duration_after_exit=3.0,
                label="GRAVITA RIDOTTA", visual_material=_m(M_SALTO))

    # piazza con la sbarra che uccide: va saltata
    a.pedana("P3_SbarraMortale", "Percorso", 2500, 0, 100, 700, 700, materiale=pav2)
    a.spawn_cpp("SpinningObstacle", "Sbarra_Mortale", "Gioco/Ostacoli", (2500, 0, 100),
                degrees_per_second=60.0, bar_size=a.vec(600, 30, 30), bar_height=50.0,
                kill_on_touch=True)

    # ascensore
    piattaforma_mobile("Ascensore", 3050, 0, 100, (0, 0, 300), tempo=2.5, pausa=1.0)
    a.pedana("P4_Arrivo", "Percorso", 3450, 0, 400, 400, 400, materiale=pav)
    traguardo(3450, 0, 400, LIV1, messaggio="Hai finito tutti e tre i livelli!", azzera=True)
    trappola_sotto(centro_x=1800)
    a.salva()
    a.log("Livello 3 creato")


def costruisci_tutto():
    livello_1()
    livello_2()
    livello_3()
    a.log("Fatto. Apri L_GD13_Livello1 e premi Play.")


if __name__ == "__main__":
    costruisci_tutto()
