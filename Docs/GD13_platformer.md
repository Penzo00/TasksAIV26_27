# GD13 – Platformer in tre livelli (con quello che abbiamo visto a lezione)

Tre livelli giocabili con **Bot2** (che resta il personaggio del giocatore), costruiti da uno script Python dell'editor e fatti di **classi C++ del progetto**, tutte con i parametri esposti nei Details. In più, rispetto alla consegna:

- **Wall run e wall jump** (con una sezione dedicata nel livello 2 e un passaggio veloce nel livello 3).
- **L'oggetto del doppio salto** nel livello 1: lo raccogli e da lì in poi, premendo salto in aria, fai un secondo salto **controllabile** (la direzione si corregge verso dove stai spingendo lo stick/WASD). Se manchi una piattaforma con il primo salto, col secondo ti salvi.

## Primo avvio: cosa controllare

1. **Compilare** (Visual Studio/Rider, oppure "Compile" nell'editor). Ho aggiunto il modulo `EnhancedInput` a `AIV_UE5Intro.Build.cs`. Non ho potuto compilare da qui: se c'è un errore, mandatemi il log e lo sistemo.
2. **Riavviare l'editor una volta.** In `Config/DefaultEngine.ini` c'è un `[CoreRedirects]` per il pacchetto Scan Labs: i suoi asset sono stati spostati da `Content/Scanlabz_Interiors_1/` alla radice di `Content`, e i riferimenti interni (mesh → materiali → texture) puntavano ancora alla vecchia cartella. Per questo i prop apparivano grigi. Dopo il riavvio conviene fare *Fix Up Redirectors* sulla cartella `Content` e salvare tutto.
3. **Lanciare gli script** (Tools > Execute Python Script…): prima `Content/Python/aiv_materiali.py` (crea i materiali in `/Game/AIV/Materiali`), poi `gd13_platformer.py`. Lo script dei livelli chiama comunque i materiali da solo.
4. Aprire `L_GD13_Livello1` e premere **Play**.
5. Se volete provare il livello 2 o 3 senza passare dal primo: nella GameMode (`PlatformerGameMode`, World Settings) c'è `bDoubleJumpForTesting`, oppure da console `AIVUnlockAll`. `AIVResetProgress` azzera il salvataggio.

Controlli: quelli di Bot2 (WASD, Shift per correre, Spazio per saltare). il tasto di `IA_Interact` (già nell'`IMC_Default`) per interagire con le leve.

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
Isola d'erba con staccionata su spline, alberi e cespugli, un mulino che ti segue con lo sguardo, una pila di casse fisiche, una pozzanghera (il materiale fisico dell'acqua cambia il suono dei passi). Una **pedana a pressione** apre una **porta doppia** nel muro e accende una lampada (che ascolta l'event dispatcher). Poi gradini con le monete, una **piattaforma mobile** con il suo suono, una **zona velocità** prima di un salto lungo, e **l'oggetto del doppio salto** sotto un faro azzurro. Subito dopo una parete di 3,3 m (con un salto solo non ci si arriva) e tre salti dove, se sbagli, ti correggi in aria. Chi cade in acqua riparte.

### Livello 2 – "Ostacoli e congegni" (pomeriggio)
Pedana di lancio verso una sporgenza alta → piazza sotto il tiro di una **torretta**, con ripari e una cura → tre **piattaforme che crollano** → una **leva** ("[E] Chiama l'ascensore") → la **sezione di wall run**: due muri blu sfalsati, uno a destra e uno a sinistra; corri sul primo, salta sul secondo, esci sulla piattaforma. Se cadi, sotto c'è un pavimento con una pedana che ti rilancia all'inizio della sezione → corridoio di **lava** (toglie vita, con le decal delle bruciature) → la **sbarra che spinge** → piazza di **ghiaccio** con il "negozio": paghi 5 monete e sale un ponte sicuro, oppure rischi il ponte di ghiaccio gratis.

### Livello 3 – "Di notte, a tempo" (90 secondi)
Luna, nebbia volumetrica e **bracieri** con la luce che tremola. Piattaforma laterale → **portale** che ti teletrasporta su un'isola alta → **gravità ridotta** per un salto lunghissimo → piazza con la **sbarra che uccide** (da saltare) → un **wall run** veloce (o un doppio salto ben fatto) → due salti da doppio salto → una **torretta che anticipa** il tuo movimento → ascensore → traguardo, che riporta al livello 1 e azzera le morti.

## Modificare i livelli

Tutto è in `Content/Python/gd13_platformer.py`: ogni riga è un pezzo del percorso con posizione e parametri in chiaro (cm, il percorso va lungo +X). Rilanciare lo script **ricrea** i livelli da capo. In alternativa si modificano a mano nell'editor, come qualunque livello: i congegni hanno i bersagli (`Targets`, `ListenTo`) impostabili con il contagocce nei Details.
