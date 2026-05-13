// Klassifikation: manuelle Grenzen, k-NN und Entscheidungsbaum

static uint16_t u16diff(uint16_t a, uint16_t b) {
  return (a > b) ? (a - b) : 0;
}

static void featureVector(const PeakFeatures& f, float* x) {
  x[0] = (float)f.rp_min;
  x[1] = (float)f.l_min;
  x[2] = (float)f.delta_rp;
  x[3] = (float)f.delta_l;
  x[4] = (float)f.rp_avg;
  x[5] = (float)f.l_avg;
}

static void thresholdValues(const PeakFeatures& f, uint16_t* rp, uint16_t* l) {
  if (thresholdMode == THR_AVG) {
    *rp = f.rp_avg;
    *l = f.l_avg;
  } else if (thresholdMode == THR_DELTA) {
    *rp = f.delta_rp;
    *l = f.delta_l;
  } else {
    *rp = f.rp_min;
    *l = f.l_min;
  }
}

static float normFeature(float v, uint8_t i) {
  return (modelStd[i] > 0.0f) ? ((v - modelMean[i]) / modelStd[i]) : 0.0f;
}

static void normalizeFeatures(const float* in, float* out) {
  for (uint8_t i = 0; i < FEATURE_COUNT; i++) out[i] = normFeature(in[i], i);
}

// ── Schwellenwert ──
void classifyThreshold(const PeakFeatures& f, char* out, int* angle) {
  uint16_t rp, l;
  thresholdValues(f, &rp, &l);
  for (uint8_t i = 0; i < thrCount; i++) {
    if (rp >= thrClasses[i].rp_lo && rp <= thrClasses[i].rp_hi &&
        l  >= thrClasses[i].l_lo  && l  <= thrClasses[i].l_hi) {
      strlcpy(out, thrClasses[i].name, 16);
      *angle = thrClasses[i].angle;
      return;
    }
  }
  out[0] = '\0';
}

// ── k-NN auf Peak-Features ──
void classifyKnn(const PeakFeatures& f, char* out, int* angle) {
  if (knnCount == 0) { out[0] = '\0'; return; }

  float raw[FEATURE_COUNT], x[FEATURE_COUNT];
  featureVector(f, raw);
  normalizeFeatures(raw, x);

  float dists[MAX_SAMPLES];
  for (uint8_t i = 0; i < knnCount; i++) {
    float d = 0.0f;
    for (uint8_t j = 0; j < FEATURE_COUNT; j++) {
      float e = x[j] - knnSamples[i].x[j];
      d += e * e;
    }
    dists[i] = d;
  }

  char voteNames[MAX_CLASSES][16] = {};
  int  voteAngles[MAX_CLASSES] = {};
  int  votes[MAX_CLASSES] = {};
  uint8_t voteCount = 0;
  int k = min(knnK, (int)knnCount);

  for (int ki = 0; ki < k; ki++) {
    uint8_t best = 0;
    for (uint8_t i = 1; i < knnCount; i++)
      if (dists[i] < dists[best]) best = i;

    uint8_t slot = voteCount;
    for (uint8_t v = 0; v < voteCount; v++) {
      if (strcmp(voteNames[v], knnSamples[best].name) == 0) { slot = v; break; }
    }
    if (slot == voteCount && voteCount < MAX_CLASSES) {
      strlcpy(voteNames[slot], knnSamples[best].name, 16);
      voteAngles[slot] = knnSamples[best].angle;
      voteCount++;
    }
    if (slot < MAX_CLASSES) votes[slot]++;
    dists[best] = 1e30f;
  }

  if (voteCount == 0) { out[0] = '\0'; return; }
  uint8_t winner = 0;
  for (uint8_t i = 1; i < voteCount; i++)
    if (votes[i] > votes[winner]) winner = i;

  strlcpy(out, voteNames[winner], 16);
  *angle = voteAngles[winner];
}

// ── Entscheidungsbaum ──
void classifyTree(const PeakFeatures& f, char* out, int* angle) {
  if (treeCount == 0) { out[0] = '\0'; return; }

  float raw[FEATURE_COUNT], x[FEATURE_COUNT];
  featureVector(f, raw);
  normalizeFeatures(raw, x);

  int idx = 0;
  for (uint8_t depth = 0; depth < MAX_TREE_NODES; depth++) {
    if (idx < 0 || idx >= treeCount) break;
    TreeNode& n = treeNodes[idx];
    if (n.feature < 0) {
      strlcpy(out, n.label, 16);
      *angle = n.angle;
      return;
    }
    if (n.feature >= FEATURE_COUNT) break;
    idx = (x[n.feature] <= n.threshold) ? n.left : n.right;
  }
  out[0] = '\0';
}

