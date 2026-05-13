# Praktikumsversuch: Vom induktiven Sensor zur Münzklassifikation

**Dauer:** ca. 2 Stunden  
**Gruppen:** 2-3 Personen  
**Ziel:** Sensorprinzip verstehen, Messdaten aufnehmen, Merkmale bilden und drei Klassifikationsverfahren vergleichen. Als Objekte stehen Münzen verschiedener Währungen und Länder zur Verfügung — Euro, US-Dollar, Schweizer Franken, britische Pfund und weitere. Ergänzend können Unterlegscheiben, Büroklammern oder Alufolie als Referenzmaterialien verwendet werden.

Der Versuch besteht aus drei Abschnitten. Jeder Abschnitt beginnt mit Theorie, danach folgt ein Praxisteil und am Ende eine kurze Diskussion bzw. ein Quiz.

---

## Abschnitt 1: Sensorprinzip und Abstand

### Theorie

Der LDC1101 ist ein Induktiv-Digital-Wandler. Er arbeitet mit einer Spule und einem Kondensator als LC-Schwingkreis:

```text
f0 = 1 / (2 * pi * sqrt(L * C))
```

Wenn sich ein metallisches Objekt der Spule nähert, ändern sich die Eigenschaften des Schwingkreises. Der Sensor liefert in diesem Versuch zwei Messwerte:

| Messwert | Bedeutung | Typische Interpretation |
|----------|-----------|-------------------------|
| `RP` | Parallelwiderstand / Dämpfung | sinkt bei stark leitfähigen Objekten durch Wirbelströme |
| `L` | Induktivitätswert / Frequenzverschiebung | sinkt bei allen Münzen im Kanalabstand (~1 mm); bei direktem Kontakt kann L bei ferromagnetischen Materialien kurz ansteigen |

Wichtige Einflussgrößen:

| Einflussgröße | Wirkung |
|---------------|---------|
| Material | Stahl, Kupferlegierungen und Aluminium beeinflussen `RP` und `L` unterschiedlich |
| Abstand | größere Distanz schwächt das Signal stark und nichtlinear |
| Objektfläche | größere Münzen koppeln stärker in das Feld ein |
| Lage | gekippte oder schräg geführte Münzen erzeugen andere Werte |
| Abtastrate | bestimmt, wie dicht der ESP32 Messwerte ausliest |
| Mittelung | glättet Rauschen, kann aber schnelle Änderungen verschmieren |

In Abschnitt 1 wird noch nicht der finale Sortieraufbau verwendet. Der Sensor sitzt in einer Distanz-Vorrichtung, mit der der Abstand zwischen Sensor und Münze kontrolliert eingestellt wird. Die LDC1101-Spezialregister bleiben fest eingestellt; variiert werden nur Abstand, Material, Abtastrate und optional die Mittelung in der Auswertung.

### Praxis

**Vorbereitung**

1. Sensor in der Distanz-Vorrichtung anschließen.
2. Mit dem WLAN `CoinSorter_XXXX` verbinden.
3. Das Passwort vom LCD ablesen.
4. Im Browser `192.168.4.1` öffnen.
5. Abtastrate zunächst auf 50 Hz stellen.

#### 1.1 Luftreferenz und Materialvergleich

Messe zuerst die Luftreferenz ohne Objekt. Danach halte jedes Objekt bei gleichem Abstand, z. B. 2 mm, für ca. 3 s über die Spule. Wählt selbst 6–8 Objekte aus dem verfügbaren Münzsortiment — nehmt bewusst Münzen aus verschiedenen Ländern und Materialgruppen.

| Objekt | Währung / Material | `RP` | `L` | `delta_RP` | `delta_L` | Beobachtung |
|--------|-------------------|------|-----|------------|-----------|-------------|
| Luft | — | | | - | - | |
| | | | | | | |
| | | | | | | |
| | | | | | | |
| | | | | | | |
| | | | | | | |
| | | | | | | |
| Büroklammer | Stahl | | | | | |
| Alufolie | Aluminium | | | | | |

Rechnung:

1. Bestimme `delta_RP` und `delta_L` für jedes Objekt.
2. Markiere das Objekt mit der größten `RP`-Änderung.
3. Markiere das Objekt mit der größten `L`-Änderung.

#### 1.2 Abstandsmessung

Wählt zwei Münzen aus verschiedenen Materialgruppen — idealerweise eine ferromagnetische (z. B. 1 ct Euro oder 1 Penny GB, Stahlkern) und eine nicht-ferromagnetische (z. B. 10 ct Euro oder 25 Cent USD, Kupferlegierung). Stellt nacheinander definierte Abstände ein.

