#include <WiFi.h>
#include <WebServer.h>

#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Mitsubishi.h>

// ======================================================
// WI-FI
// ======================================================

const char* ssid = "B-13-3A";
const char* password = "Braga@2026";

// ======================================================
// SERVIDOR
// ======================================================

WebServer server(80);

// ======================================================
// PINOS
// ======================================================

// GPIO 21 será o transmissor IR.
// O GPIO não deve alimentar o LED IR diretamente.
// Use transistor/MOSFET + LED IR + resistor adequado.
const uint16_t PINO_IR = 21;

// GPIO 22 continua sendo o LED da sua interface.
const uint16_t PINO_LED = 22;

// ======================================================
// ESTADOS
// ======================================================

// Mantemos o nome "luz" para que o HTML existente
// continue funcionando sem nenhuma alteração.
bool luzSala = false;
bool fitaLed = false;

// Estado lógico do ar-condicionado.
// O HTML pode continuar usando "motor" para compatibilidade.
bool arLigado = false;

// ======================================================
// MITSUBISHI ELECTRIC
// ======================================================

IRMitsubishiAC ar(PINO_IR);

// Temperatura padrão enviada ao ar.
float temperaturaAr = 24.0;

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
// STATUS
// ======================================================

void handleStatus() {

    adicionarCORS();

    String json = "{";

    json += "\"connected\":true,";

    // Mantido para compatibilidade com o HTML atual
    json += "\"motor\":";
    json += arLigado ? "true" : "false";

    json += ",";

    // Mantido para compatibilidade com o switch "luz"
    json += "\"luz\":";
    json += arLigado ? "true" : "false";

    json += ",";

    json += "\"led\":";
    json += fitaLed ? "true" : "false";

    json += ",";

    json += "\"ar\":";
    json += arLigado ? "true" : "false";

    json += ",";

    json += "\"temperaturaAr\":";
    json += String(temperaturaAr, 1);

    json += "}";

    server.send(
        200,
        "application/json",
        json
    );
}

// ======================================================
// PREPARAR ESTADO DO AR
// ======================================================

void prepararAr() {

    ar.setTemp(
        temperaturaAr
    );

    ar.setMode(
        kMitsubishiAcCool
    );

    ar.setFan(
        kMitsubishiAcFanAuto
    );
}

// ======================================================
// LIGAR AR VIA IR
// ======================================================

void ligarAr() {

    prepararAr();

    ar.setPower(true);

    ar.send();

    arLigado = true;

    Serial.println(
        "IR -> Mitsubishi Electric: LIGAR"
    );
}

// ======================================================
// DESLIGAR AR VIA IR
// ======================================================

void desligarAr() {

    ar.setPower(false);

    ar.send();

    arLigado = false;

    Serial.println(
        "IR -> Mitsubishi Electric: DESLIGAR"
    );
}

// ======================================================
// /luz?state=on/off
//
// IMPORTANTE:
// O HTML continua chamando /luz.
// Agora esse comando controla o AR por IR.
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

    // --------------------------------------------------
    // LIGAR AR
    // --------------------------------------------------

    if (state == "on") {

        ligarAr();

        // Mantemos luzSala sincronizada para
        // compatibilidade com o JSON antigo.
        luzSala = true;
    }

    // --------------------------------------------------
    // DESLIGAR AR
    // --------------------------------------------------

    else if (state == "off") {

        desligarAr();

        luzSala = false;
    }

    // --------------------------------------------------
    // ESTADO INVÁLIDO
    // --------------------------------------------------

    else {

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

    // Campos esperados pelo HTML
    json += "\"luz\":";
    json += arLigado ? "true" : "false";

    json += ",";

    json += "\"ar\":";
    json += arLigado ? "true" : "false";

    json += ",";

    json += "\"motor\":";
    json += arLigado ? "true" : "false";

    json += "}";

    server.send(
        200,
        "application/json",
        json
    );

    Serial.print(
        "Comando /luz recebido -> AR "
    );

    Serial.println(
        arLigado ? "LIGADO" : "DESLIGADO"
    );
}

