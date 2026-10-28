# Esperimenti riproducibili

## Protocollo eseguito

`make experiments` esegue i quattro preset a `T=500`, `dt=0.01` e `0.005`,
`N=512`, `L=128`, campo inizialmente nero, posizione centrale, θ=π/2.
Non usa randomizzazione. I valori completi sono in
[`experiments/results.json`](../experiments/results.json); lo script è
[`experiments/run.py`](../experiments/run.py). Floating point, compilatore
e libm possono cambiare dettagli su tempi lunghi.

Risultati arrotondati (spostamento netto nella traiettoria sollevata dal toro):

| Preset | dt | Spostamento | Cambi di segno c−0.5 | Finito |
|---|---:|---:|---:|---|
| orbit | .01 | 2.980 | 0 | sì |
| orbit | .005 | 2.980 | 0 | sì |
| balanced | .01 | 183.728 | 441 | sì |
| balanced | .005 | 535.107 | 498 | sì |
| broad | .01 | 746.373 | 385 | sì |
| broad | .005 | 759.926 | 391 | sì |
| fast-turn | .01 | 200.859 | 868 | sì |
| fast-turn | .005 | 632.503 | 774 | sì |

Sono configurazioni motivate dimensionalmente e **provate**, non risultati di
una ricerca esaustiva/ottimizzazione. `broad` mantiene circa costanti
ωσ/v e κσ/v, raddoppia la scala locale ma non L: il rapporto dominio/kernel
cambia. La somiglianza di due spostamenti finali non dimostra somiglianza
delle traiettorie o stabilità di una struttura.

Il conteggio di inversioni campiona il segno alla fine di ogni passo, non misura
esattamente gli zeri continui e non è un esponente di Lyapunov.
`active_fraction` è la frazione **istantanea** di nodi con colore >0.1,
non l'area mai visitata: una seconda dose può scurire una zona già attraversata.
I range si riferiscono alle coordinate non avvolte e possono superare L.

## Validazione distinta dall'esplorazione

`make test` verifica intervalli brevi e controlli analitici, oltre a 50000 passi
per preset. I confronti a T=2 hanno mostrato:

| Raffinamento | Differenza stato | RMS campo |
|---|---:|---:|
| dt .01 vs .005, h=.25 | 0.000674018 | 0.0000103628 |
| dt .005 vs .0025, h=.25 | 0.000307878 | 0.00000339735 |
| h .25 vs .125, dt=.0025 | 0.236961 | 0.00985352 |
| h .125 vs .0625, dt=.0025 | 0.0598846 | 0.00249315 |

Le differenze di stato sommano distanza delle posizioni e distanza angolare
periodica; il campo viene confrontato sui nodi della griglia più grossolana.
Il test di griglia usa `L=32`, con `N=128,256,512`; quello temporale `L=128,N=512`.
La sensibilità spaziale **non è trascurabile**, nonostante la convergenza
osservata: le misure quantitative richiedono raffinamenti ulteriori.

Per κ=0, raggio `v/omega=2` e periodo `2pi/omega`: periodicità del modello
analitico dimostrabile, non di una struttura emergente. L'integrazione numerica
approssima quel cerchio, non si richiede una ricorrenza esatta floating point.

## Controllo aggiuntivo di lunga durata

È stato eseguito anche:

```sh
./build/ant-headless --preset balanced --steps 1000000
```

Con i default (`dt=.01`, `N=512`) raggiunge `T=10000`, con stato e campo
finiti (`finite=true`), colore medio circa 0.498313 e 40663 cambi di segno
campionati. Tempo osservato circa 24 secondi, memoria residente circa 4.4 MB
sulla macchina di sviluppo con compilazione `-O2`: indicazione di fattibilità,
non benchmark portabile. Questo controllo non è incluso negli otto esperimenti
di `results.json` e non verifica la convergenza della traiettoria lunga.

## Ricerche successive (non risultati acquisiti)

- Periodicità: confrontare **agente e campo**, non solo ritorni della posizione.
- Stabilità: perturbare dato iniziale/parametri e confrontare osservabili a
  più risoluzioni, tenendo fisso il dominio fisico.
- Caos: convergenza prima della separazione, esponenti/indicatori appropriati;
  divergenza tra due timestep non basta.
- Autostrade: lavorare con coordinate sollevate, verificare deriva persistente
  e motivi spaziali ripetuti prima e dopo gli attraversamenti del bordo.

Esempio di campionamento più fine, da confrontare con la medesima durata:

```sh
./build/ant-headless --preset balanced --resolution 1024 --dt .0025 \
  --steps 200000 --every 100 --csv build/fine.csv
```

Nessun preset viene presentato come prova di autostrada, caos o attrattore stabile.
