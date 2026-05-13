# Excel-Vorlage fuer Abschnitt 2

Diese Datei beschreibt die Arbeitsblaetter der Excel-Vorlage. Sie ist absichtlich textbasiert, damit die Struktur im Repository nachvollziehbar bleibt. Daraus kann bei Bedarf eine echte `Auswertung_Muenzsensor.xlsx` erstellt werden.

## Blatt `Rohdaten`

CSV aus dem Dashboard einfuegen. Das Dashboard exportiert mit Semikolon-Trennung.

| Spalte | Inhalt |
|--------|--------|
| A | `Zeit_ms` |
| B | `RP` |
| C | `L` |
| D | `Label` manuell eintragen, z. B. `objekt_a` oder `stahl_scheibe` |
| E | `Peak_ID` manuell oder per Markierung eintragen |

## Blatt `Peak-Auswertung`

Eine Zeile pro Peak.

| Spalte | Formel / Inhalt |
|--------|------------------|
| A `Label` | Muenzklasse |
| B `Peak_ID` | laufende Nummer |
| C `rp_luft` | Luftreferenz aus Abschnitt 1 |
| D `l_luft` | Luftreferenz aus Abschnitt 1 |
| E `rp_min` | `=MINWENNS(Rohdaten!B:B;Rohdaten!E:E;B2)` |
| F `l_min` | `=MINWENNS(Rohdaten!C:C;Rohdaten!E:E;B2)` |
| G `delta_rp` | `=C2-E2` |
| H `delta_l` | `=D2-F2` |

Falls `MINWENNS`/`MAXWENNS` nicht verfuegbar sind, kann der Peak-Bereich auch direkt markiert werden, z. B. `=MIN(Rohdaten!B20:B55)`.

## Blatt `Diagramme`

Erstelle diese Diagramme:

1. Zeitdiagramm: `Zeit_ms` auf x-Achse, `RP` und `L` als Linien.
2. Streudiagramm: `delta_rp` auf x-Achse, `delta_l` auf y-Achse, Punkte nach `Label` faerben.
3. Optional: `rp_min` gegen `l_min`.

## Blatt `Grenzen`

Hier werden manuelle Entscheidungsregeln dokumentiert.

| Regel | Bedingung | Klasse |
|-------|-----------|--------|
| 1 | z. B. `delta_l > ...` | Stahlgruppe |
| 2 | z. B. `delta_rp > ...` | stark leitfähige Gruppe |
| 3 | sonst | übrige Objektgruppe |

Diese Regeln werden in Abschnitt 3 mit k-NN und Entscheidungsbaum verglichen.
