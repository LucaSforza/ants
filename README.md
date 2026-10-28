# Ants — formica di Langton discreta e continua

C11 e raylib 6.0 (già inclusa). La modalità originale rimane quella predefinita;
la nuova modalità è un sistema dinamico accoppiato agente–campo, **non**
un'interpolazione del moto su celle e non un'implementazione di Lenia.

## Compilazione ed esecuzione

Su Linux servono compilatore C, Make, CMake, header OpenGL/X11 e le dipendenze
desktop di raylib. Nessuna nuova libreria per il modello numerico.

```sh
make                         # raylib locale + build/ant
./build/ant                  # originale: 12000 passi, 100 passi/frame
./build/ant -t 1000000 -s 1000
./build/ant --continuous
./build/ant --continuous --preset broad --trail --rate 8
./build/ant --continuous --speed 6 --omega 3 --kappa 7.52 \
  --sigma 1 --size 128 --resolution 512 --dt 0.005
./build/ant --continuous --help
```

Solo calcolo, anche senza raylib, display o dipendenze grafiche installate:

```sh
make headless
./build/ant-headless --preset balanced --steps 100000 --csv build/path.csv --every 100
# Equivalente, dal programma grafico:
./build/ant --continuous --headless --steps 100000
```

Headless stampa un riepilogo JSON e, opzionalmente, campioni CSV della traiettoria
(coordinate toroidali, orientamento, colore percepito; incluso stato iniziale e
finale). `--every` riguarda solo il CSV. `--steps` limita i passi; in GUI il
default è illimitato, headless è 100000. Gli override numerici prevalgono sul
preset indipendentemente dall'ordine degli argomenti. `--continuous` / `-c`
deve essere il primo argomento del programma grafico.

## Modello

Il dominio è il toro quadrato di lato $L$. Con posizione $X=(x,y)$ e
orientamento $\theta$ (radianti):

$$
\dot x=v\cos\theta,\qquad \dot y=v\sin\theta,\qquad
\dot\theta=\omega[2c(X,t)-1],
$$

$$
\partial_t\phi(z,t)=\kappa K_\sigma(z-X(t)),\qquad
c(z,t)=\frac{1-\cos\phi(z,t)}2.
$$

Stato iniziale: campo nero $\phi=0$, agente al centro, $\theta=\pi/2$.
Le coordinate seguono lo schermo raylib: $y$ cresce verso il basso,
quindi un angolo crescente appare orario. Non si pretende di mantenere la
stessa convenzione destra/sinistra della regola discreta.

### Perché una fase

L'identità **esatta** $c(\phi+\pi)=1-c(\phi)$ generalizza l'inversione del
colore: una dose integrata di fase pari a π ribalta qualsiasi colore.
La scrittura è continua e la dose dipende da velocità, distanza e ritorni;
non si forza un flip a ogni attraversamento. Il solo colore non determina
la dinamica: fasi opposte possono avere lo stesso colore ma evoluzione diversa.
La legge alternativa $\dot c=a(1-2c)$ tende invece al grigio e non alterna.

### Kernel realmente implementato

Sia $d$ lo spostamento periodico minimo, $q=|d|^2/\sigma^2$ e
$z=(q-9)/7$. Usiamo $K_\sigma(d)=e^{-q/2}T(q)$, con:

$$
T(q)=\begin{cases}
1 & q\le9,\\
(1-z)^3(1+3z+6z^2) & 9<q<16,\\
0 & q\ge16.
\end{cases}
$$

È gaussiano entro 3σ e raccordato $C^2$ a zero a 4σ: nessun taglio brusco.
Il picco è 1, **non** l'integrale; cambiare σ cambia la dose totale.
La condizione $4\sigma\le L/2$ rende il supporto periodico non ambiguo.
Questa scelta permette aggiornamenti locali efficienti ed è parte del modello,
non un'approssimazione nascosta della gaussiana infinita.

## Parametri

