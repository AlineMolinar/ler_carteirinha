#include <Adafruit_PN532.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include <SPI.h>

// definicao dos pinos 
#define PN532_SCK  (19)
#define PN532_MISO (11)
#define PN532_MOSI (12)
#define PN532_SS   (13)

Adafruit_PN532 sensor(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
    40, 41, 39, 42, 45, 48, 47, 21, 14, 5, 6, 7, 15, 16, 4, 8, 3, 46, 9, 1, 0, 210, 30, 16, 0, 22, 13, 10,  1, 12000000    
);

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(800, 480, bus);


static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;
static lv_disp_drv_t disp_drv;
lv_obj_t* texto;

// atualizacao da tela  
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  //desenhar a tela no display
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);

  lv_disp_flush_ready(disp);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(2, OUTPUT); 
  digitalWrite(2, HIGH);
  gfx->begin();
  lv_init();
  
  // alocar memoria para o buffer
  disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * 800 * 40, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  
  if (!disp_draw_buf) {
     Serial.println("ERRO: PSRAM nao alocada! Verifique Tools > PSRAM");
     while(1);
  }
 
  // configura o driver do display
  lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, 800 * 40);
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 800;
  disp_drv.ver_res = 480;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // nfc
  SPI.begin(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
  sensor.begin();
  sensor.SAMConfig();
  sensor.setPassiveActivationRetries(0xFF); 
  uint32_t versiondata = sensor.getFirmwareVersion();
  Serial.print("Firmware check: "); Serial.println((versiondata>>24) & 0xFF, HEX);
  Serial.println("NFC pronto");

  //config da interface 
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x111184), LV_PART_MAIN);

  texto = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(texto, &lv_font_montserrat_28, LV_PART_MAIN);
  lv_obj_set_style_text_color(texto, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  
  lv_label_set_text(texto, "Aproxime a\ncarteirinha");
  lv_obj_set_style_text_align(texto, LV_TEXT_ALIGN_CENTER, 0);

  lv_obj_set_size(texto, 400, 100); 

  lv_obj_set_pos(texto, 50, 90);
 
}

void loop() {
  lv_timer_handler(); 

  static unsigned long tempoNFC = 0;
  if (millis() - tempoNFC > 300) { 
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 }; 
    uint8_t uidLength;

    //ler o cartao
    bool leu = sensor.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100);

  //char uidStr[50] = "";
   if (leu) {
      Serial.println("\nCARTAO IDENTIFICADO");
      
      Serial.print("UID Bytes: ");
      for (uint8_t i = 0; i < uidLength; i++) {
        Serial.print(" 0x");
        if (uid[i] < 0x10) Serial.print("0"); 
        Serial.print(uid[i], HEX);

        // char buffer[5];
        // sprintf(buffer, "%02X", uid[i]);
        // strcat(uidStr, buffer);

      }
      Serial.println(); 

      lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x00A32A), LV_PART_MAIN);
      lv_label_set_text(texto, "Carteirinha \ndetectada!");
      //lv_label_set_text(texto, uidStr);
      lv_timer_handler();
      
      delay(2000);

      lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x111184), LV_PART_MAIN);
      lv_label_set_text(texto, "Aproxime a\ncarteirinha");
    }
    tempoNFC = millis();
  }
}