| Abstand in mm | `RP` Münze 1 | `L` Münze 1 | `delta_RP` M1 | `delta_L` M1 | `RP` Münze 2 | `L` Münze 2 | `delta_RP` M2 | `delta_L` M2 |
|---------------|-------------|------------|--------------|-------------|-------------|------------|--------------|-------------|
| 0 | | | | | | | | |
| 1 | | | | | | | | |
| 2 | | | | | | | | |
| 4 | | | | | | | | |
| 6 | | | | | | | | |
| 8 | | | | | | | | |
| 10 | | | | | | | | |

Notiert welche Münzen ihr gewählt habt: Münze 1: __________ Münze 2: __________

Rechnung:

1. Zeichne `delta_RP` über dem Abstand.
2. Zeichne `delta_L` über dem Abstand.
3. Prüfe qualitativ: Ist der Zusammenhang linear?

#### 1.3 Wiederholbarkeit

Wählt eine Münze aus dem Sortiment und wiederholt dieselbe Messung fünfmal bei gleichem Abstand. Notiert welche Münze: __________

| Wiederholung | `RP` | `L` |
|--------------|------|-----|
| 1 | | |
| 2 | | |
| 3 | | |
| 4 | | |
| 5 | | |

Berechne für `RP` und `L`:

```text
Mittelwert = Summe der Werte / Anzahl
relative Streuung = Standardabweichung / Mittelwert
```

### Diskussion / Quiz

1. Bei ferromagnetischen Materialien wie Stahl erwartet man theoretisch einen **Anstieg** von `L`, weil die hohe Permeabilität µr das Magnetfeld der Spule verstärkt. Im Versuch sinkt `L` aber auch bei Stahlmünzen. Was verhindert den L-Anstieg bei ~900 kHz und 1 mm Abstand?  
   *Hinweis: Skintiefe δ = √(2ρ / ωμ)*
2. Bei welcher Betriebsfrequenz würde man den L-Anstieg bei Stahl auch auf Abstand messen können, und warum ist diese Frequenz für den LDC1101 nicht erreichbar?
3. Warum kann Alufolie eine deutliche `RP`-Änderung erzeugen, obwohl sie nicht ferromagnetisch ist?
4. Warum ist der Abstand eine kritische Störgröße?
5. Was bedeutet es für den Schwingkreis, wenn der gemessene `L`-Wert beim Objekt kleiner wird?
6. Warum kann Mittelung hilfreich sein, aber bei schnellen Bewegungen auch stören?

### Musterlösungen Abschnitt 1

1. Die Skintiefe bei Stahl (ρ ≈ 1×10⁻⁷ Ω·m, µr ≈ 200) bei 900 kHz beträgt δ = √(2×10⁻⁷ / (2π×9×10⁵×4π×10⁻⁷×200)) ≈ 17 µm. Das Magnetfeld dringt also nur 17 µm tief in den Stahl ein. Die Wirbelströme fließen in dieser dünnen Oberflächenschicht und schirmen das Innere ab — der Permeabilitätseffekt des Stahlkerns bleibt unsichtbar. Deshalb sinkt L auch bei Stahl.
2. Damit δ ≥ 1 mm gilt, muss f ≤ 127 Hz. Der LDC1101 arbeitet aber zwischen 500 kHz und 10 MHz — fünf Größenordnungen zu hoch.
3. Aluminium ist zwar nicht ferromagnetisch (µr ≈ 1, kein L-Effekt), aber ein sehr guter elektrischer Leiter. Die hohe Leitfähigkeit erzeugt starke Wirbelströme, die dem Schwingkreis Energie entziehen → RP sinkt deutlich.
4. Die Feldstärke fällt mit ~1/r³ ab. Schon eine Abstandsänderung von 1 mm verändert die Messwerte erheblich. Unterschiedliche Durchlaufgeschwindigkeiten oder schräg liegende Münzen führen zu variablen Abständen und damit zu streuenden Features.
5. Ein kleinerer L-Wert bedeutet eine höhere Resonanzfrequenz des LC-Kreises (f = 1/(2π√LC)). Das Objekt reduziert die effektive Induktivität durch sein Gegenfeld (Wirbelstrom-Gegenfeld schwächt das Spulenfeld).
6. Mittelung über mehrere Samples reduziert Messrauschen und stabilisiert stationäre Messwerte. Bei einer Münze die in 100–200 ms vorbeifliegt glättet eine zu lange Mittelung aber den Peak weg — Spitzenwerte werden unterschätzt und Peaks von schnell fallenden Münzen können vollständig verschwinden.

---

