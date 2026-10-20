# Homework - Part 2: Falstad Circuits Simulation

Acest repository conține simulările realizate în **Falstad Circuit Simulator**, împreună cu toate calculele analitice explicite pentru curent, căderi de tensiune, puteri disipate și regim tranzitoriu RC.

*(Notă: În simulări s-a utilizat un LED roșu standard cu căderea de tensiune în conducție directă $V_{LED} = 2.0\,\text{V}$).*

---

## 1. Series Circuit (5 V DC)

### Configurația Circuitului
- **Sursă de tensiune:** $V_S = 5\,\text{V DC}$
- **Rezistoare în serie:** $R_1 = 100\,\Omega$, $R_2 = 200\,\Omega$
- **LED în serie:** $V_{LED} = 2.0\,\text{V}$


1. **Rezistența echivalentă ($R_{eq}$):**
   $$R_{eq} = R_1 + R_2 = 100\,\Omega + 200\,\Omega = 300\,\Omega$$

2. **Curentul prin circuit ($I$):**
   Conform Legii lui Ohm aplicate pe întregul circuit serie:
   $$I = \frac{V_S - V_{LED}}{R_{eq}} = \frac{5\,\text{V} - 2.0\,\text{V}}{300\,\Omega} = \frac{3.0\,\text{V}}{300\,\Omega} = 0.01\,\text{A} = \mathbf{10\,\text{mA}}$$
   *(Valoarea respectă cerința $I \in [5\,\text{mA}, 20\,\text{mA}]$).*

3. **Căderile de tensiune pe fiecare rezistor:**
   $$V_{R1} = I \cdot R_1 = 0.01\,\text{A} \cdot 100\,\Omega = \mathbf{1.0\,\text{V}}$$
   $$V_{R2} = I \cdot R_2 = 0.01\,\text{A} \cdot 200\,\Omega = \mathbf{2.0\,\text{V}}$$

4. **Puterea disipată pe fiecare rezistor:**
   $$P_{R1} = I^2 \cdot R_1 = (0.01\,\text{A})^2 \cdot 100\,\Omega = 0.0001 \cdot 100 = 0.01\,\text{W} = \mathbf{10\,\text{mW}}$$
   $$P_{R2} = I^2 \cdot R_2 = (0.01\,\text{A})^2 \cdot 200\,\Omega = 0.0001 \cdot 200 = 0.02\,\text{W} = \mathbf{20\,\text{mW}}$$

5. **Verificare KVL (Teorema a II-a a lui Kirchhoff):**
   Suma algebrică a tensiunilor pe ochiul de circuit este zero:
   $$-V_S + V_{R1} + V_{R2} + V_{LED} = 0$$
   $$-5.0\,\text{V} + 1.0\,\text{V} + 2.0\,\text{V} + 2.0\,\text{V} = 0\,\text{V} \implies \mathbf{5.0\,\text{V} = 5.0\,\text{V}} \quad (\text{Verificat})$$

---

## 2. Parallel Circuit (9 V DC)

### Configurația Circuitului
- **Sursă de tensiune:** $V_S = 9\,\text{V DC}$
- **Ramura 1:** Rezistor $R_1 = 470\,\Omega$ în serie cu $\text{LED}_1$ ($V_{LED1} = 2.0\,\text{V}$)
- **Ramura 2:** Rezistor $R_2 = 1000\,\Omega$ ($1\,\text{k}\Omega$) în serie cu $\text{LED}_2$ ($V_{LED2} = 2.0\,\text{V}$)


1. **Căderile de tensiune și curenții pe fiecare ramură ($I_1, I_2$):**
   Deoarece ramurile sunt conectate în paralel direct la sursa de $9\,\text{V}$:
   - **Ramura 1:**
     $$V_{R1} = V_S - V_{LED1} = 9.0\,\text{V} - 2.0\,\text{V} = 7.0\,\text{V}$$
     $$I_1 = \frac{V_{R1}}{R_1} = \frac{7.0\,\text{V}}{470\,\Omega} \approx 0.01489\,\text{A} = \mathbf{14.89\,\text{mA}} \in [5\,\text{mA}, 20\,\text{mA}]$$
   - **Ramura 2:**
     $$V_{R2} = V_S - V_{LED2} = 9.0\,\text{V} - 2.0\,\text{V} = 7.0\,\text{V}$$
     $$I_2 = \frac{V_{R2}}{R_2} = \frac{7.0\,\text{V}}{1000\,\Omega} = 0.007\,\text{A} = \mathbf{7.00\,\text{mA}} \in [5\,\text{mA}, 20\,\text{mA}]$$

