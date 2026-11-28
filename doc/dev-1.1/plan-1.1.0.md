# Piano di sviluppo per fasi: photo-snap 1.1.0

**Nome prodotto:** `photo-snap`  
**Versione attuale:** `1.0.0`  
**Versione target:** `1.1.0`

## Stato iniziale e obiettivo

La versione attuale (`1.0.0`) è un'applicazione Qt Widgets per catalogare, sfogliare e esportare una libreria fotografica. Le parti riutilizzabili sono:

- apertura di immagini da file e trascinamento;
- catalogo locale, miniature, navigazione per data e ricerca;
- visualizzazione con `PhotoView`, basata su `QGraphicsView`;
- fit, zoom, pan, dimensione reale e visualizzazione HiDPI;
- rotazione del solo risultato visualizzato di +90°;
- copia del risultato o dei file selezionati negli appunti;
- modifica dei metadati EXIF;
- build CMake multipiattaforma.

Il nuovo editor partirà da questo codice, senza riscriverlo. A ogni fase verrà consegnata una versione compilabile e utilizzabile. Non si passa alla fase successiva finché il gate della fase corrente non è superato.

Il risultato finale sarà un editor multipiattaforma con elaborazione non distruttiva, anteprima rapida, esportazione ad alta risoluzione e undo/redo.

## Vincoli invariati

- C++17 o successivo, Qt6 Widgets e CMake.
- Windows, Linux e macOS.
- Nessuna dipendenza Qt Commercial, OpenCV, Skia, OpenGL o Qt Quick obbligatoria nella prima implementazione.
- L'immagine caricata resta invariata in memoria e sul disco.
- I parametri di modifica sono separati dai pixel dell'immagine.
- Preview ed export applicano la stessa sequenza di trasformazioni.
- La UI non contiene algoritmi di elaborazione.
- Il core usa `QImage`; `QPixmap` viene creato solo per la visualizzazione.
- Il backend GPU resta un'opportunità futura, non una parte iniziale.

## Ordine unico delle operazioni

Tutte le versioni successive alla fase 2 useranno questa pipeline:

1. rotazione;
2. crop;
3. luminosità;
4. contrasto;
5. saturazione;
6. filtri;
7. esportazione.

`PreviewRenderer` lavora su una copia ridimensionata quando necessario. `ImageExporter` parte sempre da `ImageDocument::originalImage()` alla risoluzione disponibile.

## Architettura da introdurre progressivamente

```text
src/
├── app/
│   └── MainWindow.cpp
├── ui/
│   ├── PhotoView.cpp
│   ├── CropOverlay.cpp
│   └── AdjustmentPanel.cpp
├── core/
│   ├── ImageDocument.cpp
│   ├── ImageAdjustments.h
│   ├── ImageProcessor.cpp
│   ├── ImageTransform.cpp
│   ├── ImageFilters.cpp
│   ├── PreviewRenderer.cpp
│   └── ImageExporter.cpp
└── commands/
    ├── RotateCommand.cpp
    ├── CropCommand.cpp
    └── AdjustmentCommand.cpp
```

La migrazione procede per fasi: i file esistenti restano attivi finché i nuovi componenti non sono integrati e verificati. Non aggiungere directory o classi che non sono necessarie alla fase corrente.

## Fase 0: baseline verificata

### Obiettivo

Congelare il comportamento attuale come punto di partenza e preparare un harness di test minimo.

### Attività

1. Verificare apertura, visualizzazione, zoom, pan, libreria e copia.
2. Mappare i punti di integrazione di `MainWindow`, `PhotoView` e `openImage()`.
3. Aggiungere una configurazione CMake per test Qt senza introdurre framework esterni.
4. Aggiungere test base per lettura immagini e trasformazioni 90° già presenti.
5. Creare una procedura ripetibile di build Debug e Release.

### Versione utilizzabile

L'applicazione resta la versione corrente della libreria fotografica, ma la build viene estesa a test eseguibili dal workspace.

### Gate

- build pulita su almeno una configurazione di sviluppo;
- test baseline eseguiti con esito positivo;
- nessuna regressione nella libreria, nel caricamento o nella copia.

## Fase 1: modello documento e visualizzazione non distruttiva

### Obiettivo

Separare immagine originale, stato delle modifiche e visualizzazione.

### Attività

1. Creare `ImageAdjustments` con valori neutri:
   - rotazione `0.0`;
   - crop normalizzato `[0,1]`;
   - luminosità `0.0`;
   - contrasto `1.0`;
   - saturazione `1.0`;
   - filtri disattivati.
2. Creare `ImageDocument`, responsabile di:
   - `QImage originalImage`;
   - parametri correnti;
   - percorso e stato modificato;
   - API esplicite per cambiare i parametri;
   - segnali di modifica.
3. Sostituire gradualmente `m_originalPixmap` con un riferimento a `QImage` e mantenere `m_originalPixmap` solo come cache di compatibilità se necessario.
4. Conservare l'immagine già orientata da `QImageReader::setAutoTransform(true)`.
5. Centralizzare il rendering nel thread principale tramite un primo `PreviewRenderer` neutro.
6. Conservare `PhotoView` e migliorarlo solo dove necessario: fit, zoom, pan, resize, HiDPI e aggiornamento coerente dall'immagine preview.

