// Grove RGB LCD — Anzeige

static const uint8_t ICON_PERSON[8] = {0x0E, 0x0E, 0x0E, 0x00, 0x04, 0x0E, 0x1F, 0x1F};

void setupDisplay() {
  lcd.begin(16, 2);
  lcd.createChar(0, (uint8_t*)ICON_PERSON);
  lcd.home();
  lcd.setRGB(255, 255, 0);
  lcd.print("CoinSorter");
  lcd.setCursor(0, 1);
  lcd.print("Release ");
  lcd.print(APP_VERSION);
}

void showSsidScreen() {
  lcd.clear();
  lcd.setRGB(0, 100, 255);
  lcd.setCursor(0, 0); lcd.print(apSSID);
  lcd.setCursor(0, 1); lcd.print("PW:"); lcd.print(apPW);
}

static void printClientBadge(uint8_t count) {
  char tmp[3];
  if (count > 99) count = 99;
  lcd.setCursor(13, 0);
  lcd.write((uint8_t)0);
  snprintf(tmp, sizeof(tmp), "%2u", count);
  lcd.print(tmp);
}

void showDataScreen(uint8_t count) {
  char tmp[17];

  if (activeTab == TAB_RECORD) {
    lcd.setRGB(255, 180, 0);
    lcd.setCursor(0, 0);
    snprintf(tmp, sizeof(tmp), "Aufzeichnung ");
    lcd.print(tmp);
    printClientBadge(count);
    lcd.setCursor(0, 1);
    snprintf(tmp, sizeof(tmp), "RP%5u L%5u  ", lastRp, lastL);
    lcd.print(tmp);
    return;
  }

  if (activeTab == TAB_SORT) {
    lcd.setRGB(0, 180, 255);
    lcd.setCursor(0, 0);
    snprintf(tmp, sizeof(tmp), "Sortierung   ");
    lcd.print(tmp);
    printClientBadge(count);
    lcd.setCursor(0, 1);
    if (sortActionState == SORT_WAIT)
      snprintf(tmp, sizeof(tmp), "%-8s warten ", lastClass);
    else if (sortActionState == SORT_HOLD)
      snprintf(tmp, sizeof(tmp), "%-8s sort.  ", lastClass);
    else
      snprintf(tmp, sizeof(tmp), "%-10s %3d%c ", lastClass, displayAngle, (char)0xDF);
    lcd.print(tmp);
    return;
  }

  lcd.setRGB(0, 255, 80);
  lcd.setCursor(0, 0);
  snprintf(tmp, sizeof(tmp), "RP:%5u     ", lastRp);
  lcd.print(tmp);
  printClientBadge(count);

  lcd.setCursor(0, 1);
  snprintf(tmp, 12, "L :%5u   ", lastL);
  lcd.print(tmp);
  lcd.setCursor(11, 1);
  snprintf(tmp, 6, " %3d%c", displayAngle, (char)0xDF);
  lcd.print(tmp);
}
