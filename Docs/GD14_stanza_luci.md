# GD14 – Una stanza illuminata con uno scopo

Task della lezione 14 di Level Design. La consegna chiede:

- una stanza nuova, costruita da zero con la geometria;
- solo Scan Labs e lo Starter Content (niente Fab, niente Medieval Village, niente stanza di Scan Labs riciclata);
- risultato pseudo-realistico, *a significant room*;
- luci usate in modo logico, con uno scopo preciso, **spiegando lo scopo**;
- almeno tre screenshot.

## L'idea: il salotto del lettore, al tramonto

Un piccolo salotto con un angolo lettura vicino alla finestra. Il sole del tardo pomeriggio entra basso da ovest, attraverso dei listelli di legno (un brise-soleil) che lo spezzano in strisce sul pavimento e sulla libreria. La lampada accanto alla poltrona è già accesa: la stanza sta passando dalla luce del giorno a quella della sera. Oltre la porta, il corridoio è freddo e buio.

Il tema visivo è il **contrasto caldo/freddo**: il sole e la lampada sono caldi (arancio), le ombre e il corridoio sono freddi (azzurri).

## Pianta

```
            y = 0  (muro OVEST, con la finestra)
     x=0 +----------[====FINESTRA====]--------------+ x=700
         |  lampada                                 |
         |     poltrona   tavolino     scatole      |---+
  muro   |                 (frutta)                 | P |  corridoio
  SUD    |  scarpe                                  | O |  (rect light
 (quadro,|                                          | R |   fredda)
  presa) |  pianta    [=====LIBRERIA=====]  statuetta| T |
         +------------------------------------------+---+
            y = 520 (muro EST)         muro NORD (quadro, porta)
```

Fuori dalla finestra ci sono solo un prato e una fila di alberi: è tutto ciò che si vede da dentro ("occhio non vede, cuore non duole").

## Le luci e il loro scopo

| # | Luce | Tipo | Scopo |
|---|------|------|-------|
| 1 | `Sole_TardoPomeriggio` | Directional, ~3300 K, bassa (pitch −22°) | È la **luce principale e la composizione**. Entra dalla finestra e, passando tra i listelli, disegna strisce di luce e ombra (il chiaroscuro di cui si è parlato a lezione, e il "ritmo" delle ombre di Totten). Le strisce guidano lo sguardo dalla finestra verso la libreria. **Una sola** directional light nel livello. |
| 2 | `Cielo_ColoreOmbre` | Sky Light, leggermente azzurra | Decide **di che colore sono le ombre**. Azzurrine, così fanno contrasto con il sole caldo: la stanza sembra quella di fine giornata senza bisogno di cambiare nient'altro. |
| 3 | `Lampada_LuceMotivata` | Point light, ~2600 K | La lampada "motiva" la luce dell'angolo lettura: il giocatore vede la lampada e accetta che quella luce venga da lì. Come nel barile di *Lies of P*, la luce **non è dentro il paralume** ma sopra e davanti, verso la poltrona, e la mesh della lampada **non proietta ombre** (la "lampada senza ombra"): così l'angolo è illuminato bene e non ci sono ombre brutte della lampada sul muro. |
| 4 | `Rinforzo_Libreria_SenzaOmbre` | Spot light debole, **Cast Shadows spento** | **Rafforza** la libreria sul muro est, che altrimenti sarebbe quasi tutta in ombra: i libri si leggono. Senza ombre non se ne accorge nessuno, e non si somma alle ombre delle altre luci (evita anche che Unreal spenga le ombre per troppe luci vicine). |
| 5 | `Corridoio_SegnalaUscita` | Rect light fredda oltre la porta | **Segnala l'uscita** (la luce agli incroci di Christopher Alexander, citato da Totten) e dà profondità all'inquadratura: il fondo freddo stacca la stanza calda. |
| – | `PostProcess_Globale` | Post Process Volume, Infinite Extent | Esposizione appena più bassa (per non "bruciare" le strisce di sole), vignetta leggera, bloom moderato. |