// ======================================================
// /led?state=on/off
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
    }

    else if (state == "off") {

        fitaLed = false;

        digitalWrite(
            PINO_LED,
            LOW
        );
    }

    else {

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

    Serial.print(
        "Fita LED: "
    );

    Serial.println(
        fitaLed ? "LIGADA" : "DESLIGADA"
    );
}

// ======================================================
// /ar?state=on/off
//
// Opcionalmente disponível também de forma direta.
// ======================================================

void handleAr() {

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

        ligarAr();
    }

    else if (state == "off") {

        desligarAr();
    }

    else {

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
    json += "\"ar\":";
    json += arLigado ? "true" : "false";

    json += ",";

    json += "\"motor\":";
    json += arLigado ? "true" : "false";

    json += "}";

    server.send(
        200,
        "application/json",
        json
    );
}

// ======================================================
// /ar/temp?value=24
// ======================================================

void handleArTemperatura() {

    adicionarCORS();

    if (!server.hasArg("value")) {

        server.send(
            400,
            "application/json",
            "{\"success\":false,\"error\":\"Parametro value ausente\"}"
        );

        return;
    }

    float novaTemperatura =
        server.arg("value").toFloat();

    if (
        novaTemperatura < 16 ||
        novaTemperatura > 30
    ) {

        server.send(
            400,
            "application/json",
            "{\"success\":false,\"error\":\"Temperatura deve estar entre 16 e 30 graus\"}"
        );

        return;
    }

    temperaturaAr = novaTemperatura;

    // Se o ar estiver ligado, transmite
    // a nova configuração imediatamente.
    if (arLigado) {

        prepararAr();

        ar.setPower(true);

        ar.send();
    }

    String json = "{";

    json += "\"success\":true,";
    json += "\"connected\":true,";
    json += "\"ar\":";
    json += arLigado ? "true" : "false";

    json += ",";

    json += "\"temperatura\":";
    json += String(
        temperaturaAr,
        1
    );

    json += "}";

    server.send(
        200,
        "application/json",
        json
    );
}

// ======================================================
// OPTIONS / CORS
// ======================================================

void handleOptions() {

    adicionarCORS();

    server.send(204);
}

// ======================================================
// SETUP
// ======================================================

void setup() {

    Serial.begin(115200);

    // ==================================================
    // GPIO LED
    // ==================================================

    pinMode(
        PINO_LED,
        OUTPUT
    );

    digitalWrite(
        PINO_LED,
        LOW
    );

    // ==================================================
    // IR
    // ==================================================

    ar.begin();

    // Estado inicial do ar
    arLigado = false;

    prepararAr();

    ar.setPower(false);

    // ==================================================
    // WI-FI
    // ==================================================

    Serial.println();
    Serial.println(
        "=============================="
    );

    Serial.println(
        "ESP32 - CENTRAL DE CONTROLE"
    );

    Serial.println(
        "MITSUBISHI ELECTRIC + IR"
    );

    Serial.println(
        "=============================="
    );

    Serial.print(
        "Conectando ao Wi-Fi"
    );

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

    // ==================================================
    // ROTAS
    // ==================================================

    server.on(
        "/status",
        HTTP_GET,
        handleStatus
    );

    // Mantém a API que seu HTML já utiliza
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

    // Rota direta adicional para o ar
    server.on(
        "/ar",
        HTTP_GET,
        handleAr
    );

    server.on(
        "/ar/temp",
        HTTP_GET,
        handleArTemperatura
    );

    // ==================================================
    // OPTIONS
    // ==================================================

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

    server.on(
        "/ar",
        HTTP_OPTIONS,
        handleOptions
    );

    server.on(
        "/ar/temp",
        HTTP_OPTIONS,
        handleOptions
    );

    // ==================================================
    // SERVIDOR
    // ==================================================

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
