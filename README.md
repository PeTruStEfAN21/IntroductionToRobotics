# IntroductionToRobotics
# Homework - Part 2: Falstad Circuits Simulation

Acest repository conține simulările și calculele analitice pentru circuitele electrice de curent continuu (DC) realizate în **Falstad Circuit Simulator**.

## Structura Proiectului

### 1. Series Circuit (5 V DC)
- **Topologie:** Sursă de 5 V DC conectată în serie cu două rezistoare ($R_1 = 100\,\Omega$, $R_2 = 200\,\Omega$) și un LED ($V_{LED} \approx 2\,\text{V}$).
- **Rezultate cheie:**
  - Curent prin circuit: $I = 10\,\text{mA}$ (în intervalul optim $5\text{–}20\,\text{mA}$).
  - Căderi de tensiune: $V_{R1} = 1\,\text{V}$, $V_{R2} = 2\,\text{V}$, $V_{LED} = 2\,\text{V}$.
  - Puteri disipate: $P_{R1} = 10\,\text{mW}$, $P_{R2} = 20\,\text{mW}$.
  - S-a verificat **Teorema a II-a a lui Kirchhoff (KVL)**: $\sum V = 5\,\text{V}$.

### 2. Parallel Circuit (9 V DC)
- **Topologie:** Sursă de 9 V DC care alimentează două ramuri paralele independente, fiecare având un rezistor și un LED în serie ($R_1 = 470\,\Omega$, $R_2 = 1\,\text{k}\Omega$).
- **Rezultate cheie:**
  - Curenți pe ramuri: $I_1 \approx 14.89\,\text{mA}$ și $I_2 = 7.0\,\text{mA}$.
  - Curent total: $I_{total} = 21.89\,\text{mA}$.
  - Puteri disipate: $P_{R1} \approx 104.3\,\text{mW}$, $P_{R2} = 49.0\,\text{mW}$.
  - S-au verificat **KCL** în nodul principal ($I_{total} = I_1 + I_2$) și **KVL** pe fiecare buclă.

### 3. RC Circuit - Capacitive Fade-Out (12 V DC)
- **Topologie:** Circuit de încărcare/descărcare RC cu buton (Push Button), rezistor de încărcare ($100\,\Omega$), ramură cu LED + rezistor de limitare ($1\,\text{k}\Omega$) și două condensatoare ($C_1 = C_2 = 1000\,\mu\text{F}$).
- **Comparație Serie vs. Paralel:**
  - **Serie:** $C_{eq} = 500\,\mu\text{F}$, $\tau = 0.5\,\text{s}$ (descărcare rapidă).
  - **Paralel:** $C_{eq} = 2000\,\mu\text{F}$, $\tau = 2.0\,\text{s}$ (efect pronunțat de *fade-out* la eliberarea butonului).
- **Analiză Tranzitorie (Prag $V_{target} = 6\,\text{V}$, configurație paralel):**
  - Încărcare ($0\,\text{V} \rightarrow 6\,\text{V}$): $t \approx 0.139\,\text{s}$.
  - Descărcare ($12\,\text{V} \rightarrow 6\,\text{V}$): $t \approx 1.386\,\text{s}$ (ideal RC) / $1.832\,\text{s}$ (cu pragul LED-ului).

## Cum se rulează simulările
1. Deschide [Falstad Circuit Simulator](https://www.falstad.com/circuit/).
2. Mergi la `File` -> `Import From Text...` (sau `Open File...`) și încarcă fișierele `.txt` aferente fiecărui exercițiu din acest repository.