## Abschnitt 2: Datensatz und Merkmale

### Theorie

Ein Klassifikator arbeitet nicht gut mit beliebigen Rohdaten, sondern braucht vergleichbare Merkmale. Wenn eine Münze über den Sensor geführt wird, entsteht im Zeitverlauf ein Peak. Aus diesem Peak werden wenige Kennwerte extrahiert.

| Begriff | Bedeutung |
|---------|-----------|
| Rohdaten | alle Messpunkte `Zeit_ms`, `RP`, `L` |
| Peak | zusammenhängender Signalbereich, während eine Münze über dem Sensor ist |
| Label | bekannte Klasse, z. B. `euro_10ct`, `usd_quarter`, `gbp_penny` oder `chf_20rp` |
| Feature | berechnetes Merkmal aus dem Peak |
| Ausreißer | Messung, die nicht zur übrigen Klasse passt |

In diesem Versuch verwenden wir diese Peak-Features:

| Feature | Bedeutung |
|---------|-----------|
| `rp_min` | kleinster `RP`-Wert im Peak |
| `l_min` | kleinster `L`-Wert im Peak |
| `delta_rp` | `RP_Luft` minus `rp_min` — immer positiv |
| `delta_l` | `L_Luft` minus `l_min` — im Kanalabstand (~1 mm) immer positiv |
| `rp_avg` | Durchschnitt von `RP` im Messfenster (200 ms) |
| `l_avg` | Durchschnitt von `L` im Messfenster (200 ms) |

Der ESP32 berechnet alle sechs Features automatisch und schreibt sie direkt in eine exportierbare Tabelle (Peak-Feature-Export). Excel wird für Streudiagramme und manuelle Grenzwertanalyse genutzt.

### Praxis

**Vorbereitung**

1. Finalen Durchlauf-/Sortieraufbau verwenden.
2. Dashboard öffnen, Tab „Aufzeichnen" wählen.
3. Abtastrate auf 50 Hz stellen.
4. Unter „Peak-Feature-Export" auf **Start** klicken — der ESP32 beginnt, jeden erkannten Peak mit allen sechs Features zu puffern (max. 80 Einträge).

**Datensatz aufnehmen**

1. Münze gleichmäßig durch den Kanal führen — pro Münzsorte mind. 10 Durchläufe.
2. Wiederhole das für 6–8 verschiedene Münzsorten.
3. Den Zähler im Dashboard beobachten; er zeigt die Anzahl bereits gepufferter Peaks.

Zieldatensatz:

```text
6-8 Münzsorten × 10 Durchläufe = ca. 60–80 Peaks
```

4. Wenn genug Peaks gesammelt sind, auf **Stop** klicken und dann auf **Download** — das lädt `peaks.csv` herunter.
5. Datei umbenennen oder direkt so in Excel öffnen.

**Excel-Auswertung**

1. `peaks.csv` in Excel öffnen (Trennzeichen: Semikolon).
2. Streudiagramm aus zwei Features erstellen, z. B. `delta_rp` gegen `delta_l`.
3. Punkte nach Label (Münzsorte) einfärben.
4. Gruppen, Überlappungen und Ausreißer identifizieren.

> Tipp: Mit **Clear** im Dashboard lässt sich der Puffer zurücksetzen, um eine neue Aufnahmesession zu starten.

### Diskussion / Quiz

1. Warum sind mehrere Wiederholungen pro Münzsorte nötig?
2. Woran erkennt man einen schlechten Durchlauf?
3. Welche zwei Münzsorten liegen im Streudiagramm nah beieinander, obwohl sie aus verschiedenen Ländern stammen?
4. Welches Feature trennt ferromagnetische Münzen (z. B. 1 ct Euro, 1 Penny GB) von nicht-ferromagnetischen besonders gut?
5. Warum ist ein Peak-Feature robuster als ein einzelner Messpunkt?

### Musterlösungen Abschnitt 2