### Test

- caricamento di immagini valide e non valide;
- l'immagine originale non cambia dopo richieste di aggiornamento;
- parametri neutri riproducono l'immagine originale;
- resize e fit non degradano l'immagine sorgente;
- cambio foto resetta i parametri del documento.

### Versione utilizzabile

Versione della libreria con il nuovo documento interno. Apertura, libreria, zoom, pan, fit e copia continuano a funzionare; il pannello dell'editor può ancora essere assente.

### Gate

- nessun accesso diretto ai pixel dell'immagine originale fuori dal core;
- nessuna perdita progressiva di qualità durante zoom e resize;
- build e test passanti.

## Fase 2: rotazione e crop interattivo

### Obiettivo

Rendere `ImageTransform` e il crop disponibili in anteprima senza modificare l'immagine originale.

### Attività

1. Implementare rotazione libera da -180° a +180° con precisione 0,1°.
2. Aggiungere rotazioni rapide +90°, -90° e reset.
3. Sostituire la trasformazione cumulativa attuale con un parametro numerico applicato all'immagine originale.
4. Aggiungere il controllo dello slider senza pixel permanenti durante il trascinamento.
5. Implementare `CropOverlay` con selezione, spostamento e otto maniglie.
6. Supportare rapporti Originale, 1:1, 4:3, 3:2, 16:9 e Libero.
7. Memorizzare il crop come `QRectF` normalizzato rispetto all'immagine originale.
8. Confermare e annullare il crop separatamente dal rimanente stato documento.
9. Definire e testare la conversione delle coordinate immagine/widget con zoom, pan e rotazione.

### Test

- rotazione 90°, -90° e 180°;
- precisione 0,1°;
- crop centrale e rapporti predefiniti;
- crop invariato al resize del widget;
- rotazione seguita da crop produce lo stesso risultato in preview;
- l'immagine originale resta invariata.

### Versione utilizzabile

La libreria può aprire una foto e usare rotazione libera/rapida e crop. La copia può esportare solo l'anteprima applicata delle trasformazioni geometriche o può continuare temporaneamente a usare l'immagine originale, ma questa limitazione deve essere indicata nello stato della fase.

### Gate

- rotazione e crop fluidi;
- nessuna modifica distruttiva;
- coordinate corrette con zoom e pan;
- test geometrici passanti.

## Fase 3: regolazioni cromatiche e sistema di anteprima

### Obiettivo

Aggiungere luminosità, contrasto e saturazione con aggiornamenti rapidi e senza bloccare la UI.

### Attività

1. Implementare i tre filtri base in `ImageProcessor`/`ImageFilters`.
2. Aggiungere pannello con slider, campo numerico e reset individuale.
3. Aggiornare i parametri nel documento senza modificare i pixel.
4. Implementare `PreviewRenderer` con:
   - ridimensionamento proporzionale;
   - debounce con `QTimer`, circa 30 ms;
   - cache dell'ultimo risultato;
   - identificatore progressivo delle richieste;
   - scarto dei risultati obsoleti.
5. Preparare il worker asincrono con `QThreadPool`/`QRunnable` o `QtConcurrent` solo per operazioni misurabilmente costose.
6. Convertire il risultato worker in `QPixmap` esclusivamente nel thread principale.

### Test

- valori neutri non cambiano i pixel;
- luminosità, contrasto e saturazione sono riproducibili;
- più regolazioni non alterano l'immagine originale;
- gli aggiornamenti obsoleti non raggiungono `PhotoView`;
- interfaccia reattiva durante trascinamento e resize.

### Versione utilizzabile

Editor minimo con rotazione, crop, luminosità, contrasto e saturazione. Tutte le funzioni della libreria continuano ad essere disponibili.

### Gate

- preview senza blocchi percepibili;
- nessuna race sui dati;
- valore finale coerente dopo il rilascio dello slider;
- test cromatici e timing passanti.

## Fase 4: filtri fotografici

### Obiettivo

Aggiungere filtri composibili e controllabili.

### Attività

1. Implementare i filtri iniziali:
   - bianco e nero;
   - seppia;
   - temperatura colore;
   - nitidezza.
2. Introdurre parametri indipendenti e reset singolo.
3. Aggiungere filtro rapido o preset già previsto nell'interfaccia, se serve un piccolo controllo per attivazione.
4. Ottimizzare l'accesso ai pixel con `bits()`/`scanLine()`.
5. Gestire correttamente alpha, formati pixel, alpha premoltiplicato e clamping numerico.
6. Integrare cache e invalidazione con la sequenza unica Preview/Export.

### Test

- ogni filtro a valori neutri e estremi;
- combinazione di più filtri;
- immagine originale non modificata;
- alpha preservata;
- preview uguale alla sequenza usata dall'export a parità di risoluzione.

### Versione utilizzabile

Editor completo per la modifica visuale: rotazione, crop, regolazioni, filtri, libreria e copia della preview.