Più una `SkyAtmosphere` (serve al sole per colorare il cielo) e una `ExponentialHeightFog` leggera per l'esterno.

## Come l'ho fatto: uno script Python dell'editor

Non avendo Unreal a disposizione, ho costruito il livello con **uno script Python che gira dentro l'editor**: [`Content/Python/gd14_stanza_luci.py`](../Content/Python/gd14_stanza_luci.py). Lo script fa esattamente quello che si farebbe a mano (trascinare cubi, prop e luci e impostarne i valori nei Details), ma lo fa in automatico e in modo ripetibile. Il risultato è un livello normale, salvato in `Content/Maps/L_GD14_StanzaLuci`. Dopo averlo generato ci si lavora con l'editor come con qualunque altro livello.

Perché Python e non C++ o Blueprint:

- **C++** richiederebbe di trasformare il progetto (oggi solo Blueprint) in un progetto C++ e di compilarlo con Visual Studio. Inoltre un attore C++ che piazza oggetti a runtime non lascerebbe nel livello oggetti spostabili a mano.
- Un **Blueprint** con il Construction Script va bene per cose ripetitive come la staccionata lungo una spline, ma è un file binario: non lo potevo scrivere da qui.
- Lo **script Python dell'editor** crea attori veri, salvati nella mappa, come se li avessimo trascinati noi. È testo, quindi si legge, si modifica e si versiona su Git.

### Lanciarlo

1. Apri il progetto. Nel `.uproject` ho abilitato i plugin **Python Editor Script Plugin** ed **Editor Scripting Utilities**: se Unreal chiede di riavviare, riavvia.
2. Lancia lo script in uno dei due modi:
   - **Tools → Execute Python Script…** e scegli `Content/Python/gd14_stanza_luci.py`;
   - oppure dalla **Output Log**: imposta il menu a sinistra della riga di comando su `Python` e scrivi `import gd14_stanza_luci; gd14_stanza_luci.costruisci()`.