1. Eine einzelne Messung kann durch Schräglage, unterschiedliche Geschwindigkeit oder Rauschen verfälscht sein. Mehrere Wiederholungen zeigen die natürliche Streuung der Klasse und ermöglichen es, Ausreißer zu erkennen. Mit ~10 Samples pro Klasse lässt sich bereits ein stabiler Mittelwert und eine Standardabweichung schätzen.
2. Ein schlechter Durchlauf zeigt sich durch: Peak deutlich außerhalb des Clusters der anderen Wiederholungen, auffällig breiter oder asymmetrischer Peak (Münze schräg oder zu langsam), fehlender Peak (Münze hat den Sensor nicht getroffen), oder zwei Peaks in einer Aufnahme (Münze zweimal gezählt).
3. Münzen mit gleicher Materialzusammensetzung liegen nah beieinander, unabhängig von der Währung — z. B. 10 ct Euro und 25 Cent USD (beide Kupferlegierung, ähnliche Größe), oder 1 ct Euro und 1 Penny GB (beide Stahl mit Kupferplattierung).
4. `delta_rp` trennt ferromagnetische von nicht-ferromagnetischen Münzen weniger gut als erwartet, weil der Skin-Effekt bei ~900 kHz den Permeabilitätsunterschied unterdrückt. `delta_l` ist ebenfalls für alle Münzen positiv. Am besten trennt die **Kombination** aus `delta_rp` und `delta_l` im Streudiagramm — ferromagnetische Münzen tendieren zu größerem `delta_l` relativ zu `delta_rp`. Die Mittelwert-Features `rp_avg` und `l_avg` können zusätzlich helfen, Münzen zu trennen, die ähnliche Minimalwerte haben aber unterschiedliche Peakbreiten (z. B. große vs. kleine Münzen gleichen Materials).
5. Ein einzelner Messpunkt ist stark vom genauen Zeitpunkt des Lesens abhängig — ob die Münze gerade über der Spulenmitte ist oder schon vorbeigeflogen. Das Peak-Minimum (`rp_min`, `l_min`) ist das reproduzierbarste Merkmal, weil es den Extremwert des gesamten Durchlaufs erfasst, unabhängig von Abtastzeitpunkt und Geschwindigkeit.

---

## Abschnitt 3: Klassifikation, ML-Grundlagen und Sortierung

### Theorie

Klassifikation bedeutet: Aus Messmerkmalen wird eine Klasse vorhergesagt.

```text
[rp_min, l_min, delta_rp, delta_l, rp_avg, l_avg] -> Klasse
```

In diesem Abschnitt vergleichen wir drei Ansätze.

| Methode | Idee | Vorteil | Nachteil |
|---------|------|---------|----------|
| Manuelle Grenzen | Menschen legen Regeln im Streudiagramm fest | sehr erklärbar | schlecht bei überlappenden Klassen |
| k-NN | neue Messung bekommt die Klasse der nächsten Trainingspunkte | anschaulich, wenig Annahmen | braucht gute Skalierung und genug Daten |
| Entscheidungsbaum | automatische Wenn-Dann-Regeln | gut erklärbar | kann bei zu großer Tiefe überfitten |

Grundbegriffe des maschinellen Lernens:

| Begriff | Bedeutung |
|---------|-----------|
| Supervised Learning | Lernen aus Daten mit bekannten Labels |
| Trainingsdaten | Daten, mit denen das Modell gebaut wird |
| Testdaten | Daten, mit denen das Modell geprüft wird |
| Generalisierung | Modell funktioniert auch auf neuen Messungen |
| Overfitting | Modell passt zu stark auf Trainingsdaten und wird schlecht bei neuen Daten |
| Konfusionsmatrix | Tabelle aus tatsächlichen und vorhergesagten Klassen |

Welche Methode verwendet man wann?

| Situation | Geeignete Methode |
|-----------|-------------------|
| wenige, klar trennbare Klassen | manuelle Grenzen |
| wenig Modellannahmen, anschauliche Distanzen | k-NN |
| erklärbare automatische Regeln | Entscheidungsbaum |
| viele Daten, komplexe Grenzen | fortgeschrittene Modelle wie Random Forest, SVM oder neuronale Netze |

Für diesen Versuch bleiben wir bewusst bei erklärbaren Methoden. Ziel ist nicht maximale Modellkomplexität, sondern der Vergleich zwischen Genauigkeit, Erklärbarkeit und Robustheit.

### Praxis

#### 3.1 Manuelle Grenzen

1. Verwende das Streudiagramm aus Excel.
2. Zeichne Grenzen ein, die Münzgruppen trennen.
3. Formuliere Regeln, z. B.:

```text
Wenn delta_l groß ist -> ferromagnetische Gruppe
Wenn delta_rp groß und delta_l klein ist -> stark leitfähige Gruppe
Sonst -> übrige Objektgruppe
```

4. Trage die Regeln im Dashboard ein oder exportiere sie über das Notebook in `model.json`.

#### 3.2 k-NN und Entscheidungsbaum im Notebook

1. CSVs laden.
2. Peaks und Features extrahieren.
3. Trainings- und Testdaten aufteilen.
4. k-NN für mehrere `k` testen.
5. Entscheidungsbaum mit kleiner Tiefe trainieren, z. B. `max_depth = 3`.
6. Genauigkeit und Konfusionsmatrix für alle drei Methoden vergleichen.
7. Gemeinsames `model.json` exportieren.

