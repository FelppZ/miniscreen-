#include <WiFi.h>
#include <WebServer.h>

// ======================================================
// WI-FI
// ======================================================

//const char* ssid = "CONDCAZUL";
//const char* password = "cazuL265251";

const char* ssid = "BRAGA";
const char* password = "Fab222324@";

// ======================================================
// SERVIDOR
// ======================================================

WebServer server(80);

// ======================================================
// PINOS
// ======================================================

const int PINO_LUZ = 21;
const int PINO_LED = 22;

// ======================================================
// ESTADOS
// ======================================================

bool luzSala = false;
bool fitaLed = false;
bool motor = false;

// ======================================================
// CORS
// ======================================================

void adicionarCORS() {

    server.sendHeader(
        "Access-Control-Allow-Origin",
        "*"
    );

    server.sendHeader(
        "Access-Control-Allow-Methods",
        "GET, OPTIONS"
    );

    server.sendHeader(
        "Access-Control-Allow-Headers",
        "Content-Type"
    );
}

// ======================================================
// /status
// ======================================================

void handleStatus() {

    adicionarCORS();

    String json = "{";

    json += "\"connected\":true,";

    json += "\"motor\":";
    json += motor ? "true" : "false";

    json += ",";

    json += "\"luz\":";
    json += luzSala ? "true" : "false";

    json += ",";

    json += "\"led\":";
    json += fitaLed ? "true" : "false";

    json += "}";

    server.send(
        200,
        "application/json",
        json
    );
}

// ======================================================
// /luz?state=on
// ======================================================

void handleLuz() {

    adicionarCORS();

    if (!server.hasArg("state")) {

        server.send(
            400,
            "application/json",
            "{\"success\":false,\"error\":\"Parametro state ausente\"}"
        );

        return;
    }

    String state = server.arg("state");

    if (state == "on") {

        luzSala = true;

        digitalWrite(
            PINO_LUZ,
            HIGH
        );

    } else if (state == "off") {

        luzSala = false;

        digitalWrite(
            PINO_LUZ,
            LOW
        );

    } else {

        server.send(
            400,
            "application/json",
            "{\"success\":false,\"error\":\"Estado invalido\"}"
        );

        return;
    }

    String json = "{";

    json += "\"success\":true,";
    json += "\"connected\":true,";
    json += "\"luz\":";
    json += luzSala ? "true" : "false";

    json += "}";

    server.send(
        200,
        "application/json",
        json
    );

    Serial.print("Luz sala: ");

    if (luzSala) {
        Serial.println("LIGADA");
    } else {
        Serial.println("DESLIGADA");
    }
}

// ======================================================
// /led?state=on
// ======================================================

void handleLed() {

    adicionarCORS();

    if (!server.hasArg("state")) {

        server.send(
            400,
            "application/json",
            "{\"success\":false,\"error\":\"Parametro state ausente\"}"
        );

        return;
    }

    String state = server.arg("state");

    if (state == "on") {

        fitaLed = true;

        digitalWrite(
            PINO_LED,
            HIGH
        );

    } else if (state == "off") {

        fitaLed = false;

        digitalWrite(
            PINO_LED,
            LOW
        );

    } else {

        server.send(
            400,
            "application/json",
            "{\"success\":false,\"error\":\"Estado invalido\"}"
        );

        return;
    }

    String json = "{";

    json += "\"success\":true,";
    json += "\"connected\":true,";
    json += "\"led\":";
    json += fitaLed ? "true" : "false";

    json += "}";

    server.send(
        200,
        "application/json",
        json
    );

    Serial.print("Fita LED: ");

    if (fitaLed) {
        Serial.println("LIGADA");
    } else {
        Serial.println("DESLIGADA");
    }
}

// ======================================================
// OPTIONS / CORS
// ======================================================

void handleOptions() {

    adicionarCORS();

    server.send(
        204
    );
}

// ======================================================
// SETUP
// ======================================================

void setup() {

    Serial.begin(115200);

    // --------------------------------------------------
    // GPIO
    // --------------------------------------------------

    pinMode(
        PINO_LUZ,
        OUTPUT
    );

    pinMode(
        PINO_LED,
        OUTPUT
    );

    // Estados iniciais
    digitalWrite(
        PINO_LUZ,
        luzSala ? HIGH : LOW
    );

    digitalWrite(
        PINO_LED,
        fitaLed ? HIGH : LOW
    );

    // --------------------------------------------------
    // WI-FI
    // --------------------------------------------------

    Serial.println();
    Serial.println("==============================");
    Serial.println("ESP32 - CENTRAL DE CONTROLE");
    Serial.println("==============================");

    Serial.print("Conectando ao Wi-Fi");

    WiFi.begin(
        ssid,
        password
    );

    while (
        WiFi.status() != WL_CONNECTED
    ) {

        delay(500);

        Serial.print(".");
    }

    Serial.println();
    Serial.println();

    Serial.println(
        "Wi-Fi conectado!"
    );

    Serial.print(
        "IP do ESP32: "
    );

    Serial.println(
        WiFi.localIP()
    );

    // --------------------------------------------------
    // ROTAS
    // --------------------------------------------------

    server.on(
        "/status",
        HTTP_GET,
        handleStatus
    );

    server.on(
        "/luz",
        HTTP_GET,
        handleLuz
    );

    server.on(
        "/led",
        HTTP_GET,
        handleLed
    );

    // --------------------------------------------------
    // CORS OPTIONS
    // --------------------------------------------------

    server.on(
        "/status",
        HTTP_OPTIONS,
        handleOptions
    );

    server.on(
        "/luz",
        HTTP_OPTIONS,
        handleOptions
    );

    server.on(
        "/led",
        HTTP_OPTIONS,
        handleOptions
    );

    // --------------------------------------------------
    // SERVIDOR
    // --------------------------------------------------

    server.begin();

    Serial.println(
        "Servidor HTTP iniciado!"
    );

    Serial.println();
}

// ======================================================
// LOOP
// ======================================================

void loop() {

    server.handleClient();
}
