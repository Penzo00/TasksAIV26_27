# GD14 – Il rifugio all'ora blu

Task della lezione 14 di Level Design. La consegna chiede una stanza nuova costruita da zero con la geometria, solo Scan Labs e Starter Content (niente Fab, niente Medieval Village, niente stanza di Scan Labs riciclata), un risultato pseudo-realistico e *significativo*, luci usate con uno scopo **spiegato**, almeno tre screenshot.

> Nota: lo Starter Content non è nel progetto, quindi ho usato solo Scan Labs (mesh e texture) più le forme base dell'engine e il plugin BlockOut Tools.

## L'idea

La soffitta di un rifugio di montagna, nel momento in cui fa buio. Fuori il blu del crepuscolo e la luna; dentro il fuoco acceso, la lampada vicino alla poltrona, la lampada sul tavolo. **Qualcuno è appena rientrato**: le scarpe bagnate (con le orme) accanto alla porta ancora socchiusa, la legna accatastata vicino al camino, due bicchieri sul tavolino, un libro lasciato aperto.

Il tema visivo è il **contrasto caldo/freddo**: il fuoco (1850 K) è il punto focale e la luce chiave; la finestra e la porta (8500–9500 K) sono il contrappunto freddo. Le immagini di riferimento (interno di baita, tetto spiovente con travi a vista, camino, tappeto, poltrona) hanno dato il **mood**; pianta, arredo e luci sono progettati da zero.

## Pianta (8 × 6 m, tetto a due falde, colmo a 4,5 m)

```
                 y = 600  (falda nord, muretto 1,8 m)
   x=0 +--------------------------------------------------+ x=800
 PORTA |  pianta   [====== LIBRERIA BASSA ======]  lampada | legna
 soc-  |  scarpe                      poltrona N    da terra|
 chiusa|                 ( tappeto  )                 +----+
       |   [ TAVOLO ]    (  pouf    )                 |FUOCO| CAMINO
FINE-  |    panca        (          )                 +----+ (pietra,
 STRA  |                              poltrona S            |  mensola,
(luna) |  [===== CUCINA: mobili dipinti, mensola =====]     |  candele)
       +--------------------------------------------------+
                 y = 0  (falda sud, muretto 1,8 m)
```

L'asse della stanza va dalla **finestra (fredda)** al **camino (caldo)**: entrando dalla porta lo sguardo corre lungo tutta la stanza fino al fuoco.

## Le luci e il loro scopo

| # | Luce | Tipo | Scopo |
|---|---|---|---|
| 1 | `Luce_Fuoco_Chiave` | `AFlickeringLight` (C++), point 130 cd, 1850 K, ombre | **Luce chiave e punto focale.** Tutto nella stanza è girato verso il fuoco (le poltrone, il tappeto). Tremola con un rumore di Perlin (intensità e un poco la posizione): le ombre "respirano" come con una fiamma vera, e insieme tremola il materiale emissivo delle braci. |
| 2 | `Fuoco_Rimbalzo_SenzaOmbre` | Point, debole, raggio sorgente grande, **senza ombre** | La luce che esce dalla bocca del camino e scalda il tappeto e le poltrone. Senza ombre e morbida: non disegna ombre doppie, completa la 1. |
| 3 | `Luna` | Directional 2,5 lux, 8500 K | **Controluce freddo** dalla finestra: disegna la finestra (con la croce del telaio) sul pavimento e, con la nebbia volumetrica, i raggi nell'aria. È anche il "sole" dell'atmosfera: colora il cielo di blu profondo. Una sola directional nel livello. |
| 4 | `Cielo` | Sky Light, real time capture, bassa | **Il colore delle ombre**: dove il fuoco non arriva la stanza resta blu, non nera. È il caldo/freddo "gratis". |
| 5 | `Porta_LuceFredda` | Rect light fuori dalla porta, 9500 K | Una **lama di luce blu** entra dalla porta socchiusa e corre sul pavimento: guida l'occhio verso l'uscita (la luce sui passaggi di Totten/Alexander) e racconta che qualcuno è appena entrato. |
| 6 | `Luce_Lampada_Terra` | Point 22 cd, 2700 K, ombre | **Luce pratica** dell'angolo lettura. È motivata (la lampada si vede), ma la point light è sotto il paralume e la mesh della lampada **non proietta ombre**: niente ombra brutta della lampada sul muro (la "lampada senza ombra" degli esempi di *Lies of P*). |
| 7 | `Luce_Sospensione` | Point 28 cd, 2700 K, ombre | Un **cerchio di luce sul tavolo**: separa la zona pranzo dal soggiorno e mette in risalto la frutta, secondo punto d'interesse. |
| 8 | `Luce_Candela_S/N` | Point 2,5 cd, 2000 K, **senza ombre** | Piccoli **accenti** sulla mensola (la foto, il vaso). Senza ombre perché sono minuscole e vicine al muro. |
| 9 | `Libreria_Riempimento_SenzaOmbre` | Spot debole, 3200 K, **senza ombre** | **Riempimento**: il fuoco prende la libreria di taglio e i dorsi dei libri sarebbero illeggibili. Senza ombre non si nota e non si somma alle ombre delle altre luci. |
| – | `PostProcess_Stanza` | Post Process Volume, infinito | Esposizione automatica limitata (non "rincorre" le fiamme), bloom per fuoco e candele, vignetta per chiudere l'inquadratura, un filo di grana, bilanciamento del bianco appena caldo. |
| – | `Nebbia` | Exponential Height Fog **volumetrica** | Rende visibili i raggi della luna e l'alone del fuoco. |

