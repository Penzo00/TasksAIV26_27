# GD14 – Il rifugio colorato

Task della lezione 14 di Level Design. La consegna chiede una stanza nuova costruita da zero con la geometria, solo Scan Labs e Starter Content (niente Fab, niente Medieval Village, niente stanza di Scan Labs riciclata), un risultato pseudo-realistico e *significativo*, luci usate con uno scopo **spiegato**, almeno tre screenshot.

> Nota: lo Starter Content non è nel progetto, quindi ho usato solo Scan Labs (mesh e texture) più le forme base dell'engine e il plugin BlockOut Tools.

## L'idea

La soffitta di un rifugio di montagna, **calda e colorata**, al tramonto. Lo stile prende il *mood* della foto di riferimento (una baita accogliente, quasi da fiaba): muri con una carta da parati arancio a rombi (procedurale), soffitto a perline con travi scure, un **camino di mattoni ad arco** con il focolare tondo, credenze dipinte di viola con i pomelli d'ottone, un tappeto tondo a cerchi verdi e gialli, mensole piene di oggetti, un letto con le colonnine tornite e un comò, l'angolo cucina con l'alzatina di mattoni e le pentole appese, una lama di sole che entra dalla finestra sulle assi del pavimento. Pianta, arredo e luci sono progettati da zero: **non** ci sono personaggi, oggetti o loghi del gioco da cui viene la foto.

La storia: **qualcuno è appena rientrato**. Le scarpe bagnate (con le orme) accanto alla porta socchiusa, la legna accatastata vicino al camino, due bicchieri sul tavolino, un libro lasciato aperto.

Il tema di luce è il **contrasto caldo/freddo**: il fuoco (1850 K) è il punto focale; il sole al tramonto (2400 K, radente) e il cielo dalla finestra e dalla porta sono la luce "di fuori". `ORA` in cima allo script cambia l'ora: `"tramonto"` (predefinita, stanza calda e in penombra come nelle foto di riferimento), `"pomeriggio"` (più luce, sole 4300 K) o `"ora_blu"` (luna 8500 K, cielo basso e blu).

> Sul "pseudo-realistico" della consegna: le forme e i colori sono volutamente un po' da cartone, ma i materiali sono le texture fotografiche di Scan Labs e la luce è fisica (Lumen, temperature in Kelvin, lux e candele): il risultato è una baita stilizzata illuminata in modo credibile. Se si vuole più realismo basta cambiare i materiali di muri e mobili (`IntonacoArancio` → `Intonaco`, `LegnoViola` → `LegnoDipinto`).

## Pianta (8 × 6 m, tetto a due falde, colmo a 4,5 m)

```
                 y = 600  (falda nord, muretto 1,8 m)
   x=0 +--------------------------------------------------+ x=800
 PORTA |  pianta   [====== LIBRERIA BASSA ======]  lampada | legna
 soc-  |  scarpe                      poltrona N    da terra|
 chiusa|                 ( tappeto  )                 +----+
       |   [ TAVOLO ]    (  pouf    )                 |FUOCO| CAMINO
FINE-  |    panca        (          )                 +----+ (mattoni,
 STRA  |                              poltrona S            |  ad arco,
(sole) |  [===== CUCINA: mobili dipinti, mensola =====]     |  mensole)
       +--------------------------------------------------+
                 y = 0  (falda sud, muretto 1,8 m)
```

L'asse della stanza va dalla **finestra (fredda)** al **camino (caldo)**: entrando dalla porta lo sguardo corre lungo tutta la stanza fino al fuoco.

## Le luci e il loro scopo

