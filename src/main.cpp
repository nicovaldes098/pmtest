#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

// Credenciales WiFi
const char* ssid = "IngenieriaPM";
const char* password = "ingenieriatech";

// Configuración de ThingsBoard
const char* mqtt_server = "iot.redelocker.com";  // Cambia si usas otro servidor
const char* token = "yo0hjotu99whp5du8vpr";  // Token de acceso del dispositivo
const int mqtt_port = 1883;      // Puerto MQTT estándar sin SSL

WiFiClient espClient;
PubSubClient client(espClient);

// Función para actualizar el firmware desde una URL
void updateFirmware(const char* firmwareUrl) {
    Serial.println("Iniciando actualización OTA desde: " + String(firmwareUrl));
    
    WiFiClient client;
    ESPhttpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    t_httpUpdate_return result = ESPhttpUpdate.update(client, firmwareUrl);

    switch (result) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("Error en la actualización OTA: %s\n", ESPhttpUpdate.getLastErrorString().c_str());
            break;
        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("No hay nuevas actualizaciones.");
            break;
        case HTTP_UPDATE_OK:
            Serial.println("Actualización exitosa, reiniciando...");
            ESP.restart();
            break;
    }
}

// Callback para recibir atributos compartidos desde ThingsBoard
void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Mensaje recibido [");
    Serial.print(topic);
    Serial.print("]: ");
    
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println(message);

    // Busca la clave "fw_url" en el mensaje recibido
    if (message.indexOf("fw_url") != -1) {
        int start = message.indexOf("fw_url") + 9; // Salta "fw_url":""
        int end = message.indexOf("\"", start);
        String firmwareUrl = message.substring(start, end);
        Serial.println("URL de firmware recibida: " + firmwareUrl);
        
        // Llamar a la función de actualización
        updateFirmware(firmwareUrl.c_str());
    }
}

// Conectar a WiFi
void setup_wifi() {
    Serial.print("Conectando a ");
    Serial.println(ssid);
    
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConexión WiFi establecida.");
}

// Conectar al servidor de ThingsBoard
void reconnect() {
    while (!client.connected()) {
        Serial.print("Intentando conectar a ThingsBoard... ");
        if (client.connect("ESP8266", token, "")) {
            Serial.println("Conectado!");
            client.subscribe("v1/devices/me/attributes");  // Suscribirse a atributos compartidos
        } else {
            Serial.print("Error de conexión, rc=");
            Serial.print(client.state());
            Serial.println(" Intentando de nuevo en 5 segundos...");
            delay(5000);
        }
    }
}


void setup() {
    Serial.begin(115200);
    setup_wifi();

    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);

    reconnect();

    // Solicita la URL del firmware en ThingsBoard
    client.publish("v1/devices/me/attributes/request", "{\"clientKeys\":\"fw_url\"}");
    Serial.println("Solicitud enviada para obtener la URL del firmware...");
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
}
