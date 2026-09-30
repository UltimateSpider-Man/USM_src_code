# Ultimate Spider-Man PC — Online M1

**Prima implementazione sorgente Host/Join. Non è ancora un multiplayer completo né una build Windows verificata.**

Questa versione modifica `USM_multiplayer_gamepad_source.zip`, mantenendo la precedente correzione BOM/`Float`. Sostituisce l'ingresso nella vecchia arena con una sessione IP integrata nel percorso di gioco nativo. Il nome **M1** identifica il primo traguardo: connessione, identità, presenza e visualizzazione dei giocatori. Non indica che il mondo di gioco sia già sincronizzato.

## Cosa contiene realmente

Il nuovo menu offre nickname, IP host, porta, numero di posti, Crea sessione, Unisciti, Continua, Disconnetti e Indietro. La sessione usa TCP per identità, elenco partecipanti e controllo della connessione, e UDP per le pose. Supporta da 2 a 8 posti, con 4 come impostazione iniziale. Otto è un limite implementato e testato nel trasporto, non una garanzia di prestazioni nel motore.

Ogni computer mantiene il proprio eroe nativo, la propria telecamera e un solo schermo. La mod non crea un eroe locale di indice 1, non applica fisica da arena e non trasmette video. Acquisisce posizione, orientamento completo, velocità e clip di animazione di base dell'eroe; gli altri partecipanti diventano attori remoti non collidenti nel mondo locale. Le pose remote vengono interpolate, senza aspettare l'input dell'altro giocatore per avanzare il gioco locale.

Il sorgente include indicatori nella minimappa nativa e nomi proiettati sopra i personaggi. I colori appartengono alla sessione: host blu, altri posti con colori distinti. Il colore dell'host non cambia sul PC del client. I blip restano indipendenti dal raggio di caricamento dei modelli. La resa reale di attori, animazioni, nomi e mappa **deve ancora essere provata nel gioco**.

### Non ancora implementato

NPC, traffico, nemici, salute/danni condivisi, PvP, missioni, oggetti interattivi e stato della campagna non vengono sincronizzati. L'host assegna identità e distribuisce pose, ma **non simula autorevolmente la fisica nativa degli altri giocatori**. Questa versione non include previsione/riconciliazione della fisica, rollback o protezioni anti-cheat. Non sincronizza i vincoli delle ragnatele, tutti gli strati di animazione, effetti, prese e interazioni fra personaggi.

Quindi due personaggi presenti nella stessa città non equivalgono ancora a una partita cooperativa con combattimento condiviso. Non usare questa versione per valutare la correttezza di un boss o di una missione in co-op. Durante una missione storia riconosciuta dal motore, la replica viene sospesa e riprende in città libera.

## Applicazione e compilazione

Per evitare conflitti, usare il sorgente completo di questa versione in una cartella separata. In alternativa, estrarre la patch **nella radice dell'ultimo progetto con supporto gamepad**, dove si trova `CMakeLists.txt`. Non estrarla in `build/` o direttamente nella cartella di gioco. Conservare una copia delle proprie modifiche e dei salvataggi.

La patch non sovrascrive `multiplayer.ini`: le configurazioni dei gamepad e le risorse personaggio già personalizzate rimangono. La vecchia sezione `[Multiplayer]` dell’arena non viene usata dal nuovo percorso; `[Gamepads]` resta valida. Il file `multiplayer_online_defaults.ini` è un modello da consultare/integrare, non il nome letto dal gioco.

Dalla radice, usando il proprio ambiente **MinGW a 32 bit**:

```sh
cmake -S . -B build-online -DULTIMATE_RELEASE_XBPACK_MODE=OFF -DUSM_AUTO_DEPLOY=OFF
cmake --build build-online --target USM ultimate_release -j2
```

È necessaria una nuova configurazione CMake per raccogliere i nuovi `.cpp`. I target retail PC collegano `ws2_32`, `bcrypt` e `iphlpapi`; non si aggiungono SDL o servizi esterni. I target XBPACK/prerelease mantengono l'integrazione online disattivata. Nessun file viene distribuito automaticamente nell'installazione del gioco.