Le unità di lunghezza e tempo sono arbitrarie ma coerenti.

| Opzione | Default | Significato |
|---|---:|---|
| `--speed` | 6 | $v$, lunghezze / tempo; può essere zero |
| `--omega` | 3 | $\omega$, massima velocità angolare, rad / tempo |
| `--kappa` | 7.52 | $\kappa$, massima velocità di scrittura, rad di fase / tempo |
| `--sigma` | 1 | scala spaziale del kernel; supporto 4σ |
| `--size` | 128 | $L$, lato fisico del toro |
| `--resolution` | 512 | $N$, nodi per lato, $h=L/N$ |
| `--dt` | 0.01 | $\Delta t$, passo fisso in tempo simulato |
| `--rate` | 1 | tempo simulato richiesto / secondo reale, non cambia le equazioni |

Per un attraversamento rettilineo isolato sul centro del kernel,
$\Delta\phi\approx\kappa\sqrt{2\pi}\sigma/v$ (trascurando il raccordo).
Il default sceglie questa quantità vicina a π. È una **stima**, non una
garanzia: la traiettoria reale curva e l'agente percepisce anche la propria
scrittura. Parametri adimensionali utili: $\omega\sigma/v$,
$\kappa\sigma/v$ e $L/\sigma$ per $v>0$.

## Visualizzazione e controlli (modalità continua)

- Campo: blu scuro = 0, ciano = 0.5, giallo = 1; formica rossa e direzione bianca.
- **Spazio**: pausa/riprendi; **R**: reset dello stato, mantiene parametri e rate.
- **+ / -**: raddoppia/dimezza il rate (da 1/1024 a 1024).
- **T**: mostra/nasconde la traiettoria; **N**: un passo mentre in pausa.
- Pannello: tempo simulato, iterazioni, parametri, rate e arretrato di calcolo.

L'accumulatore temporale usa il tempo reale ma esegue solo passi di dimensione
`dt`. Un budget di lavoro di circa 10 ms/frame mantiene reattiva l'interfaccia;
in sovraccarico l'arretrato si conserva e il programma rallenta rispetto al rate
richiesto, **senza aumentare dt o saltare evoluzione**. Pausa: non accumula tempo.
La traccia conserva al massimo 8192 campioni, uno ogni almeno 0.04 unità simulate;
si interrompe visivamente alle giunzioni del toro. Lo stato numerico non dipende
dalla traccia, dalla frequenza di disegno o dal CSV.

## Discretizzazione e limiti

`continuous.c` non dipende da raylib. A ogni nodo si integra la fase e si
ricostruisce $c_h=\mathcal B_h[(1-\cos\phi_{ij})/2]$ mediante interpolazione
bilineare **periodica dei colori**. Le fasi si memorizzano modulo 2π per evitare
crescita illimitata; interpolarle direttamente introdurrebbe false transizioni.

Il midpoint esplicito (RK2) integra insieme agente e campo: il colore allo stadio
intermedio include la mezza scrittura valutata nella posizione iniziale.
Non è un aggiornamento sequenziale agente-poi-campo. Dettagli e formule in
[`docs/model.tex`](docs/model.tex), compilabile con `make latex` (TeX opzionale).

La CLI rifiuta NaN, infiniti e configurazioni non risolte:

$$
h\le\sigma/2,\quad v\Delta t\le\sigma/4,\quad
\omega\Delta t\le0.1,\quad \kappa\Delta t\le0.25.
$$

Sono criteri prudenziali di campionamento, **non un teorema di accuratezza**.
$32\le N\le2048$, $1\le L\le10^6$, $0\le v,\omega,\kappa\le10^6$,
$10^{-9}\le\Delta t\le1$. Il default ha $h=\sigma/4$; per misure precise
raffinare ulteriormente griglia e timestep. Il campo non diffonde né decade e
può sviluppare dettagli sotto-griglia; su tempi lunghi la convergenza delle
singole traiettorie non è garantita. Il bilineare è continuo ma solo a tratti
derivabile, quindi non si assume ordine due uniforme per ogni traiettoria.
La riduzione modulo conserva esattamente il modello in aritmetica esatta,
ma resta l'errore floating point. Il contatore è `uint64_t`, il tempo `double`:
non si promette precisione per un numero arbitrariamente grande di passi.

