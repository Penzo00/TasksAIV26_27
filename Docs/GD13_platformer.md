# GD13 – Platformer 3D in tre livelli, con Bot2

Il task di game programming (lezioni 12–13) chiede:

- un platformer 3D con il personaggio base, che cammina e salta;
- **almeno tre livelli**: finito un livello si passa al successivo con un **Open Level**;
- degli **ostacoli**, alcuni dei quali fanno **ricominciare il livello** (trappole, buchi, qualcosa che toglie vita);
- delle **zone che cambiano il personaggio** (corre più veloce, salta più in alto…);
- **un livello a tempo**: se non lo si finisce entro il tempo, riparte;
- qualche asset per decorare (qui si usano gli asset già nel progetto).

Il personaggio è **BP_Bot2**.

## Cosa c'è

### Le classi C++ (`Source/AIV_UE5Intro/Platformer/`)

Sono i concetti della lezione 13 (componenti, ereditarietà, Game Instance, Delta Time) scritti in C++. Ogni parametro è una proprietà modificabile nei **Details** dell'actor.

| Classe | Che cos'è | Parametri principali |
|---|---|---|
| `PlatformerGameMode` | La GameMode dei livelli dei task: il pawn è `BP_Bot2`. | il percorso di BP_Bot2 è nel `.cpp` |
| `PlatformerRunSubsystem` | Lo stato che sopravvive ai cambi di livello (un *Game Instance Subsystem*): **conta le morti**, fa ripartire il livello, apre il successivo. È come il contatore delle morti nella Game Instance della lezione, ma non obbliga a toccare `GI_Platform`. | – |
| `PlatformerHealthComponent` | Il **componente vita** (come l'Health Component della lezione). A zero il livello riparte. Se Bot2 non ce l'ha, le zone di danno glielo aggiungono da sole. | `MaxHealth`, `bRestartLevelOnDeath`, `RestartDelay` |
| `PlatformerZone` | **Classe base** (astratta) di tutte le zone: una scatola che reagisce quando entra o esce il personaggio, con una pedana visibile sul fondo e una scritta. È l'**ereditarietà** della lezione: le figlie cambiano solo *cosa succede*. | `ZoneExtent`, `bOnlyPlayer`, `bShowVisual`, `VisualMaterial`, `VisualThickness`, `Label` |
| ↳ `KillZone` | Trappola: chi la tocca riparte (buchi, cadute). | `RestartDelay` |
| ↳ `DamageZone` | Toglie vita a intervalli finché si resta dentro (la Damage Area della lezione). | `DamagePerHit`, `HitInterval` |
| ↳ `ModifierZone` | Cambia il personaggio: velocità, salto, gravità. L'effetto finisce dopo un po' che si è usciti, oppure resta per tutto il livello. | `WalkSpeedMultiplier`, `JumpMultiplier`, `GravityMultiplier`, `DurationAfterExit` (0 = finisce subito, negativo = permanente) |
| ↳ `LevelGoal` | Traguardo: **Open Level** del livello successivo. | `NextLevel`, `Message`, `Delay`, `bResetDeathCount` |
| `LevelTimer` | Conto alla rovescia, scritto a schermo e nel mondo. Quando arriva a zero il livello riparte. Usa il **Delta Time**, quindi va alla stessa velocità a qualunque fps. Il traguardo lo ferma. | `TimeLimit`, `WarningThreshold`, `bShowOnScreen`, `bShowInWorld` |
| `MovingPlatform` | Piattaforma che va avanti e indietro con pausa agli estremi (Tick + Lerp, come in lezione). Il punto di arrivo si trascina nel viewport con il rombo del widget. | `EndOffset`, `TravelTime`, `PauseTime`, `StartDelay`, `bSmooth`, `PlatformSize` |
| `SpinningObstacle` | Sbarra rotante che spinge via il personaggio, oppure lo fa ripartire. | `DegreesPerSecond`, `BarSize`, `BarHeight`, `PushStrength`, `bKillOnTouch` |

Tutte le classi si trovano nel Content Browser sotto **C++ Classes → AIV_UE5Intro → Platformer**. Si trascinano nel livello come qualunque actor. Se ti serve aggiungere grafica o logica in Blueprint, crea un **Blueprint figlio** (tasto destro sulla classe → *Create Blueprint class based on…*).

### I tre livelli (`Content/Maps/L_GD13_Livello1/2/3`)

Li costruisce lo script `Content/Python/gd13_platformer.py`.

| Livello | Cosa si impara | Pezzi |
|---|---|---|
| **1 – Si impara** | salti e gradini, poi correre | piattaforme con salti di 2 m, gradini di 60 cm, **zona velocità** (×1,8 per 4 s) prima di un salto lungo, traguardo → livello 2 |
| **2 – Ostacoli** | usare i bonus e non farsi male | **pedana di salto** (×1,7) per salire su una sporgenza di 2,5 m, **piattaforma mobile** sul vuoto, passerella con **zona che toglie vita** (20 ogni 0,4 s), **sbarra rotante** che spinge giù, traguardo → livello 3 |
| **3 – A tempo** | fare tutto in **60 secondi** | piattaforma mobile laterale, **gravità ridotta** per un salto lungo, **sbarra che uccide** da saltare, **ascensore**, traguardo → di nuovo livello 1 (azzera le morti) |

In ogni livello, sotto il percorso c'è una `KillZone` invisibile: chi cade riparte. Ogni livello ha PlayerStart, sole, cielo e la GameMode `PlatformerGameMode` impostata nei World Settings.

## Come si usa

1. **Compila** il progetto: apri `AIV_UE5Intro.slnx` e fai *Build*, oppure apri l'editor e lascia che compili il modulo.
2. In Unreal: **Tools → Execute Python Script…** → `Content/Python/gd13_platformer.py`. Lo script crea i tre livelli e li salva. Rilanciarlo li **cancella e ricrea**.
3. Apri `L_GD13_Livello1` e premi **Play**.

## Modificare

- **A mano, nell'editor**: seleziona una zona, una piattaforma o una sbarra e cambia i valori nei Details. Ogni parametro ha un tooltip in italiano.
- **Nello script**: le funzioni `livello_1/2/3` sono un elenco di pezzi, una riga per pezzo, con posizione (cm, il percorso va lungo +X) e parametri in chiaro. Per esempio, per un salto più facile basta avvicinare una `pedana(...)`.
- **Nel C++**: per una zona nuova, crea una classe figlia di `APlatformerZone` e ridefinisci `OnCharacterEntered` / `OnCharacterExited`.

## Cose da controllare la prima volta

Il codice è stato scritto senza poter compilare o provare il progetto, quindi:

1. **Compilazione**: se il compilatore segnala un errore, mandami il messaggio e lo correggo.
2. **Distanze dei salti**: dipendono da velocità e forza del salto di Bot2, che lo script non conosce. Se un salto è impossibile o troppo facile, sposta la pedana (nell'editor o nello script) o cambia i moltiplicatori delle zone.
3. **Materiali delle pedane**: le zone usano materiali già presenti (blu, oro, `M_LIGHT` per il danno). Se uno non ti piace, cambia `VisualMaterial` nei Details.
4. **Messaggi a schermo** (timer, vita, morti): usano *On Screen Debug Message*, quindi si vedono in editor e nelle build di sviluppo, ma non in una build *Shipping*. Per un'interfaccia vera si può fare un widget che legge `GetTimeLeft()` e `GetDeathCount()`, già esposti ai Blueprint.