**Non è incluso un nuovo EXE/DLL compilato.** Il tentativo di configurazione nativa effettuato qui termina perché mancano `i686-w64-mingw32-gcc/g++`. Eventuali binari o array C già presenti nel progetto sono materiale preesistente, non risultati di questa compilazione.

## Flusso Host/Join previsto

I passaggi seguenti descrivono il funzionamento scritto nel sorgente, non una prova Radmin già superata.

1. Avviare la stessa versione della mod sui PC e rendere raggiungibile l'IPv4 dell'host. Con Radmin VPN, entrambi i partecipanti devono essere nella stessa rete virtuale. Autorizzare il gioco nel firewall per **TCP e UDP sulla porta scelta**; non disabilitare tutto il firewall. La mod non configura Radmin, router o firewall.
2. Dal menu principale aprire **MULTIPLAYER MODE**, oppure premere **M** o il tasto PS. Scegliere nickname e porta. Il pannello elenca gli IPv4 locali: privilegia un'interfaccia che si chiama Radmin e consente di scorrerli con R1/RB. Non deduce l'IP VPN dal suo prefisso numerico.
3. L'host seleziona **CREA SESSIONE** e comunica il proprio IP Radmin/LAN e la porta, normalmente `7777`. Il client seleziona **UNISCITI ALLA SESSIONE** dopo aver inserito questi dati nel menu. `127.0.0.1` serve solo per collegamenti sullo stesso PC.
4. Il gioco richiama il normale percorso **Continua/caricamento**. La sessione resta aperta durante il caricamento. Quando non è disponibile un salvataggio continuabile, il pannello lo segnala: chiuderlo e caricare normalmente un salvataggio, senza creare un secondo giocatore locale.
5. Entrare nella stessa città in modalità libera sui due PC. La mod non copia il salvataggio dell'host, non assegna una posizione comune di spawn e non teletrasporta automaticamente chi entra. I giocatori possono trovarsi tramite la mappa quando identità del livello e risorse coincidono.

Il nickname, l'ultimo IP, la porta e i posti scelti nel menu vengono salvati in `[Online]` dentro `multiplayer.ini`, accanto all'eseguibile. Il nome ammette 1–24 caratteri ASCII: lettere, numeri, spazi interni, `_`, `-`, `.`. Scollegarsi prima di modificarlo durante una sessione. Le normali funzioni di salvataggio automatico del gioco **non** vengono disabilitate: usare copie dei salvataggi per il collaudo.

`ContentTag` è un confronto manuale di configurazione: non certifica che tutti gli asset/mod siano identici. Usare gli stessi dati e la stessa versione. Il protocollo è incompatibile con la precedente arena LAN.

## Controlli

| Funzione | DualShock 4 / DualSense | Xbox / generico con profilo equivalente |
|---|---|---|
| Menu online dal menu principale | PS; oppure selezionare la voce e Cross | Selezionare la voce e A, oppure F6 |
| Selezione / conferma / indietro | Stick o D-pad / Cross / Circle | Stick o D-pad / A / B |
| Movimento nativo | Stick sinistro | Stick sinistro |
| Telecamera nativa | Stick destro | Stick destro |
| Salto / pareti | Cross / Circle | A / B |
| Pugno / calcio | Square / Triangle | X / Y |
| Azioni native aggiuntive | L1, R1, L2, R2 e clic stick | LB, RB, LT, RT e clic stick |
| Pausa nativa | Options | Start |
| Pannello online durante il gioco | PS | F6 |

Il tasto PS viene letto dal dispositivo DirectInput Sony (USB/Bluetooth), con `Guide=13` come valore predefinito. Per altri pad DirectInput si puo configurare `Guide` con il numero del pulsante in joy.cpl; `0` lo disabilita. XInputGetState non espone il tasto Guide: con un remapper XInput usare F6 oppure il dispositivo Sony DirectInput.

Da tastiera il pannello è disponibile anche con **F6**. Options/Start da solo durante il gioco non abbandona la sessione. Una pausa locale non mette in pausa gli altri PC; il personaggio in pausa viene indicato come fermo. Per uscire scegliere **DISCONNETTI**. Il pannello online assorbe i comandi mentre è aperto.

