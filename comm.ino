// HTTP-Routen + WebSocket-Handler + Broadcast

void setupRoutes() {
  // Dashboard
  server.on("/", HTTP_GET, []() {
    File f = LittleFS.open("/index.html", "r");
    if (f) { server.streamFile(f, "text/html"); f.close(); }
    else    { server.send_P(200, "text/html", UPLOAD_PAGE); }
  });

  // Dashboard hochladen
  server.on("/update", HTTP_POST, []() {
    server.send(200, "text/plain", "OK");
  }, []() {
    static File uf;
    HTTPUpload& up = server.upload();
    if      (up.status == UPLOAD_FILE_START) { uf = LittleFS.open("/index.html", "w"); }
    else if (up.status == UPLOAD_FILE_WRITE) { if (uf) uf.write(up.buf, up.currentSize); }
    else if (up.status == UPLOAD_FILE_END)   { if (uf) uf.close(); }
  });

  // k-NN Modell hochladen
  server.on("/model", HTTP_POST, []() {
    if (loadKnnModel())
      server.send(200, "text/plain", "OK");
    else
      server.send(500, "text/plain", "Fehler beim Laden");
  }, []() {
    static File mf;
    HTTPUpload& up = server.upload();
    if      (up.status == UPLOAD_FILE_START) { mf = LittleFS.open("/model.json", "w"); }
    else if (up.status == UPLOAD_FILE_WRITE) { if (mf) mf.write(up.buf, up.currentSize); }
    else if (up.status == UPLOAD_FILE_END)   { if (mf) mf.close(); }
  });

  // Peak-Feature-Export
  server.on("/peaks.csv", HTTP_GET, []() {
    String s = "label;rp_min;l_min;delta_rp;delta_l;rp_avg;l_avg\r\n";
    for (uint8_t i = 0; i < peakLogCount; i++) {
      const PeakFeatures& f = peakLog[i].f;
      s += String(peakLog[i].label) + ';' +
           f.rp_min  + ';' + f.l_min   + ';' +
           f.delta_rp + ';' + f.delta_l + ';' +
           f.rp_avg  + ';' + f.l_avg   + "\r\n";
    }
    server.send(200, "text/csv; charset=utf-8", s);
  });

  server.begin();
}

void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t len) {
  if (type != WStype_TEXT) return;
  String msg = String((char*)payload);

  if (msg.startsWith("SET_DISP:")) {
    displayAngle = constrain(msg.substring(9).toInt(), -90, 90);

  } else if (msg == "SET_TAB:sensor") {
    activeTab = TAB_SENSOR;

  } else if (msg == "SET_TAB:record") {
    activeTab = TAB_RECORD;

  } else if (msg == "SET_TAB:sort") {
    activeTab = TAB_SORT;

  } else if (msg.startsWith("SET_ANG:")) {
    cancelSortAction();
    aktuellerWinkel = constrain(msg.substring(8).toInt(), 0, 180);
    meinServo.write(aktuellerWinkel);

  } else if (msg.startsWith("SET_HOME:")) {
    neutralWinkel = constrain(msg.substring(9).toInt(), 0, 180);

  } else if (msg.startsWith("SET_FREQ:")) {
    int hz = msg.substring(9).toInt();
    if (hz > 0 && hz <= 100) sendIntervalMs = 1000 / hz;

  } else if (msg == "SET_MODE:manual") {
    cancelSortAction();
    sortMode = MODE_MANUAL;
    strlcpy(lastClass, "manuell", sizeof(lastClass));

  } else if (msg == "SET_MODE:threshold") {
    sortMode = MODE_THRESHOLD;

  } else if (msg == "SET_THR_MODE:peak") {
    thresholdMode = THR_PEAK;

  } else if (msg == "SET_THR_MODE:avg") {
    thresholdMode = THR_AVG;

  } else if (msg == "SET_THR_MODE:delta") {
    thresholdMode = THR_DELTA;

  } else if (msg.startsWith("SET_THR_WIN:")) {
    thresholdWindowMs = (uint16_t)constrain(msg.substring(12).toInt(), 20, 3000);

  } else if (msg == "SET_MODE:knn") {
    sortMode = MODE_KNN;
    if (knnCount == 0) loadModel();

  } else if (msg == "SET_MODE:tree") {
    sortMode = MODE_TREE;
    if (treeCount == 0) loadModel();

  } else if (msg.startsWith("SET_AIR:")) {
    airThreshold = (uint16_t)constrain(msg.substring(8).toInt(), 1, 65535);

  } else if (msg.startsWith("SET_THR:")) {
    // Format: SET_THR:name:rp_lo:rp_hi:l_lo:l_hi:angle
    if (thrCount >= MAX_CLASSES) return;
    String s = msg.substring(8);
    int p[5];
    p[0] = s.indexOf(':');
    for (int i = 1; i < 5; i++) p[i] = s.indexOf(':', p[i-1] + 1);
    if (p[4] < 0) return;
    ThrClass& c = thrClasses[thrCount++];
    s.substring(0,        p[0]).toCharArray(c.name, 16);
    c.rp_lo = (uint16_t)constrain(s.substring(p[0]+1, p[1]).toInt(), 0, 65535);
    c.rp_hi = (uint16_t)constrain(s.substring(p[1]+1, p[2]).toInt(), 0, 65535);
    c.l_lo  = (uint16_t)constrain(s.substring(p[2]+1, p[3]).toInt(), 0, 65535);
    c.l_hi  = (uint16_t)constrain(s.substring(p[3]+1, p[4]).toInt(), 0, 65535);
    if (c.rp_lo > c.rp_hi) { uint16_t t = c.rp_lo; c.rp_lo = c.rp_hi; c.rp_hi = t; }
    if (c.l_lo > c.l_hi)   { uint16_t t = c.l_lo;  c.l_lo  = c.l_hi;  c.l_hi  = t; }
    c.angle = constrain(s.substring(p[4]+1).toInt(), 0, 180);

  } else if (msg == "CLR_THR") {
    thrCount = 0;
    strlcpy(lastClass, "?", sizeof(lastClass));

  } else if (msg == "PEAK_LOG:start") {
    peakLogActive = true;

  } else if (msg == "PEAK_LOG:stop") {
    peakLogActive = false;

  } else if (msg == "PEAK_LOG:clear") {
    peakLogCount = 0;
    peakLogActive = false;
  }
}

void broadcastSensor(uint16_t rp, uint16_t l) {
  char buf[440];
  snprintf(buf, sizeof(buf),
    "{\"t\":%lu,\"angle\":%d,\"rp\":%u,\"l\":%u,\"class\":\"%s\",\"mode\":%u,\"air\":%u,"
    "\"thrMode\":%u,\"thrWin\":%u,\"knn\":%u,\"tree\":%u,"
    "\"pkLog\":%u,\"pkLogAct\":%u,"
    "\"peak\":{\"rp_min\":%u,\"l_min\":%u,\"delta_rp\":%u,\"delta_l\":%u,"
    "\"rp_avg\":%u,\"l_avg\":%u,\"n\":%u}}",
    (unsigned long)millis(),
    aktuellerWinkel, rp, l, lastClass, (uint8_t)sortMode, airThreshold,
    (uint8_t)thresholdMode, thresholdWindowMs, knnCount, treeCount,
    (uint8_t)peakLogCount, (uint8_t)peakLogActive,
    lastPeak.rp_min, lastPeak.l_min, lastPeak.delta_rp, lastPeak.delta_l,
    lastPeak.rp_avg, lastPeak.l_avg, lastPeak.n);
  ws.broadcastTXT(buf);
}
