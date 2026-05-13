# %% [markdown]
# # Abschnitt 3: Klassifikation von Muenz-Peaks
#
# Dieses Skript ist als Notebook-Vorlage gedacht. In VS Code, Jupyter oder
# PyCharm koennen die `# %%`-Zellen einzeln ausgefuehrt werden.
#
# Erwartung:
# - CSV-Dateien aus dem Dashboard liegen im Ordner `messdaten/`
# - Dateinamen enthalten das Label, z. B. `objekt_a.csv` oder `stahl_scheibe.csv`
# - CSV-Spalten: `Zeit_ms;RP;L`

# %%
from __future__ import annotations

import json
from pathlib import Path

import numpy as np
import pandas as pd
from sklearn.metrics import ConfusionMatrixDisplay, classification_report
from sklearn.model_selection import train_test_split
from sklearn.neighbors import KNeighborsClassifier
from sklearn.preprocessing import StandardScaler
from sklearn.tree import DecisionTreeClassifier, export_text


DATA_DIR = Path("messdaten")
FEATURES = ["rp_min", "l_min", "delta_rp", "delta_l"]

# Servo-Zielwinkel fuer den Live-Test. Bei Bedarf an den Aufbau anpassen.
CLASS_ANGLES = {
    "objekt_a": 25,
    "objekt_b": 45,
    "objekt_c": 65,
    "objekt_d": 85,
    "objekt_e": 105,
    "objekt_f": 125,
}


# %%
def label_from_filename(path: Path) -> str:
    stem = path.stem.lower()
    for token in ("muenze_", "munze_", "coin_"):
        stem = stem.replace(token, "")
    return stem


def read_dashboard_csv(path: Path) -> pd.DataFrame:
    # Dashboard-CSV ist Excel-freundlich mit Semikolon getrennt. Fallback fuer alte Komma-CSVs.
    df = pd.read_csv(path, sep=";")
    if len(df.columns) == 1:
        df = pd.read_csv(path)
    return df.rename(columns=str.strip)


def extract_peaks(df: pd.DataFrame, label: str, min_gap: int = 3) -> list[dict]:
    rp = df["RP"].to_numpy()
    l_val = df["L"].to_numpy()

    rp_air = float(np.percentile(rp, 90))
    l_air = float(np.percentile(l_val, 90))
    noise = max(float(np.std(rp[: min(len(rp), 50)])), 20.0)
    threshold = rp_air - 4.0 * noise

    peaks: list[dict] = []
    active = False
    start = 0
    gap = 0

    for i, value in enumerate(rp):
        present = value < threshold
        if present and not active:
            active = True
            start = i
            gap = 0
        elif active and not present:
            gap += 1
            if gap >= min_gap:
                end = max(start + 1, i - gap + 1)
                seg_rp = rp[start:end]
                seg_l = l_val[start:end]
                if len(seg_rp) >= 3:
                    rp_min = float(np.min(seg_rp))
                    l_min = float(np.min(seg_l))
                    peaks.append(
                        {
                            "label": label,
                            "rp_min": rp_min,
                            "l_min": l_min,
                            "delta_rp": rp_air - rp_min,
                            "delta_l": l_air - l_min,
                        }
                    )
                active = False
        elif active:
            gap = 0

    return peaks


# %%
rows = []
for csv_path in sorted(DATA_DIR.glob("*.csv")):
    label = label_from_filename(csv_path)
    df = read_dashboard_csv(csv_path)
    rows.extend(extract_peaks(df, label))

data = pd.DataFrame(rows)
print(data.head())
print(f"{len(data)} Peaks aus {data['label'].nunique() if not data.empty else 0} Klassen")


# %%
X = data[FEATURES].to_numpy()
y = data["label"].to_numpy()

X_train, X_test, y_train, y_test = train_test_split(
    X, y, test_size=0.25, stratify=y, random_state=42
)

scaler = StandardScaler()
X_train_s = scaler.fit_transform(X_train)
X_test_s = scaler.transform(X_test)


# %% [markdown]
# ## Methode 1: manuelle Grenzen
#
# Diese Beispielregeln muessen mit dem Excel-Streudiagramm an den eigenen Aufbau
# angepasst werden.

# %%
def manual_predict_one(row: np.ndarray) -> str:
    rp_min, l_min, delta_rp, delta_l = row
    if delta_l > 80:
        return "stahl"
    if delta_rp > 2500 and delta_l < 80:
        return "nordic"
    return "bimetall"


manual_pred = np.array([manual_predict_one(row) for row in X_test])
print("Manuelle Grenzen")
print(classification_report(y_test, manual_pred, zero_division=0))


# %% [markdown]
# ## Methode 2: k-NN

# %%
for k in (1, 3, 5):
    knn = KNeighborsClassifier(n_neighbors=k)
    knn.fit(X_train_s, y_train)
    print(f"k={k}: {knn.score(X_test_s, y_test):.3f}")

knn = KNeighborsClassifier(n_neighbors=3)
knn.fit(X_train_s, y_train)
knn_pred = knn.predict(X_test_s)
print(classification_report(y_test, knn_pred, zero_division=0))
ConfusionMatrixDisplay.from_predictions(y_test, knn_pred)


# %% [markdown]
# ## Methode 3: kleiner Entscheidungsbaum

# %%
tree = DecisionTreeClassifier(max_depth=3, random_state=42)
tree.fit(X_train_s, y_train)
tree_pred = tree.predict(X_test_s)
print(export_text(tree, feature_names=FEATURES))
print(classification_report(y_test, tree_pred, zero_division=0))
ConfusionMatrixDisplay.from_predictions(y_test, tree_pred)


# %% [markdown]
# ## Export fuer ESP32

# %%
def export_tree_nodes(clf: DecisionTreeClassifier) -> list[dict]:
    tree_ = clf.tree_
    labels = clf.classes_
    nodes = []
    for i in range(tree_.node_count):
        if tree_.children_left[i] == tree_.children_right[i]:
            label = labels[int(np.argmax(tree_.value[i][0]))]
            nodes.append(
                {
                    "feature": -1,
                    "threshold": 0,
                    "left": -1,
                    "right": -1,
                    "label": str(label),
                    "angle": CLASS_ANGLES.get(str(label), 90),
                }
            )
        else:
            nodes.append(
                {
                    "feature": int(tree_.feature[i]),
                    "threshold": float(tree_.threshold[i]),
                    "left": int(tree_.children_left[i]),
                    "right": int(tree_.children_right[i]),
                    "label": "",
                    "angle": 90,
                }
            )
    return nodes


model = {
    "version": 2,
    "features": FEATURES,
    "classes": [
        {"label": label, "angle": CLASS_ANGLES.get(str(label), 90)}
        for label in sorted(set(y))
    ],
    "manual": {"rules": []},
    "knn": {
        "k": 3,
        "mean": scaler.mean_.tolist(),
        "std": scaler.scale_.tolist(),
        "samples": [
            {
                "label": str(label),
                "angle": CLASS_ANGLES.get(str(label), 90),
                "x": x.tolist(),
            }
            for x, label in zip(X_train_s, y_train)
        ],
    },
    "tree": {"nodes": export_tree_nodes(tree)},
}

Path("model.json").write_text(json.dumps(model, indent=2), encoding="utf-8")
print("model.json geschrieben")