static void applyDetectedClass(const char* detected, int targetAngle) {
  if (!detected || detected[0] == '\0') {
    strlcpy(lastClass, "?", sizeof(lastClass));
    classHoldCount = 0;
    return;
  }

  if (strcmp(detected, lastClass) == 0) {
    classHoldCount++;
  } else {
    strlcpy(lastClass, detected, sizeof(lastClass));
    classHoldCount = 1;
  }

  if (classHoldCount >= HOLD_N) {
    if (sortActionState == SORT_IDLE) {
      sortTargetAngle = constrain(targetAngle, 0, 180);
      sortActionState = SORT_WAIT;
      sortActionAtMs = millis() + SORT_WAIT_MS;
    }
  }
}

void cancelSortAction() {
  sortActionState = SORT_IDLE;
  sortActionAtMs = 0;
}

void processSortAction() {
  if (sortActionState == SORT_IDLE) return;

  uint32_t now = millis();
  if ((int32_t)(now - sortActionAtMs) < 0) return;

  if (sortActionState == SORT_WAIT) {
    aktuellerWinkel = constrain(sortTargetAngle, 0, 180);
    meinServo.write(aktuellerWinkel);
    sortActionState = SORT_HOLD;
    sortActionAtMs = now + SORT_HOLD_MS;
    return;
  }

  aktuellerWinkel = constrain(neutralWinkel, 0, 180);
  meinServo.write(aktuellerWinkel);
  sortActionState = SORT_IDLE;
}

bool classifyPeakAndActuate(const PeakFeatures& f) {
  if (sortMode == MODE_MANUAL) {
    strlcpy(lastClass, "manuell", sizeof(lastClass));
    if (peakLogActive && peakLogCount < MAX_PEAK_LOG) {
      peakLog[peakLogCount].f = f;
      strlcpy(peakLog[peakLogCount].label, "manuell", 16);
      peakLogCount++;
    }
    return false;
  }

  char detected[16] = "";
  int targetAngle = aktuellerWinkel;

  if (sortMode == MODE_THRESHOLD)
    classifyThreshold(f, detected, &targetAngle);
  else if (sortMode == MODE_KNN)
    classifyKnn(f, detected, &targetAngle);
  else if (sortMode == MODE_TREE)
    classifyTree(f, detected, &targetAngle);

  if (peakLogActive && peakLogCount < MAX_PEAK_LOG) {
    peakLog[peakLogCount].f = f;
    strlcpy(peakLog[peakLogCount].label, detected[0] ? detected : "?", 16);
    peakLogCount++;
  }

  applyDetectedClass(detected, targetAngle);
  return detected[0] != '\0';
}

void updatePeak(uint16_t rp, uint16_t l) {
  bool objectPresent = (rp < airThreshold);

  if (!objectPresent) {
    if (airSamples < 32) airSamples++;
    airRpRef = (uint16_t)(((uint32_t)airRpRef * (airSamples - 1) + rp) / airSamples);
    airLRef  = (uint16_t)(((uint32_t)airLRef  * (airSamples - 1) + l)  / airSamples);

    if (peakActive) {
      peakActive = false;
      lastPeak.rp_min = peakRpMin;
      lastPeak.l_min = peakLMin;
      lastPeak.delta_rp = u16diff(airRpRef, peakRpMin);
      lastPeak.delta_l = u16diff(airLRef, peakLMin);
      lastPeak.rp_avg = peakCount ? (uint16_t)(peakRpSum / peakCount) : peakRpMin;
      lastPeak.l_avg = peakCount ? (uint16_t)(peakLSum / peakCount) : peakLMin;
      lastPeak.n = peakCount;
      if (!peakClassified) peakClassified = classifyPeakAndActuate(lastPeak);
      peakRpMin = 65535;
      peakLMin = 65535;
      peakRpSum = 0;
      peakLSum = 0;
      peakCount = 0;
      peakClassified = false;
      peakWindowDone = false;
    } else if (sortMode != MODE_MANUAL && sortActionState == SORT_IDLE) {
      strlcpy(lastClass, "Luft", sizeof(lastClass));
      classHoldCount = 0;
    }
    return;
  }

  if (!peakActive) {
    peakActive = true;
    peakRpMin = rp;
    peakLMin = l;
    peakRpSum = rp;
    peakLSum = l;
    peakCount = 1;
    peakClassified = false;
    peakWindowDone = false;
    peakStartMs = millis();
  } else {
    if (!peakWindowDone && peakCount < 65535) {
      if (rp < peakRpMin) peakRpMin = rp;
      if (l < peakLMin) peakLMin = l;
      peakRpSum += rp;
      peakLSum += l;
      peakCount++;
    }
  }

  lastPeak.rp_min = peakRpMin;
  lastPeak.l_min = peakLMin;
  lastPeak.delta_rp = u16diff(airRpRef, peakRpMin);
  lastPeak.delta_l = u16diff(airLRef, peakLMin);
  lastPeak.rp_avg = peakCount ? (uint16_t)(peakRpSum / peakCount) : peakRpMin;
  lastPeak.l_avg = peakCount ? (uint16_t)(peakLSum / peakCount) : peakLMin;
  lastPeak.n = peakCount;

  uint32_t elapsed = millis() - peakStartMs;
  if (!peakWindowDone && elapsed >= thresholdWindowMs) {
    peakWindowDone = true;
  }

  if (peakWindowDone && !peakClassified && sortMode != MODE_MANUAL) {
    peakClassified = classifyPeakAndActuate(lastPeak);
  }
}