| # | Luce | Tipo | Scopo |
|---|---|---|---|
| 1 | `Luce_Fuoco_Chiave` | `AFlickeringLight` (C++), point 130 cd, 1850 K, ombre | **Luce chiave e punto focale.** Tutto nella stanza è girato verso il fuoco (le poltrone, il tappeto). Tremola con un rumore di Perlin (intensità e un poco la posizione): le ombre "respirano" come con una fiamma vera, e insieme tremola il materiale emissivo delle braci. |
| 2 | `Fuoco_Rimbalzo_SenzaOmbre` | Point, debole, raggio sorgente grande, **senza ombre** | La luce che esce dalla bocca del camino e scalda il tappeto e le poltrone. Senza ombre e morbida: non disegna ombre doppie, completa la 1. |
| 3 | `Sole` (o `Luna`) | Directional: tramonto 12 lux, 2400 K, radente (-9°); pomeriggio 40 lux, 4300 K; ora blu 2,5 lux, 8500 K | **La lama di luce dalla finestra**: disegna la finestra (con la croce del telaio) sulle assi del pavimento e, con la nebbia volumetrica, i raggi nell'aria. È anche il sole dell'atmosfera: colora il cielo. Una sola directional nel livello. |
| 4 | `Cielo` | Sky Light, real time capture | **Il colore delle ombre**: di pomeriggio le schiarisce (i colori della stanza restano leggibili), all'ora blu è bassa e la stanza dove il fuoco non arriva resta blu, non nera. |
| 5 | `Porta_LuceEsterna` | Rect light fuori dalla porta, 3200 K al tramonto (5500 K di pomeriggio, 9500 K all'ora blu) | Una **lama di luce** entra dalla porta socchiusa e corre sul pavimento: guida l'occhio verso l'uscita (la luce sui passaggi di Totten/Alexander) e racconta che qualcuno è appena entrato. |
| 6 | `Luce_Lampada_Terra` | Point 22 cd, 2700 K, ombre | **Luce pratica** dell'angolo lettura. È motivata (la lampada si vede), ma la point light è sotto il paralume e la mesh della lampada **non proietta ombre**: niente ombra brutta della lampada sul muro (la "lampada senza ombra" degli esempi di *Lies of P*). |
| 7 | `Luce_Sospensione` | Point 28 cd, 2700 K, ombre | Un **cerchio di luce sul tavolo**: separa la zona pranzo dal soggiorno e mette in risalto la frutta, secondo punto d'interesse. |
| 8 | `Luce_Candela_S/N` | Point 2,5 cd, 2000 K, **senza ombre** | Piccoli **accenti** sul gradino del camino, ai lati del quadro. Senza ombre perché sono minuscole e vicine al muro. |
| 9 | `Libreria_Riempimento_SenzaOmbre` | Spot debole, 3200 K, **senza ombre** | **Riempimento**: il fuoco prende la libreria di taglio e i dorsi dei libri sarebbero illeggibili. Senza ombre non si nota e non si somma alle ombre delle altre luci. |
| – | `PostProcess_Stanza` | Post Process Volume, infinito | Esposizione automatica limitata (non "rincorre" le fiamme), bloom per fuoco e candele, vignetta per chiudere l'inquadratura, un filo di grana, bilanciamento del bianco appena caldo. |
| – | `Nebbia` | Exponential Height Fog **volumetrica** | Rende visibili i raggi della luna e l'alone del fuoco. |

Tutte le luci sono **Movable** (il progetto usa Lumen con `r.AllowStaticLighting=False`).

## Come è costruita

Lo script Python dell'editor [`Content/Python/gd14_stanza_luci.py`](../Content/Python/gd14_stanza_luci.py) fa quello che si farebbe a mano, in modo ripetibile. Il risultato è un livello normale, che poi si ritocca nell'editor.

- **Due fasi, come nel level design vero.** `costruisci(blockout=True)` crea `L_GD14_Baita_Blockout`: solo volumi del plugin **BlockOut Tools** (`Blockout_Box`, colorati per funzione) e i "proxy" dei mobili, per decidere misure e inquadrature. `costruisci()` crea `L_GD14_Baita`, la stanza finita (il vecchio `L_GD14_StanzaLuci` non viene toccato).
- **Geometria da zero.** Muri, pavimento, camino e mobili sono `Blockout_Box` con un materiale nostro (*Custom Material*); le falde e le travi inclinate sono cubi orientati. Il **camino ad arco** è fatto a corsi di mattoni che lasciano libera la bocca semicircolare, con 11 conci a raggiera sull'arco, la canna a gradoni e il focolare tondo (un cilindro con il cordolo). Il muro con finestra e porta è fatto "a pezzi" intorno ai buchi. Le travi a vista (colmo, puntoni ogni metro, due catene) seguono la pendenza del tetto.
- **Mattoni procedurali** (`M_AIV_Mattoni` in `aiv_materiali.py`): il disegno dei mattoni, sfalsati a file alterne, con la malta e un colore diverso per ogni mattone, è calcolato dalla posizione nel mondo, quindi i mattoni hanno sempre la stessa misura su ogni pezzo del camino. Larghezza, altezza, malta e colori sono parametri dell'istanza.
- **Carta da parati procedurale** (`M_AIV_CartaParati`): rombi su una griglia sfalsata, con la grana dell'intonaco di Scan Labs; passo, misura del motivo e colori sono parametri.
- **Materiali World Aligned** ([`aiv_materiali.py`](../Content/Python/aiv_materiali.py)): le texture di Scan Labs (legno, intonaco, quarzo per la pietra, tessuti, rattan, corteccia) sono proiettate dai tre assi del mondo, quindi non si stirano sui cubi scalati. Le braci e le fiamme hanno un materiale emissivo animato con il rumore (come la lava). Decal per la fuliggine del camino, la cenere e le orme bagnate.
- **Prop di Scan Labs** (poltrone, lampade, libri, quadri, frutta, bicchieri, scarpe, pianta, castagno e bosco fuori dalla finestra), piazzati per **ingombro** e non per pivot: lo script legge il bounding box della mesh e la appoggia a terra, sul tavolo o contro il muro; se è troppo grande per il posto la rimpicciolisce.
- **Giocabile**: c'è un PlayerStart e la GameMode del progetto, quindi con **Play** ci si cammina dentro con Bot2.

### Lanciarlo

1. Riavviare l'editor una volta dopo aver preso il branch (redirect di Scan Labs in `DefaultEngine.ini`, vedi la doc del GD13).
2. Tools → **Execute Python Script…** → `Content/Python/gd14_stanza_luci.py`, oppure nella Output Log (riga `Python`):
   `import gd14_stanza_luci; gd14_stanza_luci.costruisci()` (e `costruisci(blockout=True)` per il blockout).
3. Aprire **`Content/Maps/L_GD14_Baita`** (non il vecchio `L_GD14_StanzaLuci`). Rilanciare lo script **ricrea** il livello e mette quello di prima in `L_GD14_Baita_Backup`.
4. In fondo alla Output Log lo script scrive `[GD14] fatto: N oggetti`; se una parte si è fermata scrive `ATTENZIONE` con il nome della parte e, sopra, l'errore in rosso.

### Da guardare la prima volta (lo script non è stato eseguito in Unreal)

1. **Verso dei mobili.** Lo script non sa da che lato "guarda" una mesh di Scan Labs. Se le poltrone guardano il muro invece del fuoco, cambiare `FRONTE_SCANLABS` in cima allo script (0, 90, 180 o 270) e rilanciare, oppure ruotarle a mano.
2. **Esposizione.** Con Lumen e l'esposizione automatica l'interno può venire più chiaro o più scuro del previsto: regolare `auto_exposure_bias` nel `PostProcess_Stanza`, oppure `FUOCO_CD` / `LUNA_LUX` in cima allo script.
3. **Lampada a sospensione.** Se la mesh ha un cavo molto lungo lo script la rimpicciolisce per tenerla sopra il tavolo; se non convince, spostarla a mano (e la sua luce con lei).
4. Se nella Output Log compare `asset mancante`, quel percorso nel vostro progetto è diverso: correggerlo nella costante.

## Gli screenshot

Cinque CineCamera nella cartella `Camere`:

- `Cam_1_Insieme`: dall'angolo della cucina verso il camino, tutta la stanza con tavolo, tappeto e poltrone;
- `Cam_2_Camino`: il camino ad arco, le candele, le mensole piene di oggetti, le poltrone ai lati;
- `Cam_3_Controcampo_Finestra`: dal camino verso la finestra e la porta, il fuoco in primo piano e la luce di fuori sullo sfondo;
- `Cam_4_AngoloLettura`: la poltrona, la lampada da terra, il tavolino con il libro aperto;
- `Cam_5_DaiPiediDelLetto`: dall'angolo del letto verso il camino, il tappeto tondo in mezzo.

Per ognuna: tasto destro → **Pilot**, **G** per togliere le icone, poi nella Output Log `HighResShot 2`. Le immagini finiscono in `Saved/Screenshots/`.

## Testo da consegnare insieme agli screenshot

> Ho costruito la soffitta di un rifugio di montagna, calda e colorata, al tramonto. L'idea di luce è il contrasto tra il caldo del fuoco e la luce di fuori. La luce chiave è il fuoco del camino ad arco, che è anche il punto focale: le poltrone e il tappeto tondo sono rivolti verso di lui, e la sua luce tremola con un rumore di Perlin, così le ombre si muovono come con una fiamma vera; una seconda luce senza ombre, morbida, simula il calore che esce dalla bocca del camino. Dalla finestra entra il sole del tramonto, rosso e radente, che disegna il telaio sulle assi del pavimento e, con la nebbia volumetrica, i raggi nell'aria; la Sky Light decide il colore delle ombre. Dalla porta socchiusa entra un'altra lama di luce che porta lo sguardo verso l'uscita e racconta che qualcuno è appena rientrato (ci sono le scarpe bagnate e le orme). Le luci pratiche sono motivate: la lampada da terra illumina l'angolo lettura, ma la sua luce è sotto il paralume e la lampada non fa ombra; la lampada a sospensione crea un cerchio di luce sul tavolo da pranzo; due candele accendono il camino ai lati del quadro. Infine una spot debole e senza ombre rende leggibili i libri, che il fuoco prende di taglio.