2. **Curentul total ($I_{total}$) și verificare KCL (Teorema I a lui Kirchhoff):**
   În nodul principal de ramificație, curentul care intră este egal cu suma curenților care ies:
   $$I_{total} = I_1 + I_2 = 14.89\,\text{mA} + 7.00\,\text{mA} = \mathbf{21.89\,\text{mA}}$$
   $$\sum I_{in} = \sum I_{out} \implies 21.89\,\text{mA} = 14.89\,\text{mA} + 7.00\,\text{mA} \quad (\text{KCL Verificat})$$

3. **Puterea disipată pe fiecare rezistor:**
   $$P_{R1} = \frac{V_{R1}^2}{R_1} = \frac{(7.0\,\text{V})^2}{470\,\Omega} = \frac{49}{470} \approx 0.10425\,\text{W} = \mathbf{104.25\,\text{mW}}$$
   $$P_{R2} = \frac{V_{R2}^2}{R_2} = \frac{(7.0\,\text{V})^2}{1000\,\Omega} = \frac{49}{1000} = 0.049\,\text{W} = \mathbf{49.00\,\text{mW}}$$

4. **Verificare KVL pe fiecare buclă independentă:**
   - **Bucla 1:** $-V_S + V_{R1} + V_{LED1} = -9.0\,\text{V} + 7.0\,\text{V} + 2.0\,\text{V} = \mathbf{0\,\text{V}} \quad (\text{Verificat})$
   - **Bucla 2:** $-V_S + V_{R2} + V_{LED2} = -9.0\,\text{V} + 7.0\,\text{V} + 2.0\,\text{V} = \mathbf{0\,\text{V}} \quad (\text{Verificat})$

---

## 3. RC Circuit (12 V DC)

### Configurația Circuitului
- **Sursă de tensiune:** $V_S = 12\,\text{V DC}$
- **Comutator:** Push Button (NO - Normally Open)
- **Rezistor de încărcare (în serie cu butonul):** $R_{charge} = 100\,\Omega$
- **Rezistor de descărcare / limitare curent LED:** $R_{LED} = 1000\,\Omega$ ($1\,\text{k}\Omega$)
- **Condensatoare utilizate:** $C_1 = 1000\,\mu\text{F} = 1\,\text{mF}$ și $C_2 = 1000\,\mu\text{F} = 1\,\text{mF}$



### A. Capacitatea Echivalentă ($C_{eq}$) și Constanta de Timp ($\tau$)

Raportat la rezistența ramurii de descărcare $R = R_{LED} = 1000\,\Omega$:

1. **Cazul 1: Condensatoare conectate în SERIE**
   $$C_{serie} = \frac{C_1 \cdot C_2}{C_1 + C_2} = \frac{1000\,\mu\text{F} \cdot 1000\,\mu\text{F}}{1000\,\mu\text{F} + 1000\,\mu\text{F}} = \mathbf{500\,\mu\text{F}} \text{ (sau } 0.5\,\text{mF)}$$
   - **Constanta de timp la descărcare ($\tau_{dis, serie}$):**
     $$\tau_{dis, serie} = R_{LED} \cdot C_{serie} = 1000\,\Omega \cdot 0.0005\,\text{F} = \mathbf{0.5\,\text{s}}$$
   - **Constanta de timp la încărcare ($\tau_{ch, serie}$, cu $R_{charge} = 100\,\Omega$):**
     $$\tau_{ch, serie} = R_{charge} \cdot C_{serie} = 100\,\Omega \cdot 0.0005\,\text{F} = \mathbf{0.05\,\text{s}}$$

2. **Cazul 2: Condensatoare conectate în PARALEL**
   $$C_{paralel} = C_1 + C_2 = 1000\,\mu\text{F} + 1000\,\mu\text{F} = \mathbf{2000\,\mu\text{F}} \text{ (sau } 2\,\text{mF)}$$
   - **Constanta de timp la descărcare ($\tau_{dis, paralel}$):**
     $$\tau_{dis, paralel} = R_{LED} \cdot C_{paralel} = 1000\,\Omega \cdot 0.002\,\text{F} = \mathbf{2.0\,\text{s}}$$
   - **Constanta de timp la încărcare ($\tau_{ch, paralel}$, cu $R_{charge} = 100\,\Omega$):**
     $$\tau_{ch, paralel} = R_{charge} \cdot C_{paralel} = 100\,\Omega \cdot 0.002\,\text{F} = \mathbf{0.2\,\text{s}}$$

