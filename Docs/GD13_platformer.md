# GD13 – Platformer in tre livelli "gym" (con quello che abbiamo visto a lezione)

Tre livelli giocabili con **Bot2** (che resta il personaggio del giocatore), costruiti da uno script Python dell'editor e fatti di **classi C++ del progetto**, tutte con i parametri esposti nei Details. In più, rispetto alla consegna:

- **Wall run e wall jump** (con una sezione dedicata nel livello 2 e un passaggio veloce nel livello 3).
- **L'oggetto del doppio salto** nel livello 1: lo raccogli e da lì in poi, premendo salto in aria, fai un secondo salto **controllabile** (la direzione si corregge verso dove stai spingendo lo stick/WASD). Se manchi una piattaforma con il primo salto, col secondo ti salvi.

## Primo avvio: cosa controllare

1. **Compilare** (Visual Studio/Rider, oppure "Compile" nell'editor). Ho aggiunto il modulo `EnhancedInput` a `AIV_UE5Intro.Build.cs`. Non ho potuto compilare da qui: se c'è un errore, mandatemi il log e lo sistemo.
2. **Riavviare l'editor una volta.** In `Config/DefaultEngine.ini` c'è un `[CoreRedirects]` per il pacchetto Scan Labs: i suoi asset sono stati spostati da `Content/Scanlabz_Interiors_1/` alla radice di `Content`, e i riferimenti interni (mesh → materiali → texture) puntavano ancora alla vecchia cartella. Per questo i prop apparivano grigi. Dopo il riavvio conviene fare *Fix Up Redirectors* sulla cartella `Content` e salvare tutto.
3. **Lanciare gli script** (Tools > Execute Python Script…): prima `Content/Python/aiv_materiali.py` (crea i materiali in `/Game/AIV/Materiali`), poi `gd13_platformer.py`. Lo script dei livelli chiama comunque i materiali da solo.
4. Aprire `L_GD13_Livello1` e premere **Play**.
5. Se volete provare il livello 2 o 3 senza passare dal primo: nella GameMode (`PlatformerGameMode`, World Settings) c'è `bAllAbilitiesForTesting` (o il vecchio `bDoubleJumpForTesting`), oppure da console `AIVUnlockAll`. `AIVResetProgress` azzera il salvataggio.

Controlli: quelli di Bot2 (WASD, Shift per correre, Spazio per saltare), il tasto di `IA_Interact` (già nell'`IMC_Default`) per le leve, e in più:

| Tasto | Cosa fa |
|---|---|
| Spazio in aria | doppio salto (dopo averlo raccolto) / wall jump sui muri blu |
| **Spazio tenuto premuto in aria** | **jetpack** (dopo averlo raccolto), finché c'è carburante |
| **F** (pad: tasto sinistro) | **genera una cassa** davanti a te (3 casse, poi 5 s di ricarica) |
| **Rotellina del mouse** | avvicina/allontana la telecamera; tutta avanti = **prima persona con la torcia** |

## Le 9 meccaniche nuove (e dove sono nei livelli)

| # | Richiesta | Classe C++ | Dove si prova |
|---|---|---|---|
| 1 | Actor che diventa più grande più il player è lontano | `AScaleByDistanceActor` (`PlatformerGym.h`) | L1: il **faro** all'inizio (si vede sempre). L2: tre **piattaforme che si restringono** mentre ti avvicini |
| 2 | Jetpack tenendo premuto salto | `UPlatformerJetpackComponent` + oggetto **JETPACK** | L2: salto di 11,5 m verso una piattaforma piccola. L3: dopo il wall run della combo finale |
| 3 | Torcia in prima persona con la batteria | `UPlatformerTorchComponent` + `ABatteryPickup` | L3: il **tunnel buio** (batterie all'ingresso e a metà) |
| 4 | Ultimo posto sicuro + 3 vite | `UPlatformerSafeSpotComponent`, `AFallZone`, vite in `UPlatformerRunSubsystem` | Tutti i livelli: l'acqua e il vuoto sono `AFallZone`. L1: i sassi della strada lunga |
| 5 | Oggetto presente solo il 35% delle volte | `ARandomPresenceActor` | L1: il **ponte fortunato** (se non c'è, si passa dai sassi) |
| 6 | Torretta i cui proiettili riavviano il livello | `ATurret` con `bRestartLevelOnHit` | L2: la **torretta mortale** prima del traguardo, con tre ripari |
| 7 | Tasto che genera un oggetto 3 volte, poi 5 s di attesa | `UPlatformerCrateSpawnerComponent` + `ASpawnedCrate` + oggetto **GENERATORE DI CASSE** | L1: la parete di 4,6 m. L3: l'ultima parete |
| 8 | Zona che toglie 10 HP al secondo | `ADamageZone` (10 danni ogni 1 s) | L2: il corridoio di **lava** |
| 9 | Actor che cambia modello ogni secondo | `AModelCyclerActor` | L1: il **totem**. L3: la **piattaforma che cambia forma** (salta quando è un cubo) |

I potenziamenti sono actor semplici (`AAbilityPickup`) che girano, con la **scritta sopra** che dice cosa danno e un faretto azzurro. Tutti i nuovi componenti li aggiunge la GameMode a Bot2 quando nasce, come le abilità di prima: BP_Bot2 non è toccato.

### Come funzionano, in breve

- **Faro / piattaforme che si restringono.** Ogni frame: distanza dal player → `Map Range Clamped` fra `NearDistance` e `FarDistance` → scala fra `MinScale` e `MaxScale`, con un `FInterp To` per non scattare. Con `bScaleHeight` falso cambia solo la pianta (le piattaforme restano spesse uguali).
- **Jetpack.** Ascolta `IA_Jump` (Started = premuto, Completed = rilasciato). Se il tasto è tenuto da più di `HoldDelay` (0,2 s), il personaggio è in aria e c'è carburante: `Velocity.Z` va verso `MaxUpSpeed` (260 cm/s) con `FInterp To Constant`, l'Air Control sale a 0,9 (si pilota l'atterraggio) e il carburante (2,2 s) scende. A terra si ricarica. Un **tocco** resta un doppio salto, **tenere premuto** accende il jetpack. Fiamma (cono) e luce dietro la schiena mentre si vola.
- **Torcia.** L'azione della rotellina (`IA_Platformer_Zoom`, asse 1D) è creata da codice e aggiunta con un Mapping Context a priorità alta: cambia `TargetArmLength` dello Spring Arm di Bot2. Sotto 40 cm si è in prima persona: corpo nascosto (`Owner No See`), telecamera all'altezza degli occhi, il personaggio ruota con la telecamera, compare la torcia (cilindro + Spot Light attaccati alla telecamera). La batteria (25 s) scende solo in prima persona, sotto il 20% la luce sfarfalla, a zero si spegne. Fuori dalla prima persona si ricarica piano (0,25 s/s); le batterie danno 15 s.
- **Posto sicuro e vite.** Ogni 0,2 s, se il personaggio è a terra su un pavimento fermo (non piattaforme mobili, che crollano, ascensori o casse) e **lontano dal bordo** (quattro line trace verso il basso intorno ai piedi trovano tutte il pavimento), quel punto diventa il posto sicuro. Cadendo in una `AFallZone` (o 25 m sotto l'ultimo posto sicuro) si perde una vita e si torna lì. A zero vite: **GAME OVER**, si torna al livello 1 e (opzione `bGameOverResetsAbilities`) si perdono le abilità. Le vite tornano a 3 a ogni livello nuovo; se il livello riparte (torretta mortale, vita a zero, timer) le vite restano quelle.
- **Oggetto al 35%.** In `BeginPlay`: `Random Bool with Weight 0.35`; se esce falso, `Destroy Actor`. Un messaggio dice se questa volta c'è.
- **Torretta mortale.** Stessa torretta di prima; con `bRestartLevelOnHit` il proiettile, invece di `Apply Damage`, chiama `KillPlayerAndRestart`.
- **Casse.** Al tasto F: se ci sono cariche, un line trace dall'alto davanti al personaggio trova il pavimento **o la cassa già lì** (così si impilano) e ci appoggia una cassa da 80 cm. Cariche a zero → `Set Timer` di 5 s → tornano 3. Le casse spariscono dopo 25 s e ne restano al massimo 6.
- **Lava.** `ADamageZone` con `DamagePerHit` 10 e `HitInterval` 1 s: 10 HP al secondo (con `Apply Damage`, quindi anche la barra di Bot2 scende).
- **Cambia modello.** Un `Set Timer by Event` in loop ogni secondo passa alla mesh successiva della lista (cubo, cilindro, sfera, cono) e al suo materiale.

L'HUD (in alto a destra) ora mostra anche le **vite**, il carburante del **jetpack**, le **casse** (o il tempo di ricarica) e la **batteria** della torcia; in prima persona c'è un mirino.

## Le classi C++ nuove (Source/AIV_UE5Intro/Platformer)

| Classe | Cosa fa | Lezione |
|---|---|---|
| `UPlatformerAbilitiesComponent` | Doppio salto controllabile, wall run, wall jump, coyote time, jump buffer, "premi E". Aggiunto a Bot2 dalla GameMode, senza toccare il Blueprint. | UE14 (Character Movement, forgiveness), UE12 (interazione) |
| `UPlatformerRunSubsystem` + `UPlatformerSaveGame` | Monete, abilità sbloccate, record di tempo per livello, morti; salvati su disco. Le monete raccolte valgono solo se arrivi al traguardo. | UE8 (Game Instance), GD16 (Save Game) |
| `APlatformerHUD` | Pannello in alto a destra (monete, morti, tempo, record, timer, abilità, vita), "[E] …", messaggi. Disegnato in Canvas: si vede anche in Shipping. | UE11 (UI) |
| `ACoinPickup`, `AAbilityPickup`, `AHealthPickup` | Collezionabili che ruotano e fluttuano, con luce e suono. | GD7, UE8 |
| `AActivable` → `AActivableDoor`, `AActivableLight`, `AActivableMover` | Classe base + figlie (ereditarietà): porta doppia (a battente o scorrevole), luce attivabile, ascensore. Transizione con curva (come una Timeline). | GD12 (porta doppia), UE7 (ereditarietà, luce attivabile) |
| `ASwitchBase` → `APressurePlate`, `AInteractLever` | Pedana a pressione e leva. Avvisano i bersagli con un riferimento diretto **e** con un event dispatcher (`OnSwitchChanged`). La leva implementa l'interfaccia `IPlatformerInteractable`. | UE7 (bottone, event dispatcher), UE12 (interfaccia) |
| `ATurret` + `APlatformerProjectile` | Torretta: rileva il giocatore con una sfera, ruota con interpolazione, pitch limitato (clamp), controlla la linea di vista con un trace, spara con un timer; opzionalmente anticipa il movimento. | UE4, UE5 |
| `AWindmill` | Pale che ruotano (rotator), la testa guarda il giocatore entro un angolo massimo. | UE4 |
| `ACubeSpawner` | Matrice di casse fisiche: due cicli for, trace verso il basso per appoggiarle. | UE5 |
| `AJumpPad`, `ACrumblingPlatform` | Pedana di lancio (Launch Character) e piattaforma che trema, crolla e ricompare. | GD12 |
| `AMovingPlatform` (aggiornata) | Ora ha materiale e suono, con pitch diverso all'andata e al ritorno. | GD13, UE6 (audio, pitch) |
| `ATeleportZone`, `AShopZone`, `AModifierZone` (ghiaccio) | Portale; "negozio" che spende monete e attiva qualcosa; zona che cambia attrito e frenata. | GD7, UE8, UE14 |
| `ASplineFence`, `AScatterArea`, `AFlickeringLight` | Staccionata lungo una spline "per lunghezza"; foliage a mano con un seme; luce del fuoco che tremola (rumore di Perlin). | UE16, UE15, UE13 |

`UPlatformerHealthComponent` e `ADamageZone` ora usano `ApplyDamage`, così si aggiorna anche la barra della vita di Bot2 (`WB_HUD1`).

### Doppio salto e wall run: i numeri (tutti modificabili nei Details del componente)

- Salto: `JumpZVelocity` 650 (≈2,15 m), `AirControl` 0,4, coyote time e jump buffer 0,15 s.
- Doppio salto: `DoubleJumpZVelocity` 600, `DoubleJumpSteering` 1 (quanto la nuova direzione sostituisce quella vecchia), `DoubleJumpAirControl` 0,8 fino all'atterraggio.
- Wall run: solo sui muri con il tag **`WallRun`** (`bRequireWallTag`). Si attacca se sei in aria, vicino al muro (80 cm) e vai almeno a 200 cm/s. Corri a 650 cm/s per al massimo 1,5 s, con una gravità ridotta (380). Salto = wall jump (600 via dal muro, 700 in alto); un salto da muro ti ridà il doppio salto.

## I tre livelli

I blocchi del percorso sono **`Blockout_Box` del plugin BlockOut Tools**, con un codice colore (come nel gym della lezione): grigio = percorso, arancio = salto/interazione, **blu = muro per il wall run**, rosso = pericolo, verde = traguardo. Se il plugin non si carica, lo script usa il cubo dell'engine con un materiale a griglia.

### Livello 1 – "Si impara" (mattina, isola sull'acqua)
*Nuovo:* il faro che cresce con la distanza; dopo il doppio salto il **generatore di casse** e una parete di 4,6 m (due casse + doppio salto); il **ponte fortunato** (35%) o la strada lunga sui sassi; il **totem** che cambia modello; le **3 vite** (cadere in acqua riporta all'ultimo posto sicuro).

Isola d'erba con staccionata su spline, alberi e cespugli, un mulino che ti segue con lo sguardo, una pila di casse fisiche, una pozzanghera (il materiale fisico dell'acqua cambia il suono dei passi). Una **pedana a pressione** apre una **porta doppia** nel muro e accende una lampada (che ascolta l'event dispatcher). Poi gradini con le monete, una **piattaforma mobile** con il suo suono, una **zona velocità** prima di un salto lungo, e **l'oggetto del doppio salto** sotto un faro azzurro. Subito dopo una parete di 3,3 m (con un salto solo non ci si arriva) e tre salti dove, se sbagli, ti correggi in aria. Chi cade in acqua perde una vita e torna all'ultimo posto sicuro.

### Livello 2 – "Ostacoli e congegni" (pomeriggio)
*Nuovo:* la lava toglie **10 HP al secondo**; dopo il negozio il **jetpack** e il salto lunghissimo; le **piattaforme che si restringono**; la **torretta mortale** con i ripari.

Pedana di lancio verso una sporgenza alta → piazza sotto il tiro di una **torretta**, con ripari e una cura → tre **piattaforme che crollano** → una **leva** ("[E] Chiama l'ascensore") → la **sezione di wall run**: due muri blu sfalsati, uno a destra e uno a sinistra; corri sul primo, salta sul secondo, esci sulla piattaforma. Se cadi, sotto c'è un pavimento con una pedana che ti rilancia all'inizio della sezione → corridoio di **lava** (toglie vita, con le decal delle bruciature) → la **sbarra che spinge** → piazza di **ghiaccio** con il "negozio": paghi 5 monete e sale un ponte sicuro, oppure rischi il ponte di ghiaccio gratis.

### Livello 3 – "Di notte, a tempo" (150 secondi)
*Nuovo:* dopo l'ascensore il **tunnel buio** da fare con la **torcia** in prima persona; la **piattaforma che cambia forma**; la **combo finale** (wall run → jetpack → casse + doppio salto) prima del traguardo.

(Il resto come prima.) Luna, nebbia volumetrica e **bracieri** con la luce che tremola. Piattaforma laterale → **portale** che ti teletrasporta su un'isola alta → **gravità ridotta** per un salto lunghissimo → piazza con la **sbarra che uccide** (da saltare) → un **wall run** veloce (o un doppio salto ben fatto) → due salti da doppio salto → una **torretta che anticipa** il tuo movimento → ascensore → tunnel, forme, combo → traguardo, che riporta al livello 1 e azzera le morti.

## Modificare i livelli

Tutto è in `Content/Python/gd13_platformer.py`: ogni riga è un pezzo del percorso con posizione e parametri in chiaro (cm, il percorso va lungo +X). Rilanciare lo script **ricrea** i livelli da capo. In alternativa si modificano a mano nell'editor, come qualunque livello: i congegni hanno i bersagli (`Targets`, `ListenTo`) impostabili con il contagocce nei Details.

## Asset usati e fonti (da mettere nella consegna)

| Asset | Dove è usato | Fonte |
|---|---|---|
| Scanlabz Interiors (mesh, texture, materiali) | alberi, cespugli, texture di legno, intonaco, quarzo, erba, tessuti | Fab / Unreal Marketplace – **inserire qui il link della pagina del pacchetto** (dalla libreria di Fab o dell'Epic Launcher) |
| BlockOut Tools Plugin (Dmitry Karpukhin) | tutti i blocchi del percorso (`Blockout_Box`) | https://dmkarpukhin.itch.io/blockout-tools-plugin (anche su Fab) |
| Bot2, IA_/IMC_ del Third Person, suoni in `/Game/Audio` | personaggio, input, suoni | progetto di partenza del corso (AIV) |
| Forme base dell'engine (Cube, Cylinder, Sphere, Cone) | torcia, jetpack, casse, totem, forme | Unreal Engine |

## Rifarlo in Blueprint (con un altro personaggio)

Ogni meccanica è un **componente** o un **actor** separato, quindi si può rifare una alla volta. Le corrispondenze:

| C++ | Blueprint |
|---|---|
| componente aggiunto dalla GameMode | aggiungere il componente nel Blueprint del personaggio (o tutta la logica nel personaggio) |
| `BindAction(IA_Jump, Started/Completed)` | eventi `EnhancedInputAction IA_Jump` → `Started` / `Completed` |
| azioni create da codice (zoom, casse) | creare `IA_Zoom` (Axis1D, Mouse Wheel Axis) e `IA_SpawnCrate` (Digital, F) e aggiungerli a un IMC con `Add Mapping Context` |
| `TickComponent` | `Event Tick` |
| `Move->Velocity.Z = FInterpConstantTo(...)` | `Get Character Movement` → `Velocity` → `Set Members in Vector` (Z) con `FInterp To Constant` |
| `Arm->TargetArmLength` | `Spring Arm` → `Set Target Arm Length` |
| `SetOwnerNoSee(true)` | `Mesh` → `Set Owner No See` |
| `LineTraceSingleByChannel` | `Line Trace By Channel` |
| `SetTimer(..., 5.f)` | `Set Timer by Event` |
| `FMath::FRand() < 0.35` | `Random Bool with Weight` (0.35) |
| `Destroy()` | `Destroy Actor` |
| `UPlatformerRunSubsystem` (vite, monete, abilità) | la vostra **Game Instance** (`GI_Platform`) con le stesse variabili |
| `KillPlayerAndRestart` | `Open Level` con `Get Current Level Name` |

Le classi restano nel progetto anche dopo: si possono usare come riferimento aprendo i `.h`, dove ogni parametro ha un commento.