static void loadClassArray(JsonDocument& doc) {
  modelClassCount = 0;
  JsonArray classes = doc["classes"].as<JsonArray>();
  for (JsonObject cls : classes) {
    if (modelClassCount >= MAX_CLASSES) break;
    const char* name = cls["label"] | nullptr;
    if (!name) name = cls["name"] | "?";
    strlcpy(modelClasses[modelClassCount].name, name, 16);
    modelClasses[modelClassCount].angle = constrain(cls["angle"] | 90, 0, 180);
    modelClassCount++;
  }
}

static int angleForLabel(const char* label, int fallback) {
  for (uint8_t i = 0; i < modelClassCount; i++)
    if (strcmp(modelClasses[i].name, label) == 0) return modelClasses[i].angle;
  return fallback;
}

bool loadModel() {
  File f = LittleFS.open("/model.json", "r");
  if (!f) return false;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    Serial.printf("model.json Fehler: %s\n", err.c_str());
    return false;
  }

  for (uint8_t i = 0; i < FEATURE_COUNT; i++) {
    modelMean[i] = doc["knn"]["mean"][i] | doc["mean"][i] | 0.0f;
    modelStd[i]  = doc["knn"]["std"][i]  | doc["std"][i]  | 1.0f;
  }
  knnK = doc["knn"]["k"] | doc["k"] | 3;
  knnK = constrain(knnK, 1, MAX_SAMPLES);

  loadClassArray(doc);

  knnCount = 0;
  JsonArray samples = doc["knn"]["samples"].as<JsonArray>();
  if (samples.isNull()) samples = doc["samples"].as<JsonArray>();
  for (JsonObject s : samples) {
    if (knnCount >= MAX_SAMPLES) break;
    const char* name = s["label"] | nullptr;
    if (!name) name = s["name"] | "?";
    strlcpy(knnSamples[knnCount].name, name, 16);
    knnSamples[knnCount].angle = constrain(s["angle"] | angleForLabel(knnSamples[knnCount].name, 90), 0, 180);
    for (uint8_t i = 0; i < FEATURE_COUNT; i++) knnSamples[knnCount].x[i] = s["x"][i] | 0.0f;
    knnCount++;
  }

  treeCount = 0;
  JsonArray nodes = doc["tree"]["nodes"].as<JsonArray>();
  for (JsonObject n : nodes) {
    if (treeCount >= MAX_TREE_NODES) break;
    treeNodes[treeCount].feature = n["feature"] | -1;
    treeNodes[treeCount].threshold = n["threshold"] | 0.0f;
    treeNodes[treeCount].left = n["left"] | -1;
    treeNodes[treeCount].right = n["right"] | -1;
    const char* label = n["label"] | "";
    strlcpy(treeNodes[treeCount].label, label, 16);
    treeNodes[treeCount].angle = constrain(n["angle"] | angleForLabel(treeNodes[treeCount].label, 90), 0, 180);
    treeCount++;
  }

  Serial.printf("Modell geladen: %d Klassen, %d k-NN Samples, %d Baumknoten\n",
                modelClassCount, knnCount, treeCount);
  return true;
}

bool loadKnnModel() {
  return loadModel();
}