> **Comparație:** Configurația în **paralel** are o capacitate echivalentă și o constantă de timp de **4 ori mai mari** decât configurația în serie ($\tau_{paralel} = 2.0\,\text{s}$ vs. $\tau_{serie} = 0.5\,\text{s}$). Astfel, la eliberarea butonului, LED-ul din circuitul paralel se stinge treptat (*fade-out* mult mai lent), pe când în serie se stinge de 4 ori mai repede.

---

### B. Calculul Timpului $t$ pentru Atingerea Pragului de Tensiune ($V_{target} = 6.0\,\text{V}$)

Alegem pragul de tensiune **$V_{target} = 6.0\,\text{V}$** pentru configurația **în paralel** ($C_{eq} = 2000\,\mu\text{F}$):

#### 1. Procesul de Încărcare (Charging)
- **Tensiune inițială:** $V_{init} = 0\,\text{V}$
- **Tensiune asimptotică maximă:** $V_{max} = 12.0\,\text{V}$ (sau $V_{th} \approx 11.09\,\text{V}$ ținând cont de divizorul format de $R_{charge} = 100\,\Omega$ și ramura LED-ului $R_{LED} = 1000\,\Omega$)
- **Tensiune țintă:** $V(t) = 6.0\,\text{V}$
- **Ecuația de încărcare standard ($\tau_{ch} = 0.2\,\text{s}$):**
  $$V(t) = V_{max} \cdot \left(1 - e^{-\frac{t}{\tau_{ch}}}\right)$$
  $$6.0 = 12.0 \cdot \left(1 - e^{-\frac{t}{0.2}}\right) \implies 0.5 = 1 - e^{-\frac{t}{0.2}} \implies e^{-\frac{t}{0.2}} = 0.5$$
  $$-\frac{t}{0.2} = \ln(0.5) \implies t = -0.2 \cdot (-0.6931) \approx \mathbf{0.1386\,\text{s}} \text{ (138.6 ms)}$$

#### 2. Procesul de Descărcare (Discharging)
- **Tensiune inițială (condensator încărcat):** $V_0 = 12.0\,\text{V}$ (sau $\approx 11.09\,\text{V}$ în echilibru cu ramura LED)
- **Tensiune țintă:** $V(t) = 6.0\,\text{V}$
- **Constanta de timp la descărcare:** $\tau_{dis} = 2.0\,\text{s}$

- **Varianta 1 (Descărcare RC ideală spre $0\,\text{V}$):**
  $$V(t) = V_0 \cdot e^{-\frac{t}{\tau_{dis}}}$$
  $$6.0 = 12.0 \cdot e^{-\frac{t}{2.0}} \implies e^{-\frac{t}{2.0}} = 0.5 \implies t = -2.0 \cdot \ln(0.5) \approx \mathbf{1.386\,\text{s}}$$

- **Varianta 2 (Descărcare reală în Falstad prin LED-ul cu pragul $V_{LED} = 2.0\,\text{V}$):**
  Deoarece condensatorul se descarcă prin ramura cu LED, tensiunea asimptotică spre care tinde până la blocarea LED-ului este $V_{\infty} = V_{LED} = 2.0\,\text{V}$:
  $$V(t) = V_{LED} + (V_0 - V_{LED}) \cdot e^{-\frac{t}{\tau_{dis}}}$$
  $$6.0 = 2.0 + (12.0 - 2.0) \cdot e^{-\frac{t}{2.0}} \implies 4.0 = 10.0 \cdot e^{-\frac{t}{2.0}} \implies e^{-\frac{t}{2.0}} = 0.4$$
  $$t = -2.0 \cdot \ln(0.4) = -2.0 \cdot (-0.9163) \approx \mathbf{1.833\,\text{s}}$$

---

## Fișierele de Simulare (Import in Falstad)
- `1_series_circuit.txt` – Circuitul Serie (5 V DC)
- `2_parallel_circuit.txt` – Circuitul Paralel (9 V DC)
- `3a_rc_parallel.txt` – Circuitul RC cu condensatoare în paralel (12 V DC)
- `3b_rc_series.txt` – Circuitul RC cu condensatoare în serie (12 V DC)