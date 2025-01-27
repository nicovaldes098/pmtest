#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>

// Credenciales WiFi
const char* ssid = "IngenieriaPM";
const char* password = "ingenieriatech";

// Configuración de ThingsBoard MQTT
const char* mqtt_server = "iot.redelocker.com";
const int mqtt_port = 1883;
const char* token = "yo0hjotu99whp5du8vpr";

// Last telemetry time
static unsigned long lastTelemetry = 0;

WiFiClient espClient;
PubSubClient client(espClient);

// Función para conectar a WiFi
void setup_wifi() {
    Serial.print("Conectando a ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi conectado, IP: " + WiFi.localIP().toString());
}

// Función para conectar a ThingsBoard
void reconnect() {
    while (!client.connected()) {
        Serial.print("Conectando a ThingsBoard...");
        if (client.connect("ESP8266", token, "")) {
            Serial.println("Conectado!");
            client.subscribe("v1/devices/me/attributes");  // Escuchar atributos compartidos
        } else {
            Serial.print("Error, rc=");
            Serial.print(client.state());
            Serial.println(" Reintentando en 5 segundos...");
            delay(5000);
        }
    }
}

// Configuración de OTA
void setupOTA() {
    ArduinoOTA.setHostname("ESP8266-OTA");
    ArduinoOTA.setPassword("admin");  // Cambiar por una contraseña segura

    ArduinoOTA.onStart([]() {
        Serial.println("Inicio de actualización OTA...");
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("Actualización completada, reiniciando...");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progreso: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error OTA [%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Error de autenticación");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Error al iniciar");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Error de conexión");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Error de recepción");
        else if (error == OTA_END_ERROR) Serial.println("Error al finalizar");
    });

    ArduinoOTA.begin();
    Serial.println("OTA lista, esperando actualizaciones...");
}

void setup() {
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);
    setup_wifi();
    client.setServer(mqtt_server, mqtt_port);
    reconnect();
    setupOTA();
}

// Función para enviar telemetría periódica
void sendTelemetry() {
    if (client.connected()) {
        String telemetry = "{\"temperature\": 25.5, \"humidity\": 60}";
        client.publish("v1/devices/me/telemetry", telemetry.c_str());
        Serial.println("Millis: " + String(millis()) + " - Telemetría enviada: " + telemetry);
    }
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    ArduinoOTA.handle();  // Escucha actualizaciones OTA

    if (millis() - lastTelemetry > 5000) {
        digitalWrite(LED_BUILTIN, LOW);
        sendTelemetry();
        lastTelemetry = millis();
    } else {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);
        digitalWrite(LED_BUILTIN, LOW);
        delay(1000);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);
    }
}