Costo: memoria $O(N^2)$; scrittura $O((\sigma/h)^2)$ per passo e rendering
$O(N^2)$ solo quando cambia lo stato. Nessuna scansione completa della griglia
durante un passo del nucleo numerico.

## Esperimenti e validazione

```sh
make test                    # C numerico + CLI/CSV (Python 3 standard library)
make sanitize CC=clang       # AddressSanitizer + UndefinedBehaviorSanitizer
make experiments             # rigenera experiments/results.json, T=500
```

| Preset | Variazioni rispetto al default | Scopo / osservazione |
|---|---|---|
| `orbit` | κ=0 | controllo: cerchio analitico di raggio 2, non emergenza |
| `balanced` | nessuna | scrittura circa π in passaggio rettilineo, numerose inversioni |
| `broad` | σ=2, κ=3.76, ω=1.5 | scala locale doppia, stessa dose approssimata; esplorazione estesa |
| `fast-turn` | ω=6 | curvatura più forte e molte inversioni del segno di rotazione |

Sono stati eseguiti otto esperimenti a $T=500$, con $N=512$ e
$\Delta t=0.01,0.005$: tutti finiti. I risultati includono estensione della
traiettoria sollevata dal toro, spostamento, cambi di segno, colore medio e
frazione di nodi con $c>0.1$. Le differenze finali sono anche grandi: non sono
prove di caos, periodicità o autostrade. Vedi [protocollo e risultati](docs/experiments.md).

I test confrontano orbita e scrittura a posizione fissa con soluzioni analitiche,
verificano interpolazione (anche alla cucitura delle fasi), bordi periodici,
equivarianza per traslazioni di griglia, reset, validazione e 50000 passi per
ciascun preset. La regola discreta è confrontata con un riferimento indipendente
per 200000 passi. Nei test di raffinamento a $T=2$:

- `dt` 0.01 → 0.005 → 0.0025: differenze successive dello stato circa
  $6.74\cdot10^{-4}$ → $3.08\cdot10^{-4}$;
- $h$ 0.25 → 0.125 → 0.0625 a `dt=0.0025`: circa 0.237 → 0.0599;
- scrittura stazionaria analitica: dimezzare `dt` riduce l'errore angolare di ~4.

Lo stato confrontato usa distanza periodica delle posizioni più differenza
angolare (metrica diagnostica, unità arbitrarie); si confronta anche il campo
in RMS. Sono **osservazioni numeriche riproducibili su intervalli finiti**,
non dimostrazioni di convergenza globale o indipendenza dalla griglia.

Matematicamente si hanno $0\le c\le1$, velocità costante, $|\dot\theta|\le\omega$,
la proprietà di inversione per dose π e, per dato iniziale regolare, esistenza
globale e unicità del modello regolarizzato. Non si dimostra l'esistenza di
strutture emergenti. Sul toro finito un'eventuale “autostrada” va distinta da
un transiente con deriva nelle coordinate sollevate.

## Organizzazione

- `discrete.c`: regola e CLI originali; coordinate ora ridotte sul toro anche
  nello stato per evitare overflow nelle corse lunghe, senza cambiare le celle visitate.
- `continuous.c`, `continuous.h`: modello numerico indipendente dalla grafica.
- `continuous_cli.c`: parametri, headless e CSV; `continuous_ui.c`: raylib.
- `tests/`, `experiments/`: verifiche e protocollo riproducibile.
- [`docs/model.tex`](docs/model.tex): modellazione matematica e metodo numerico.
- [`docs/experiments.md`](docs/experiments.md): protocollo, risultati e limiti.
