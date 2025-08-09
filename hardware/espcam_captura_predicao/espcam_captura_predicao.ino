#include "esp_camera.h" //esp32 v1.0.6
#include <WiFi.h>
#include <time.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//#define CAMERA_MODEL_AI_THINKER
#define CAMERA_MODEL_ESP32S3_EYE
//#define FLASH_LED_PIN 4
#include "camera_pins.h"

#define BUTTON_PIN 21  // Botão (um lado no GPIO21, outro no GND)
#define OLED_SDA 2     // SDA do OLED
#define OLED_SCL 14    // SCL do OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define BUTTON_PIN 21

const char* ssid = "visao_computacional";
const char* password = "rede_visao_comp123*";
String serverUrl = "172.22.45.4";
int serverPort = 5001;

const unsigned long captureInterval = 1 * 30 * 1000; 
unsigned long previousMillis = 0;

String espCamID = "01";

// === CONTROLE DO BOTÃO COM DEBOUNCE ===
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;
bool lastStableState = HIGH;

void oledMessage(const String &msg) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.println(msg);
  display.display();
}

void setup() {
    Serial.begin(115200);
    
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Inicializa I2C e OLED
    Wire.begin(OLED_SDA, OLED_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      Serial.println("Falha ao inicializar OLED");
      for (;;);
    }
    oledMessage("Iniciando...");

    //pinMode(FLASH_LED_PIN, OUTPUT);
    //digitalWrite(FLASH_LED_PIN, LOW);

    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    if (psramFound()) {
        config.frame_size = FRAMESIZE_VGA;
        config.jpeg_quality = 15;          
        config.fb_count = 1;
    } else {
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = 10;
        config.fb_count = 1;
    }

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Erro ao inicializar a câmera: 0x%x\n", err);
        ESP.restart();
    }

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        oledMessage("Conectando...");
    }
    Serial.println("\nConectado ao WiFi");
    oledMessage("Conectado ao WiFi");
}

String getTimestamp() {
    configTime(-3 * 3600, 0, "pool.ntp.org");
    struct tm timeInfo;
    if (!getLocalTime(&timeInfo)) {
        Serial.println("Falha ao obter hora.");
        return "unknown-time";
    }

    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%d-%m-%Y_%H-%M-%S", &timeInfo);
    return String(timestamp);
}

bool sendImageAndGetPrediction(camera_fb_t* fb) {
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client;
        if (!client.connect(serverUrl.c_str(), 8000)) {  // Porta da API FastAPI
            Serial.println("Erro ao conectar ao servidor");
            return false;
        }

        String filename = espCamID + "_" + String(millis()) + ".jpg";
        String boundary = "----WebKitFormBoundary";
        String bodyStart = "--" + boundary + "\r\n";
        bodyStart += "Content-Disposition: form-data; name=\"file\"; filename=\"" + filename + "\"\r\n";
        bodyStart += "Content-Type: image/jpeg\r\n\r\n";
        String bodyEnd = "\r\n--" + boundary + "--\r\n";

        size_t contentLength = bodyStart.length() + fb->len + bodyEnd.length();

        // Cabeçalhos HTTP
        client.printf("POST /predict HTTP/1.1\r\n");
        client.printf("Host: %s\r\n", serverUrl.c_str());
        client.printf("Content-Type: multipart/form-data; boundary=%s\r\n", boundary.c_str());
        client.printf("Content-Length: %d\r\n\r\n", contentLength);

        // Corpo
        client.print(bodyStart);
        client.write(fb->buf, fb->len);
        client.print(bodyEnd);

        // Espera resposta
        String response;
        unsigned long timeout = millis();
        while (client.connected() && millis() - timeout < 5000) {
            while (client.available()) {
                char c = client.read();
                response += c;
                timeout = millis();
            }
        }
        client.stop();

        Serial.println("Resposta bruta do servidor:");
        Serial.println(response);

        // Busca o JSON na resposta
        int jsonStart = response.indexOf("{");
        int jsonEnd = response.lastIndexOf("}");
        if (jsonStart != -1 && jsonEnd != -1) {
            String jsonString = response.substring(jsonStart, jsonEnd + 1);
            Serial.println("JSON extraído:");
            Serial.println(jsonString);

            // Extração simples (sem parser JSON avançado)
            int classPos = jsonString.indexOf("\"predicted_class\"");
            int confPos = jsonString.indexOf("\"confidence\"");
            if (classPos != -1 && confPos != -1) {
                String predictedClass = jsonString.substring(jsonString.indexOf(":", classPos) + 2, jsonString.indexOf("\"", classPos + 20));
                String confidence = jsonString.substring(jsonString.indexOf(":", confPos) + 1, jsonString.indexOf("}", confPos));

                predictedClass.trim();
                confidence.trim();

                String displayMsg = "Classe: " + predictedClass + "\nConf: " + confidence + "%";
                oledMessage(displayMsg);
                delay(10000);
                return true;
            }
        }

        oledMessage("Erro resp. servidor");
        return false;
    } else {
        Serial.println("WiFi desconectado.");
        oledMessage("WiFi desconectado");
    }
    return false;
}


void reconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    oledMessage("Reconectando WiFi...");
    WiFi.disconnect();  // Desconecta, caso esteja preso
    WiFi.begin(ssid, password);
    unsigned long startAttemptTime = millis();

    // Espera até 5 segundos pela reconexão
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 5000) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi reconectado.");
      oledMessage("WiFi reconectado");
    } else {
      Serial.println("\nFalha ao reconectar WiFi.");
      oledMessage("Falha WiFi");
    }
  }
}

void loop() {
  reconnectWiFi();

  oledMessage("Captura disponivel");
  bool currentReading = digitalRead(BUTTON_PIN);

  if (currentReading != lastStableState && (millis() - lastDebounceTime) > debounceDelay) {
    lastDebounceTime = millis();
    lastStableState = currentReading;

    if (currentReading == LOW) {
      Serial.println("Capturando...");
      oledMessage("Capturando...");

      // DESCARTA o primeiro frame para garantir que pegue um novo
      camera_fb_t* fb = esp_camera_fb_get();
      if (fb) {
        esp_camera_fb_return(fb);  // libera o antigo
        delay(100); // tempo para novo frame ser capturado (opcional)
      }

      // Captura o frame real
      fb = esp_camera_fb_get();
      if (!fb) {
        Serial.println("Erro ao capturar imagem");
        oledMessage("Erro captura");
        return;
      }

      bool enviado = sendImageAndGetPrediction(fb);
      esp_camera_fb_return(fb);  // Libera SEMPRE

      if (enviado) {
        oledMessage("Imagem enviada");
      } else {
        oledMessage("Erro ao enviar");
        Serial.println("Imagem não foi enviada.");
      }

      delay(1000);
    }
  }
}