### Gate

- tutti i filtri deterministici;
- nessuna regressione cromatica sui formati supportati;
- reset individuali e globale corretti;
- build e test passanti.

## Fase 5: Undo e Redo

### Obiettivo

Rendere tutte le modifiche reversibili senza copiare immagini nel comando.

### Attività

1. Integrare `QUndoStack`.
2. Implementare comandi per:
   - rotazione;
   - crop;
   - luminosità;
   - contrasto;
   - saturazione;
   - attivazione/disattivazione dei filtri.
3. Ogni comando conserva solo parametri precedenti e successivi.
4. Raggruppare un trascinamento dello slider in una sola operazione Undo.
5. Separare l'inizio/fine del trascinamento dalla pressione rapida.
6. Evidenziare lo stato modificato del documento.
7. Collegare Reset, menu Edit e scorciatoie a Undo/Redo.

### Test

- undo/redo per ogni parametro;
- undo dopo reset globale;
- redo scartato dopo una nuova modifica;
- un trascinamento produce un solo comando;
- apertura di una nuova foto resetta lo stack in modo coerente;
- stato modificato corretto.

### Versione utilizzabile

Editor fotografico interattivo con undo/redo completo e stato documento.

### Gate

- nessuna corrispondenza mancata tra stack e parametri;
- nessuna copia completa dell'immagine nei comandi;
- tutte le operazioni supportano Undo/Redo.

## Fase 6: esportazione ad alta risoluzione

### Obiettivo

Produrre il file definitivo partendo dall'immagine originale.

### Attività

1. Implementare `ImageExporter`.
2. Applicare sempre rotazione, crop, regolazioni e filtri nell'ordine unico.
3. Supportare JPEG, PNG e TIFF compatibilmente con i plugin disponibili.
4. Mostrare un dialogo con formato, destinazione e qualità JPEG.
5. Usare `QImageWriter` e gestire errori di scrittura.
6. Impedire sovrascritture accidentali dell'originale.
7. Conservare i metadati EXIF disponibili senza alterare il contenuto dell'originale.
8. Gestire orientamento EXIF e spazio colore con test specifici.
9. Se possibile, offrire una stima della dimensione oppure un indicatore di avanzamento per immagini grandi.

### Test

- export JPEG, PNG e TIFF;
- confronto dimensioni originali dove il formato lo consente;
- rotazione/crop/regolazioni combinate;
- nessuna modifica dell'originale;
- rifiuto di path invalido e scrittura fallita;
- nessuna sovrascrittura accidentale;
- anteprima ed export coerenti salvo differenze di risoluzione/compressione.

### Versione utilizzabile

Editor completo e distribuibile: modifica, undo/redo, libreria, anteprima e export ad alta risoluzione.

### Gate

- export riuscito sui formati richiesti;
- risultato deterministico e ripetibile;
- gestione esplicita degli errori;
- build Release dei pacchetti funzionante.

## Fase 7: stabilizzazione e ottimizzazione

### Obiettivo

Misurare, correggere e preparare la prima release dell'editor.

### Attività

1. Eseguire test prestazionali su:
   - 1920 × 1080;
   - 4000 × 3000;
   - 6000 × 4000.
2. Misurare preview, filtri, memoria, export e reattività.
3. Ottimizzare solo i colli di bottiglia misurati:
   - cache intelligente;
   - elaborazione a blocchi;
   - preview a più risoluzioni;
   - SIMD;
   - backend alternativo.
4. Verificare memoria, thread e risultati obsoleti sotto carico.
5. Aggiornare build, README e testi utente.
6. Non introdurre GPU finché un benchmark non identifica chiaramente il collo di bottiglia.

### Test

- suite completa delle fasi precedenti;
- ripetibilità su Windows, Linux e macOS;
- immagini con alpha, orientamento EXIF e formati pixel diversi;
- memoria sotto caricamento/esportazione;
- nessuna regressione della libreria esistente.

### Versione utilizzabile

Release dell'editor fotografico con libreria esistente integrata.

### Gate

- test funzionali e di integrità passanti;
- benchmark documentati;
- pacchetti Windows/Linux/macOS verificati;
- nessun errore noto critico sulla modifica o sull'export.

## Test trasversali

Ogni fase deve verificare:

- integrità dell'immagine originale;
- dimensioni, crop e parametri riproducibili;
- separazione preview/export;
- caricamento di immagini in formato supportato;
- comportamento dell'interfaccia durante resize e zoom;
- assenza di crash o risultati obsoleti;
- compilazione CMake della configurazione di rilascio.

## Regole operative

Per ogni fase:

1. analizzare i file esistenti interessati;
2. presentare un piano delle modifiche limitato alla fase;
3. implementare solo il comportamento della fase;
4. compilare Debug e, almeno alla fine della fase, testare la build Release;
5. eseguire i test della fase e quelli già superati;
6. correggere regressioni prima di aggiungere nuove funzionalità;
7. documentare decisioni architetturali rilevanti;
8. riassumere file modificati, test e risultato;
9. attendere approvazione prima di iniziare la fase successiva.