Tutte le luci sono **Movable** (il progetto usa Lumen con `r.AllowStaticLighting=False`).

## Come è costruita

Lo script Python dell'editor [`Content/Python/gd14_stanza_luci.py`](../Content/Python/gd14_stanza_luci.py) fa quello che si farebbe a mano, in modo ripetibile. Il risultato è un livello normale, che poi si ritocca nell'editor.

- **Due fasi, come nel level design vero.** `costruisci(blockout=True)` crea `L_GD14_Stanza_Blockout`: solo volumi del plugin **BlockOut Tools** (`Blockout_Box`, colorati per funzione) e i "proxy" dei mobili, per decidere misure e inquadrature. `costruisci()` crea `L_GD14_StanzaLuci`, la stanza finita.
- **Geometria da zero.** Muri, pavimento, camino e mobili sono `Blockout_Box` con un materiale nostro (*Custom Material*); le falde e le travi inclinate sono cubi orientati. Il muro con finestra e porta è fatto "a pezzi" intorno ai buchi. Le travi a vista (colmo, puntoni ogni metro, due catene) seguono la pendenza del tetto.
- **Materiali World Aligned** ([`aiv_materiali.py`](../Content/Python/aiv_materiali.py)): le texture di Scan Labs (legno, intonaco, quarzo per la pietra, tessuti, rattan, corteccia) sono proiettate dai tre assi del mondo, quindi non si stirano sui cubi scalati. Le braci e le fiamme hanno un materiale emissivo animato con il rumore (come la lava). Decal per la fuliggine del camino, la cenere e le orme bagnate.
- **Prop di Scan Labs** (poltrone, lampade, libri, quadri, frutta, bicchieri, scarpe, pianta, castagno e bosco fuori dalla finestra), piazzati per **ingombro** e non per pivot: lo script legge il bounding box della mesh e la appoggia a terra, sul tavolo o contro il muro; se è troppo grande per il posto la rimpicciolisce.
- **Giocabile**: c'è un PlayerStart e la GameMode del progetto, quindi con **Play** ci si cammina dentro con Bot2.

### Lanciarlo

1. Riavviare l'editor una volta dopo aver preso il branch (redirect di Scan Labs in `DefaultEngine.ini`, vedi la doc del GD13).
2. Tools → **Execute Python Script…** → `Content/Python/gd14_stanza_luci.py`, oppure nella Output Log (riga `Python`):
   `import gd14_stanza_luci; gd14_stanza_luci.costruisci()` (e `costruisci(blockout=True)` per il blockout).
3. Rilanciare **ricrea** il livello: se ci avete lavorato a mano, salvatelo prima con un altro nome.

### Da guardare la prima volta (lo script non è stato eseguito in Unreal)

1. **Verso dei mobili.** Lo script non sa da che lato "guarda" una mesh di Scan Labs. Se le poltrone guardano il muro invece del fuoco, cambiare `FRONTE_SCANLABS` in cima allo script (0, 90, 180 o 270) e rilanciare, oppure ruotarle a mano.
2. **Esposizione.** Con Lumen e l'esposizione automatica l'interno può venire più chiaro o più scuro del previsto: regolare `auto_exposure_bias` nel `PostProcess_Stanza`, oppure `FUOCO_CD` / `LUNA_LUX` in cima allo script.
3. **Lampada a sospensione.** Se la mesh ha un cavo molto lungo lo script la rimpicciolisce per tenerla sopra il tavolo; se non convince, spostarla a mano (e la sua luce con lei).
4. Se nella Output Log compare `asset mancante`, quel percorso nel vostro progetto è diverso: correggerlo nella costante.

## Gli screenshot

Quattro CineCamera nella cartella `Camere`:

- `Cam_1_Insieme`: dall'angolo della cucina verso il camino, tutta la stanza con tavolo, tappeto e poltrone;
- `Cam_2_Camino`: il fuoco, la mensola con le candele, le poltrone ai lati;
- `Cam_3_Controcampo_Finestra`: dal camino verso la finestra e la porta, caldo in primo piano e freddo sullo sfondo;
- `Cam_4_AngoloLettura`: la poltrona, la lampada da terra, il tavolino con il libro aperto.

Per ognuna: tasto destro → **Pilot**, **G** per togliere le icone, poi nella Output Log `HighResShot 2`. Le immagini finiscono in `Saved/Screenshots/`.

## Testo da consegnare insieme agli screenshot

> Ho costruito la soffitta di un rifugio di montagna all'ora blu, quando fuori fa buio e dentro si è appena acceso il fuoco. L'idea di luce è il contrasto caldo/freddo. La luce chiave è il fuoco del camino, che è anche il punto focale: le poltrone e il tappeto sono rivolti verso di lui, e la sua luce tremola con un rumore di Perlin, così le ombre si muovono come con una fiamma vera; una seconda luce senza ombre, morbida, simula il calore che esce dalla bocca del camino. Dalla finestra entra la luna, fredda, che disegna il telaio sul pavimento e, con la nebbia volumetrica, i raggi nell'aria; la Sky Light bassa e blu decide il colore delle ombre. Dalla porta socchiusa entra una lama di luce blu che porta lo sguardo verso l'uscita e racconta che qualcuno è appena rientrato (ci sono le scarpe bagnate e le orme). Le luci pratiche sono motivate: la lampada da terra illumina l'angolo lettura, ma la sua luce è sotto il paralume e la lampada non fa ombra; la lampada a sospensione crea un cerchio di luce sul tavolo da pranzo; due candele accendono la mensola. Infine una spot debole e senza ombre rende leggibili i libri, che il fuoco prende di taglio.