Für Google Colab werden zwei getrennte Notebooks verwendet:

- `knn_notebook.ipynb` für k-NN
- `baum_notebook.ipynb` für den Entscheidungsbaum

Beide Notebooks enthalten Upload-Zellen für die CSV-Dateien, kommentierte Auswertungsschritte und den Download der fertigen `model.json`.

#### 3.3 Live-Test am ESP32

1. `model.json` im Dashboard hochladen.
2. Modus `Grenzen`, `k-NN` oder `Baum` auswählen.
3. Münzen einzeln durch den Sortieraufbau führen.
4. Vorhergesagte Klasse, Peak-Features und Servoaktion beobachten.
5. Fehlklassifikationen notieren.

### Diskussion / Quiz

1. Welche Methode war am genauesten?
2. Welche Methode war am einfachsten zu erklären?
3. Welche Methode reagierte am empfindlichsten auf unterschiedliche Geschwindigkeit oder Abstand?
4. Warum ist eine hohe Trainingsgenauigkeit allein nicht ausreichend?
5. Warum ist der Entscheidungsbaum nicht automatisch besser, wenn man ihn tiefer macht?
6. Welche Methode würdest du für eine robuste Sortiermaschine wählen und warum?

### Musterlösungen Abschnitt 3

1. Typischerweise k-NN oder Entscheidungsbaum — beide können bei ausreichend Trainingsdaten höhere Genauigkeit als manuelle Grenzen erreichen, weil sie die Grenzen datengetrieben optimieren statt sie von Hand zu schätzen.
2. Manuelle Grenzen und Entscheidungsbaum — beide lassen sich als klare Wenn-Dann-Regeln formulieren. k-NN ist schwerer zu erklären, weil die Entscheidung von allen Trainingspunkten abhängt.
3. k-NN reagiert am empfindlichsten, weil es keine abstrahierten Regeln lernt sondern direkt auf Trainingspunkte verweist. Ändert sich der Abstand oder die Geschwindigkeit systematisch, verschieben sich alle Features gleichmäßig — und k-NN findet dann die "falschen" Nachbarn.
4. Hohe Trainingsgenauigkeit kann durch Overfitting entstehen — das Modell hat die Trainingsdaten auswendig gelernt, generalisiert aber nicht auf neue Münzen oder leicht andere Bedingungen. Entscheidend ist die Testgenauigkeit auf Daten die beim Training nicht gesehen wurden.
5. Ein tieferer Baum kann jede Besonderheit der Trainingsdaten als Regel kodieren, auch zufälliges Rauschen. Das führt zu Overfitting: Trainingsgenauigkeit steigt, Testgenauigkeit sinkt. Ein flacher Baum (max_depth 2–3) erfasst nur die robusten Haupttrennlinien.
6. Für eine robuste Sortiermaschine empfiehlt sich der **Entscheidungsbaum mit kleiner Tiefe**: die Regeln sind nachvollziehbar und können bei Fehlern manuell angepasst werden, er ist recheneffizient auf dem ESP32, und er generalisiert bei begrenzten Trainingsdaten oft besser als k-NN.

---

## Abgabe

1. Ausgefüllte Tabellen aus Abschnitt 1.
2. Diagramme `delta_RP(d)` und `delta_L(d)`.
3. CSV-Dateien oder Feature-Tabelle aus Abschnitt 2.
4. Streudiagramm der Peak-Features.
5. Konfusionsmatrizen für manuelle Grenzen, k-NN und Entscheidungsbaum.
6. Kurze Reflexion: Welche Methode ist für diesen Aufbau am sinnvollsten?

---

## Technische Eckdaten

| Parameter | Wert |
|-----------|------|
| Mikrocontroller | ESP32 |
| Sensor | LDC1101 |
| Messwerte | `RP` und `L`, jeweils 16 Bit |
| Kommunikation Sensor | SPI, 4 MHz, Mode 0 |
| Referenztakt CLKIN | 4 MHz (≥ 4 × fSENSOR, Datenblatt Abschnitt 9.1.12) |
| Sensorfrequenz fSENSOR | ~0.88 MHz (gemessen: L_DATA ≈ 579 bei Luft) |
| Dashboard | HTTP auf `192.168.4.1`, WebSocket auf Port 81 |
| Datenerfassung | einstellbar 1–100 Hz |
| Export | Zeitreihe als CSV (Excel-kompatibel); Peak-Features als `peaks.csv` (HTTP `/peaks.csv`) |
| Sortieraktor | Servo |