Per scrivere senza tastiera: sinistra/destra spostano il cursore, su/giù cambiano il carattere, Triangle/Y aggiunge un carattere, Square/X cancella quello precedente, Cross/A salva e Circle/B annulla. R1/RB scorre gli IP locali solo quando non si sta modificando un campo.

Il collegamento al motore usa il primo controller locale, preserva i binding tastiera/mouse e filtra la vecchia lettura joystick per evitare doppi comandi. Gli stick sono analogici, con deadzone configurabile. I trigger XInput sono analogici; i profili DirectInput Sony usano attualmente i pulsanti L2/R2 come valori digitali. Restano i profili DualShock 4, DualSense e le rimappature generiche dell'aggiornamento precedente. Nessun test con controller fisici USB/Bluetooth è stato eseguito qui.

In caso di duplicazione fisico/virtuale da parte di un remapper, la precedente opzione `[Gamepads] Backend=xinput` resta disponibile. La mod non installa driver né cambia mappature di sistema. `NativePadBridge=0` disattiva solo il ponte verso i comandi nativi e lascia al gioco i suoi binding.

## Risorse, nomi e distanza

Il riconoscimento del modello supporta le identità Spider-Man, Venom, Peter Parker, Carnage e Black Suit. Non crea i relativi asset. Le sezioni locali `[SpiderMan]`, `[Venom]`, `[Parker]`, `[Carnage]`, `[BlackSuit]` accettano coppie `Pack1`/`Entity1` fino a `Pack6`/`Entity6`. Risorse mancanti producono un avviso, non una sostituzione silenziosa con un altro personaggio. Black Suit e mod personalizzate richiedono pack compatibili.

`ActorDistance=250` limita il caricamento/render dei modelli remoti; lo scaricamento usa 100 unità aggiuntive di isteresi. `NameDistance=80` controlla la distanza dei nomi. Sono unità del mondo del gioco. I blip dei partecipanti nello stesso livello restano disponibili oltre il raggio del modello.

I nomi usano per ora un punto a 2,4 unità sopra la radice, non l'osso testa: l'allineamento durante arrampicata, pose rovesciate e animazioni estreme richiede collaudo. Vengono scartati punti dietro la camera/fuori schermo; non esiste ancora un test di occlusione contro i muri. Le coordinate della mappa e il passaggio alla mappa estesa richiedono verifica visiva nativa.

Il primo caricamento del pack è sincrono sul thread del gioco, mai nel render hook. È limitato a un tentativo per frame, ma **può ancora produrre scatti**. `SendHz=20` e `InterpolationMs=100` sono valori iniziali, non un'ottimizzazione definitiva per ogni rete.

## Prove incluse e limiti

Cinque suite nuove passano con GCC e con Clang + AddressSanitizer/UndefinedBehaviorSanitizer. Comprendono otto sessioni logiche su socket TCP/UDP veri e una prova con tre processi separati. I test del menu eseguono il codice di orchestrazione reale, ma sostituiscono Windows, renderer e motore con collaboratori simulati. Non verificano pixel, ABI Windows, gamepad fisici o Radmin.

Due immagini retail fornite negli array sorgente superano il confronto statico di sette slot menu e del passaggio nativo di rendering. Questo non prova che tutti i pack, tutte le chiamate native e tutti i mod siano compatibili.

Leggere `docs/online/VERIFICATION.md` per risultati, comandi, rischi e criteri mancanti. I log sono in `docs/online/checks/`. `multiplayer_online.log` raccoglie i principali eventi locali quando il gioco inizializza questo modulo; non contiene token di sessione.

La rete è destinata a partecipanti fidati: nessuna cifratura applicativa, password di sessione, account, matchmaking o migrazione dell'host. Se l'host esce, i client perdono la sessione. Il token UDP limita l'accettazione di pacchetti estranei ma non è un sistema di autenticazione completo. Non esporre la porta su reti non fidate.

## Riferimenti tecnici

I collegamenti documentano le API utilizzate, non certificano questa mod.

- Radmin VPN, rete e connessione ai giochi: https://www.radmin-vpn.com/help/
- Microsoft Winsock `recvfrom`: https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-recvfrom
- Microsoft `BCryptGenRandom`: https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcryptgenrandom
- Microsoft `GetAdaptersAddresses`: https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getadaptersaddresses
