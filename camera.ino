#include <WiFi.h>
#include <WebServer.h>
#include <esp_camera.h>
#include <ctime>
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

const char *ssid = "CAMERA";
const char *password = "password";

#define LED_GPIO_NUM 2

WebServer server(80);

void startCamera() {
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 2;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.println("Errore nell'inizializzare la fotocamera");
    return;
  }

  Serial.println("Fotocamera inizializzata correttamente!");
}

void handleJPGStream() {
  WiFiClient client = server.client();

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println();

  client.println("<html>");
  client.println("<head>");
  client.println("<title>Flusso Camera</title>");
  client.println("<style>");
  client.println("body { font-family: Arial; text-align: center; letter-spacing:-0.5px}");
  client.println("img { display: block; margin: 10px auto; max-width: 95%; }");
  client.println("button { padding: 10px 20px; font-size: 30px; cursor: pointer; }");
  client.println("</style>");
  client.println("</head>");
  client.println("<body>");
  client.println("<h1>Fotocamera</h1>");
  client.println("<img src='/stream-mjpeg' alt='Streaming video' />");
  client.println("<form action='/capture' method='get'>");
  client.println("<button type='submit'>PHOTO</button>");
  client.println("</form>");
  client.println("</body>");
  client.println("</html>");
}

void handleMJPEGStream() {
  WiFiClient client = server.client();

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println();

  while (client.connected()) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Errore nell'acquisizione dell'immagine");
      break;
    }

    client.println("--frame");
    client.println("Content-Type: image/jpeg");
    client.println("Content-Length: " + String(fb->len));
    client.println();
        client.write(fb->buf, fb->len);
    client.println();
    esp_camera_fb_return(fb);

    delay(100);
  }
}


void handleCapture() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Errore: Impossibile catturare l'immagine.");
    return;
  }

  WiFiClient client = server.client();
  if (!client) {
    esp_camera_fb_return(fb);
    return;
  }

  time_t now = time(nullptr);
  struct tm *timeinfo = localtime(&now);
  char filename[64];
  strftime(filename, sizeof(filename), "gwetanocamera%Y%m%d_%H%M%S.jpg", timeinfo);

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: image/jpeg");
  client.println("Content-Disposition: attachment; filename=" + String(filename));
  client.println("Content-Length: " + String(fb->len));
  client.println();
  client.write(fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

// Funzione di configurazione
void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  Serial.println("Access Point Avviato");
  Serial.print("IP dell'Access Point: ");
  Serial.println(WiFi.softAPIP());
  startCamera();
  server.on("/stream", HTTP_GET, handleJPGStream);
  server.on("/stream-mjpeg", HTTP_GET, handleMJPEGStream);
  server.on("/capture", HTTP_GET, handleCapture);
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html",
                "<html>"
                "<body>"
                "<h1><a href='/stream'><button>CAM</button></a></h1>"
                "</body>"
                "</html>");
  });
  server.begin();
}

void loop() {
  server.handleClient();
}