3. Lo script crea `L_GD14_StanzaLuci`, lo riempie (circa 70 attori, ordinati in cartelle nell'Outliner: `Stanza`, `Mobili`, `Prop`, `Luci`, `Telecamere`, `Esterno`) e lo salva. Nella Output Log ogni riga inizia con `[GD14]`. Se manca un asset compare un avviso e lo script va avanti.

**Attenzione:** rilanciare lo script **cancella e ricrea** `L_GD14_StanzaLuci`. Se ci hai lavorato a mano, prima fai *File → Save Current Level As…* con un altro nome. Se il livello è aperto e non si lascia cancellare, apri un altro livello e rilancia.

### Come funziona (in breve)

- **Muri con i buchi.** Ogni muro è un cubo dell'engine (`/Engine/BasicShapes/Cube`, 100 cm) scalato. Per la finestra e la porta il muro è diviso in più pezzi che lasciano il buco. È il blockout a primitivi visto nel capitolo 11.
- **Si piazza l'ingombro, non il pivot.** La funzione `piazza_mesh` legge il *bounding box* della mesh e la posiziona dicendo "il suo centro qui", "il suo bordo contro il muro", "appoggiata a questa quota". Così gli oggetti finiscono a terra, sul tavolino o contro il muro senza conoscere dove ogni modello ha il pivot.
- **La finestra di Scan Labs** (`SM_Window`) viene scalata per riempire esattamente il buco nel muro.
- **Libreria e tavolino** sono fatti di cubi con il legno di Scan Labs ("arrangiarsi con quello che si ha"). I gruppi di libri (`SM_Books_Mix_1…8`) vengono messi sui ripiani finché c'è posto, con spazi irregolari tra un gruppo e l'altro.
- **Il pizzico di caos.** Ogni prop ha un grado o due di rotazione casuale, le scarpe sono storte, le scatole del trasloco sono sghembe e una è appoggiata sull'altra. Il caso è "a seme" (`SEME_CAOS`): rilanciando esce sempre lo stesso disordine; cambiando il numero ne esce un altro.
- **Il pavimento** usa il parquet dello Starter Content (`M_Wood_Floor_Walnut_Polished`) se lo trova, altrimenti il quarzo di Scan Labs.

### Modificare

Tutto ciò che conta è in cima allo script, nelle costanti in MAIUSCOLO:

- **misure della stanza**, della finestra e della porta (`LUNGHEZZA`, `FINESTRA_X`, `PORTA_Y`, …);
- **luci**: inclinazione e direzione del sole (`SOLE_PITCH`, `SOLE_YAW`), intensità in candele o lux, temperatura in kelvin, colori;
- **prop**: la lista `PROP`, con mesh, posizione `x, y`, rotazione `yaw`, scala, `ombra=False` per non proiettare ombre, e `z="TAVOLINO"` o `z="SOPRA:NomeOggetto"` per appoggiare un oggetto sopra un altro;
- **telecamere**: la lista `TELECAMERE`.

Si può anche **non toccare lo script**: si genera il livello una volta e poi si sposta tutto a mano nell'editor, come al solito.

### Cose da controllare la prima volta

Lo script è stato scritto senza poterlo eseguire in Unreal, quindi alcune cose vanno guardate a occhio:

1. **Verso dei mobili.** Lo script non può sapere da quale lato "guarda" la poltrona di Scan Labs, né da che parte hanno il dorso i gruppi di libri. Se un oggetto è girato male, ruotalo di 90° o 180° (nell'editor, oppure cambiando il suo `yaw` nella lista `PROP`; per i libri la riga `yaw = 90.0 + ...` in `costruisci_mobili`).
2. **Le strisce di sole.** Se non cadono dove vuoi, cambia `SOLE_YAW` (direzione) e `SOLE_PITCH` (altezza). Basta anche ruotare la directional light nell'editor.
3. **Esposizione.** Con Lumen e l'esposizione automatica l'interno può sembrare più scuro o più chiaro del previsto: regola `auto_exposure_bias` nel Post Process o l'intensità del sole.
4. **Nomi degli asset.** Se nella Output Log compare `asset mancante`, il percorso di quell'asset nel tuo progetto è diverso: correggilo nella costante corrispondente.

## Gli screenshot

Nel livello ci sono tre CineCamera, nella cartella `Telecamere`:

- `Cam_1_Insieme`: tutta la stanza, dall'angolo sud-est verso la finestra;
- `Cam_2_AngoloLettura`: la poltrona, la lampada e le strisce di sole;
- `Cam_3_LibreriaEPorta`: la libreria rafforzata e la porta con il corridoio freddo.

Per ciascuna: tasto destro sulla telecamera → **Pilot**, premi **G** per togliere le icone, poi nella Output Log scrivi `HighResShot 2`. Le immagini finiscono in `Saved/Screenshots/`.

## Testo da consegnare insieme agli screenshot

> Ho fatto un piccolo salotto con un angolo lettura, al tramonto. La luce principale è il sole basso che entra dalla finestra a ovest e viene spezzato dai listelli di legno esterni: le strisce di luce e ombra danno ritmo al pavimento e portano lo sguardo verso la libreria. La Sky Light è leggermente azzurra, così le ombre sono fredde e contrastano con il sole caldo. La lampada accanto alla poltrona motiva la luce dell'angolo lettura, ma la point light non è dentro il paralume: è sopra e davanti, verso la poltrona, e la lampada non proietta ombre, come negli esempi di Lies of P. La libreria, che sarebbe in ombra, è rafforzata da una spot light debole senza ombre. Infine una rect light fredda nel corridoio oltre la porta segnala l'uscita e stacca il fondo dell'inquadratura.
