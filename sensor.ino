// LDC1101 — SPI-Kommunikation

static inline uint8_t ldcCmd(uint8_t addr, bool read) {
  return (uint8_t)((read ? 0x80 : 0x00) | (addr & 0x7F));
}

uint8_t ldcRead8(uint8_t addr) {
  digitalWrite(PIN_CS, LOW);
  delayMicroseconds(2);
  SPI.transfer(ldcCmd(addr, true));
  uint8_t v = SPI.transfer(0x00);
  delayMicroseconds(2);
  digitalWrite(PIN_CS, HIGH);
  return v;
}

void ldcWrite8(uint8_t addr, uint8_t data) {
  digitalWrite(PIN_CS, LOW);
  delayMicroseconds(2);
  SPI.transfer(ldcCmd(addr, false));
  SPI.transfer(data);
  delayMicroseconds(2);
  digitalWrite(PIN_CS, HIGH);
}

uint16_t ldcRead16(uint8_t lsbAddr) {
  uint8_t lsb = ldcRead8(lsbAddr);
  uint8_t msb = ldcRead8(lsbAddr + 1);
  return (uint16_t)(msb << 8) | lsb;
}

void ldcReadRpL(uint16_t* rp, uint16_t* l) {
  digitalWrite(PIN_CS, LOW);
  SPI.transfer(ldcCmd(REG_RP_LSB, true));
  uint8_t rpLsb = SPI.transfer(0x00);
  uint8_t rpMsb = SPI.transfer(0x00);
  uint8_t lLsb  = SPI.transfer(0x00);
  uint8_t lMsb  = SPI.transfer(0x00);
  digitalWrite(PIN_CS, HIGH);

  *rp = (uint16_t)(rpMsb << 8) | rpLsb;
  *l  = (uint16_t)(lMsb << 8) | lLsb;
}

void resetLDC() {
  SPI.beginTransaction(LDC_SPI_SETTINGS);
  ldcWrite8(REG_START_CFG, 0x01);  // sleep
  delay(10);
  ldcWrite8(REG_RP_SET,   LDC_RP_SET);
  ldcWrite8(REG_TC1,      LDC_TC1);
  ldcWrite8(REG_TC2,      LDC_TC2);
  ldcWrite8(REG_DIG_CONF, LDC_DIG_CONF);
  ldcWrite8(REG_START_CFG, 0x00);  // active
  delay(20);
  SPI.endTransaction();
}
