# -*- coding: utf-8 -*-
"""Come si costruisce ogni Blueprint: componenti, origine e logica.
Testo LaTeX. "Ninja" = Docs/Blueprint_Ninja (i grafi a nodi)."""

DESCRIZIONI = {
    "BP_Zona": dict(
        componenti=r"\nodo{Box Collision} \var{Trigger} (root; Collision Preset \emph{Trigger}), \nodo{Static Mesh} \var{Visual} (un \var{Cube}, senza collisione), \nodo{Text Render} \var{Scritta}.",
        origine=r"il \textbf{centro} della scatola. Nel \nodo{Construction Script}: \nodo{Set Box Extent} = \var{ZoneExtent}; \var{Visual} con scala $(\mathrm{ZoneExtent.X}/50;\ \mathrm{ZoneExtent.Y}/50;\ \mathrm{VisualThickness}/100)$ e Z relativa $-\mathrm{ZoneExtent.Z}+\mathrm{VisualThickness}/2$ (sta sul fondo); \nodo{Set Visibility} di \var{Visual} = \var{ShowVisual}; la scritta a Z $\mathrm{ZoneExtent.Z}+20$.",
        logica=r"filtra il giocatore e chiama gli eventi vuoti \var{Entra}/\var{Esce} che i figli riempiono (Ninja, cap.~10)."),
    "BP_Pickup": dict(
        componenti=r"\nodo{Sphere Collision} \var{Sfera} (root, raggio \var{PickupRadius}), \nodo{Static Mesh} \var{Mesh} (senza collisione), \nodo{Point Light} \var{Luce} (intensità \var{GlowIntensity}, colore \var{GlowColor}, raggio 200), \nodo{Text Render} \var{Scritta} a Z 90.",
        origine=r"il \textbf{centro} dell'oggetto (che gira e ondeggia intorno a quel punto).",
        logica=r"gira e ondeggia nel Tick; all'overlap chiama \var{Raccolto} e, se torna vero, suono e \nodo{Destroy Actor} (Ninja, cap.~11)."),
    "BP_Interruttore": dict(
        componenti=r"dipendono dal figlio (piastra o leva).",
        logica=r"\var{ImpostaStato}: chiama \var{Attiva}/\var{Disattiva} dell'interfaccia \var{BPI\_Attivabile} su ogni elemento di \var{Targets} e lancia il dispatcher \var{OnCambiato} (Ninja, cap.~9)."),
    "BP_Attivabile": dict(
        componenti=r"una \nodo{Scene} \var{Root}; il resto lo aggiunge il figlio. Implementa l'interfaccia \var{BPI\_Attivabile}.",
        logica=r"\var{Attiva} $\rightarrow$ timeline lunga \var{TransitionTime} in avanti, \var{Disattiva} $\rightarrow$ indietro; l'\emph{Update} chiama la funzione vuota \var{Applica(Alpha)} che i figli ridefiniscono. In BeginPlay: per ogni interruttore in \var{ListenTo}, \nodo{Bind Event to OnCambiato}. Se \var{StartActive}, parte già attivo (Ninja, cap.~9)."),
    "BP_ZonaCaduta": dict(
        logica=r"\var{Entra} $\rightarrow$ \var{GestisciCaduta} del componente \var{AC\_PostoSicuro}: perde una vita e torna all'ultimo posto sicuro; a zero vite, game over (Ninja, cap.~5). Nei livelli è invisibile (\var{ShowVisual} = no)."),
    "BP_ZonaMortale": dict(
        logica=r"\var{Entra} $\rightarrow$ \var{UccidiERiavvia}(\var{RestartDelay}) (Ninja, cap.~8)."),
    "BP_ZonaDanno": dict(
        logica=r"timer ripetuto ogni \var{HitInterval} che fa \nodo{Apply Damage} di \var{DamagePerHit} finché si è dentro (Ninja, cap.~8, la lava)."),
    "BP_ZonaModificatore": dict(
        logica=r"all'ingresso salva e moltiplica velocità, salto, gravità, attrito e frenata del Character Movement; all'uscita li ripristina dopo \var{DurationAfterExit} secondi (Ninja, cap.~10)."),
    "BP_Teletrasporto": dict(
        logica=r"\nodo{Teleport} dell'eroe in \nodo{Transform Location}(\var{Destination}) (la destinazione è \emph{relativa} alla zona: spuntate \emph{Show 3D Widget} sulla variabile per trascinarla nel viewport); se \var{StopVelocity}, velocità a zero (Ninja, cap.~10)."),
    "BP_Negozio": dict(
        logica=r"se le monete bastano, le toglie (\var{Cost}) e chiama \var{Attiva} su ogni elemento di \var{Targets}; con \var{OnlyOnce} funziona una volta sola. Altrimenti un messaggio ``Servono N monete'' (Ninja, cap.~10)."),
    "BP_Traguardo": dict(
        logica=r"\nodo{Do Once} $\rightarrow$ incassa le monete, registra il tempo, mostra \var{Message}, se \var{ResetDeathCount} azzera le morti, \nodo{Delay} \var{Delay} $\rightarrow$ \var{VaiAlLivello(NextLevel)} (Ninja, cap.~10). Nei livelli \_BP \var{NextLevel} è il livello \_BP successivo; dall'ultimo si torna a \var{L\_GD13\_Menu}."),
    "BP_TimerLivello": dict(
        componenti=r"\nodo{Scene} root, \nodo{Text Render} \var{Testo} (World Size 120, centrato).",
        origine=r"dove deve stare la scritta del tempo.",
        logica=r"timer ogni 0,1\,s che scala \var{TimeLimit}; sotto \var{WarningThreshold} il testo diventa rosso; a zero, \var{UccidiERiavvia} (Ninja, cap.~10). \var{ShowOnScreen} = mostra il tempo anche nell'HUD."),
    "BP_Moneta": dict(
        componenti=r"quelli di \var{BP\_Pickup}; la mesh è un \var{Cylinder} schiacciato (scala $0{,}45 \times 0{,}45 \times 0{,}08$) ruotato di 90\textdegree{} in Roll, così la si vede di taglio.",
        logica=r"\var{Raccolto}: \var{GI.Monete} += \var{Value}, ritorna vero (Ninja, cap.~11)."),
    "BP_PickupAbilita": dict(
        componenti=r"quelli di \var{BP\_Pickup}; mesh \var{Sphere} scala 0,4.",
        logica=r"\var{Raccolto}: \var{SbloccaAbilita(Ability)}, \var{MostraMessaggio(Message)}. Se \var{HideIfAlreadyUnlocked} e l'abilità c'è già, in BeginPlay si distrugge. Il cartello si scrive da solo nel Construction Script (Ninja, cap.~11)."),
    "BP_PickupSalute": dict(
        componenti=r"quelli di \var{BP\_Pickup}; mesh \var{Cylinder} scala $(0{,}6;\ 0{,}6;\ 0{,}9)$, materiale verde.",
        logica=r"\var{Raccolto}: se la vita non è piena, \nodo{Apply Damage} di $-$\var{HealAmount} all'eroe e ritorna vero; altrimenti falso e resta lì (Ninja, cap.~11)."),
    "BP_PickupBatteria": dict(
        componenti=r"quelli di \var{BP\_Pickup}; mesh \var{Cylinder} scala $(0{,}25;\ 0{,}25;\ 0{,}45)$.",
        logica=r"\var{Raccolto}: \var{Batteria} della torcia += \var{BatterySeconds} (con \nodo{Min} sul massimo) (Ninja, cap.~4 e 11)."),
    "BP_PiattaformaMobile": dict(
        componenti=r"\nodo{Static Mesh} \var{Mesh} (root, \var{Cube} con scala \var{PlatformSize}/100, collisione \emph{BlockAll}), \nodo{Audio} \var{Suono} (\var{MoveSound}).",
        origine=r"il \textbf{centro} della piattaforma (nelle tabelle la Z è già il centro: piano $-$ metà spessore).",
        logica=r"timeline di \var{TravelTime} secondi fra la posizione iniziale e iniziale + \var{EndOffset}, pausa \var{PauseTime} agli estremi, ritardo iniziale \var{StartDelay}; \nodo{Set Pitch Multiplier} del suono a \var{PitchForward} all'andata e \var{PitchBackward} al ritorno (Ninja, cap.~8)."),
    "BP_PiattaformaCrolla": dict(
        componenti=r"\nodo{Static Mesh} \var{Mesh} (root, \var{Cube} scala \var{PlatformSize}/100), \nodo{Box Collision} \var{Sopra} (extent $(\mathrm{X}/2-5;\ \mathrm{Y}/2-5;\ 20)$, a Z $\mathrm{PlatformSize.Z}/2+20$: sente chi ci sale).",
        origine=r"il \textbf{centro} della piattaforma.",
        logica=r"qualcuno sale $\rightarrow$ materiale \var{WarningMaterial} e tremolio per \var{ShakeTime} $\rightarrow$ suono, nascosta e senza collisione $\rightarrow$ dopo \var{RespawnTime} torna (Ninja, cap.~8)."),
    "BP_Trampolino": dict(
        componenti=r"\var{Base} (\var{Cylinder} scala $(1{,}4;\ 1{,}4;\ 0{,}2)$), \var{Pad} (\var{Cylinder} scala $(0{,}85;\ 0{,}85;\ 0{,}4)$ a Z 60, materiale \var{PadMaterial}), \nodo{Box Collision} \var{Trigger} extent $(40;\ 40;\ 150)$ a Z 250. \emph{Attenzione}: il trigger sta sopra il pad, così il lancio scatta mentre si passa sopra.",
        origine=r"\textbf{a terra}, al centro della base.",
        logica=r"overlap $\rightarrow$ \nodo{Launch Character} con \nodo{Rotate Vector}(\var{LaunchVelocity}, rotazione dell'actor), \emph{XY Override} = \var{OverrideHorizontal}, \emph{Z Override} sì; il pad si schiaccia per un attimo (Ninja, cap.~8)."),
    "BP_BarraRotante": dict(
        componenti=r"\var{Base} (\var{Cylinder} scala $(0{,}4;\ 0{,}4;\ \mathrm{BarHeight}/100)$ a Z $\mathrm{BarHeight}/2$), \var{Perno} (Scene), sotto il perno \var{Barra} (\var{Cube} scala \var{BarSize}/100 a Z \var{BarHeight}, collisione \emph{OverlapAllDynamic}: \textbf{non} blocca).",
        origine=r"\textbf{a terra}, al centro.",
        logica=r"\nodo{Rotating Movement} sul perno a \var{DegreesPerSecond}; all'overlap della barra: se \var{KillOnTouch} $\rightarrow$ \var{UccidiERiavvia}(0,3), altrimenti \nodo{Launch Character} lontano dal centro con forza \var{PushStrength} (Ninja, cap.~8)."),
    "BP_Torretta": dict(
        componenti=r"\var{Base} (\var{Cylinder} scala $(0{,}8;\ 0{,}8;\ 1)$ a Z 50), \var{Testa} (Scene a Z 130) con \var{TestaMesh} (\var{Sphere} 0,75), \var{Canna} (\var{Cylinder} $0{,}22 \times 0{,}22 \times 1$ ruotato in avanti) e \var{Bocca} (Scene a X 110); \nodo{Sphere Collision} \var{Rilevamento} raggio \var{Range}.",
        origine=r"\textbf{a terra} sopra la colonna rossa; yaw = verso cui guarda a riposo.",
        logica=r"Ninja, cap.~8: \nodo{Find Look at Rotation} + \nodo{RInterp To} (\var{TurnSpeed}), pitch fra \var{MinPitch} e \var{MaxPitch}, mira all'altezza del bersaglio + \var{AimHeightOffset}; spara ogni \var{FireInterval} se l'angolo è entro \var{FireConeDegrees} e la vista è libera. Con \var{LeadTarget} mira dove il giocatore \emph{sarà}: posizione + velocità $\times$ (distanza / \var{ProjectileSpeed}). Ai proiettili passa \var{ProjectileSpeed}, \var{ProjectileDamage} e \var{RestartLevelOnHit}."),
    "BP_PiastraPressione": dict(
        componenti=r"\var{Cornice} (\var{Cube} scala $(1{,}6;\ 1{,}6;\ 0{,}08)$ a Z 4), \var{Piastra} (\var{Cube} $(1{,}3;\ 1{,}3;\ 0{,}1)$ a Z 10), \nodo{Box Collision} extent $(65;\ 65;\ 40)$ a Z 40.",
        origine=r"\textbf{a terra}, al centro.",
        logica=r"un Character entra $\rightarrow$ \var{ImpostaStato}(vero), piastra giù di \var{PressDepth} in \var{PressTime}, materiale \var{OnMaterial}; se \var{StayPressed} è no, quando esce torna su (Ninja, cap.~9)."),
    "BP_Leva": dict(
        componenti=r"\var{Base} (\var{Cube} $(0{,}5;\ 0{,}7;\ 0{,}9)$ a Z 45), \var{Perno} (Scene a Z 90) con \var{Manico} (\var{Cylinder} $0{,}08 \times 0{,}08 \times 0{,}7$ a Z 35), \nodo{Text Render} a Z 190. Collisione della base sul canale \emph{Interact} (o \emph{Visibility}). Implementa \var{BPI\_Interagibile}.",
        origine=r"\textbf{a terra}.",
        logica=r"\var{Interagisci}: se \var{Toggle} alterna acceso/spento, altrimenti accende soltanto; il manico ruota da $-$40\textdegree{} a +40\textdegree{}; \var{GetPrompt} restituisce \var{Prompt} (Ninja, cap.~9)."),
    "BP_Porta": dict(
        componenti=r"due \nodo{Scene} cerniere \var{CernieraSx} (Y $= -\mathrm{DoorWidth}/2$) e \var{CernieraDx} (Y $= +\mathrm{DoorWidth}/2$), con figlie le ante (\var{Cube} scala $(\mathrm{DoorThickness};\ \mathrm{DoorWidth}/2;\ \mathrm{DoorHeight})/100$, a Y $\pm\mathrm{DoorWidth}/4$ verso il centro e Z $\mathrm{DoorHeight}/2$).",
        origine=r"\textbf{a terra}, al centro del vano.",
        logica=r"\var{Applica(Alpha)}: se \var{Sliding}, le cerniere scorrono di $\mathrm{DoorWidth}/2 \cdot$ Alpha verso l'esterno; altrimenti ruotano di $\pm\mathrm{OpenAngle} \cdot$ Alpha (Ninja, cap.~9)."),
    "BP_LuceAttivabile": dict(
        componenti=r"\nodo{Point Light} (40 cd, Movable) e, se \var{ShowBulb}, una \var{Sphere} scala 0,2 con un materiale emissivo.",
        origine=r"dove sta la lampadina.",
        logica=r"\var{Applica(Alpha)}: \nodo{Set Intensity} = 40 $\cdot$ Alpha (parte spenta)."),
    "BP_Ascensore": dict(
        componenti=r"\nodo{Static Mesh} \var{Mesh} (\var{Cube} scala \var{PlatformSize}/100) figlia della root.",
        origine=r"il \textbf{centro} della piattaforma nella posizione di partenza.",
        logica=r"\var{Applica(Alpha)}: \nodo{Set Relative Location} della mesh = \var{EndOffset} $\cdot$ Alpha."),
    "BP_CresceConDistanza": dict(
        componenti=r"\nodo{Static Mesh} \var{Mesh} (\var{ShapeMesh}, default \var{Cube}), \nodo{Text Render} sopra.",
        origine=r"il \textbf{centro} della forma.",
        logica=r"Tick: distanza dal giocatore $\rightarrow$ \nodo{Map Range Clamped} (\var{NearDistance}..\var{FarDistance} $\rightarrow$ \var{MinScale}..\var{MaxScale}) $\rightarrow$ \nodo{FInterp To} (\var{InterpSpeed}) $\rightarrow$ scala = \var{BaseSize}/100 $\cdot$ fattore (se \var{ScaleHeight} è no, la Z resta \var{BaseSize.Z}/100) (Ninja, cap.~7). Nel livello 1 cresce allontanandosi; nel livello 2 le piattaforme \emph{rimpiccioliscono} avvicinandosi: è lo stesso Blueprint con \var{MinScale} $<$ \var{MaxScale}."),
    "BP_CambiaModello": dict(
        componenti=r"\nodo{Static Mesh} \var{Mesh} (scala \var{Size}/100; con \var{Collide} collisione \emph{BlockAll}, altrimenti nessuna), \nodo{Text Render} sopra.",
        origine=r"il \textbf{centro} della forma.",
        logica=r"\var{Models} = [Cube, Cylinder, Sphere, Cone] (da \texttt{/Engine/BasicShapes}); timer ripetuto ogni \var{Interval}: indice successivo (modulo), \nodo{Set Static Mesh} e \nodo{Set Material} con lo stesso indice in \var{Materials} (Ninja, cap.~7)."),
    "BP_PresenzaCasuale": dict(
        componenti=r"\nodo{Static Mesh} \var{Mesh} (\var{ShapeMesh}, default \var{Cube}; scala \var{Size}/100; \emph{BlockAll}), \nodo{Text Render}.",
        origine=r"il \textbf{centro} dell'oggetto (per il ponte: centro del ponte, Z = piano $-$ 15).",
        logica=r"BeginPlay: \nodo{Random Bool with Weight}(\var{SpawnChance}); se esce no, nascosto e senza collisione. Se \var{Announce}, dopo 0,5\,s \var{PresentMessage} o \var{AbsentMessage} (Ninja, cap.~7)."),
    "BP_Mulino": dict(
        componenti=r"\var{Torre} (\var{Cylinder} scala $(1{,}6;\ 1{,}6;\ \mathrm{TowerHeight}/100)$ a Z $\mathrm{TowerHeight}/2$), \var{Testa} (Scene a Z \var{TowerHeight}), \var{Mozzo} (Scene a X 110 della testa) con quattro pale (\var{Cube} scala $(0{,}15;\ 0{,}7;\ \mathrm{BladeLength}/100)$, ruotate di 0, 90, 180, 270\textdegree{} in Roll e spostate di $\mathrm{BladeLength}/2+40$ lungo il proprio asse).",
        origine=r"\textbf{a terra}, al centro della torre.",
        logica=r"pale: \nodo{Add Local Rotation} Roll = \var{BladeSpeed} $\cdot$ dt; se \var{LookAtPlayer}, la testa ruota verso il giocatore con yaw limitato a $\pm$\var{YawLimit} rispetto all'actor (\nodo{Find Look at Rotation}, \nodo{Clamp Angle}) (Ninja, cap.~13)."),
    "BP_CasseFisiche": dict(
        componenti=r"una \nodo{Scene} root; le casse sono actor \var{BP\_CassaFisica} (un \var{Cube} con \emph{Simulate Physics}).",
        origine=r"sopra il centro della griglia di casse (i raggi partono da qui e scendono).",
        logica=r"BeginPlay: doppio \nodo{For Loop} \var{Rows} $\times$ \var{Columns}; per ogni cella un \nodo{Line Trace} verso il basso trova il terreno, poi \var{CubeLayers} casse impilate di lato \var{CubeSize}, passo \var{Spacing}, massa \var{CubeMass}, materiale alternato da \var{Materials} (la lezione dei due cicli for)."),
    "BP_Staccionata": dict(
        componenti=r"\nodo{Spline} \var{Percorso}, \nodo{Instanced Static Mesh} \var{Pali} (\var{PostMesh}, default \var{Cube} scala \var{PostSize}).",
        origine=r"il primo punto della spline.",
        logica=r"Construction Script: un palo ogni \var{Spacing} cm lungo la spline, \var{Rails} traverse fra i pali con spessore \var{RailThickness} (Ninja, cap.~13). I punti della spline sono nelle tabelle (coordinate \emph{locali})."),
    "BP_Spargi": dict(
        componenti=r"\nodo{Box Collision} \var{Area} (extent \var{AreaExtent}, solo per vederla nell'editor), \nodo{Instanced Static Mesh} \var{Istanze} (\var{Mesh}).",
        origine=r"il \textbf{centro} dell'area (Z: sopra il terreno, i raggi scendono).",
        logica=r"Construction Script con un \nodo{Random Stream} di seme \var{Seed}: \var{Count} punti a caso nell'area, scartando quelli a meno di \var{MinDistance} da un altro, \nodo{Line Trace} verso il basso (se \var{AlignToGround}), scala fra \var{MinScale} e \var{MaxScale}, yaw a caso, \nodo{Add Instance} (Ninja, cap.~13). Negli alberi: mesh \var{SM\_Chestnut\_4}."),
    "BP_LuceTremolante": dict(
        componenti=r"\nodo{Point Light} (root o figlia; Movable, temperatura \var{Temperature}, raggio \var{AttenuationRadius}, \emph{Source Radius} \var{SourceRadius}).",
        origine=r"dove sta la fiamma (nei bracieri: 150\,cm sopra il piano).",
        logica=r"Tick: intensità = \var{BaseIntensity} $\cdot$ (1 + rumore $\cdot$ \var{FlickerAmount}) con \nodo{FInterp To} verso un bersaglio casuale (\var{FlickerSpeed}); piccolo spostamento casuale di \var{PositionJitter} cm; sugli actor in \var{GlowActors} imposta il parametro \var{GlowParameter} del materiale emissivo in proporzione (Ninja, cap.~13). Nel braciere \var{GlowActors} = la sfera delle braci."),
}
