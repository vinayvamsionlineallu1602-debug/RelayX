```cpp
/*
 * RelayX - Portable Wi-Fi Relay
 * -----------------------------
 * ESP32-based Wi-Fi extender prototype.
 *
 * Features:
 *   - Connects to an existing Wi-Fi router
 *   - Creates a separate Wi-Fi network for users
 *   - NAT/NAPT forwards client traffic to the main router
 *   - RSSI-based placement indicator
 *   - Battery voltage monitoring
 *   - Built-in status web page
 *   - Automatic upstream reconnection
 *
 * File:
 *   relayx_wifi_relay.ino
 *
 * Hardware:
 *   - ESP32 Development Board
 *   - Li-ion/LiPo battery
 *   - USB-C charging/power circuit
 *   - Battery voltage divider
 *   - Status LED
 *
 * IMPORTANT:
 *   This project assumes an ESP32 board/core version that supports
 *   WiFi.AP.enableNAPT(), as provided by modern Arduino-ESP32.
 */

// ============================================================
// LIBRARIES
// ============================================================

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// ============================================================
// USER CONFIGURATION
// ============================================================

// Existing/main router credentials
const char* MAIN_WIFI_SSID = "YOUR_MAIN_WIFI";
const char* MAIN_WIFI_PASSWORD = "YOUR_MAIN_PASSWORD";

// Wi-Fi network created by RelayX
const char* RELAY_SSID = "RelayX";
const char* RELAY_PASSWORD = "relayx123";

// ============================================================
// PIN CONFIGURATION
// ============================================================

// Status LED
#define STATUS_LED_PIN 2

// Battery voltage ADC pin
// Change this according to your ESP32 board.
#define BATTERY_ADC_PIN 34

// ============================================================
// BATTERY CONFIGURATION
// ============================================================

// Example voltage divider:
// Battery + ---- R1 ---- ADC ---- R2 ---- GND
//
// R1 = 100k
// R2 = 100k
//
// ADC voltage is approximately half
// of the actual battery voltage.

#define R1 100000.0
#define R2 100000.0

#define ADC_REFERENCE 3.3
#define ADC_MAX_VALUE 4095.0

// Approximate single-cell Li-ion thresholds
#define BATTERY_FULL 4.20
#define BATTERY_EMPTY 3.20

// ============================================================
// ACCESS POINT CONFIGURATION
// ============================================================

IPAddress AP_IP(
    192, 168, 4, 1
);

IPAddress AP_GATEWAY(
    192, 168, 4, 1
);

IPAddress AP_SUBNET(
    255, 255, 255, 0
);

IPAddress AP_DNS(
    8, 8, 8, 8
);

// ============================================================
// WEB SERVER
// ============================================================

WebServer server(80);

// ============================================================
// STATE
// ============================================================

bool upstreamConnected = false;
bool naptEnabled = false;

unsigned long lastReconnectAttempt = 0;
unsigned long lastStatusPrint = 0;

const unsigned long RECONNECT_INTERVAL = 10000;
const unsigned long STATUS_INTERVAL = 5000;

// ============================================================
// BATTERY READING
// ============================================================

float readBatteryVoltage()
{
    int raw = analogRead(
        BATTERY_ADC_PIN
    );

    float adcVoltage =
        (raw / ADC_MAX_VALUE)
        * ADC_REFERENCE;

    float batteryVoltage =
        adcVoltage
        * ((R1 + R2) / R2);

    return batteryVoltage;
}

// ============================================================
// BATTERY PERCENTAGE
// ============================================================

int batteryPercentage()
{
    float voltage =
        readBatteryVoltage();

    if (voltage >= BATTERY_FULL)
        return 100;

    if (voltage <= BATTERY_EMPTY)
        return 0;

    float percentage =
        ((voltage - BATTERY_EMPTY) /
        (BATTERY_FULL - BATTERY_EMPTY))
        * 100.0;

    return constrain(
        (int)percentage,
        0,
        100
    );
}

// ============================================================
// RSSI / PLACEMENT QUALITY
// ============================================================

String getPlacementStatus()
{
    if (!upstreamConnected)
        return "NO CONNECTION";

    int rssi =
        WiFi.RSSI();

    /*
     * Approximate placement guidance:
     *
     * -40 to -59 dBm  = Excellent
     * -60 to -69 dBm  = Good
     * -70 to -79 dBm  = Weak
     * below -80 dBm   = Too far
     */

    if (rssi >= -59)
        return "EXCELLENT";

    if (rssi >= -69)
        return "GOOD";

    if (rssi >= -79)
        return "WEAK";

    return "MOVE CLOSER";
}

// ============================================================
// STATUS LED
// ============================================================

void updateStatusLED()
{
    if (!upstreamConnected)
    {
        // Fast blink when disconnected
        digitalWrite(
            STATUS_LED_PIN,
            millis() % 500 < 250
        );

        return;
    }

    int rssi =
        WiFi.RSSI();

    /*
     * Slow blink when signal is weak.
     * Solid when signal is usable.
     */

    if (rssi < -75)
    {
        digitalWrite(
            STATUS_LED_PIN,
            millis() % 1000 < 500
        );
    }
    else
    {
        digitalWrite(
            STATUS_LED_PIN,
            HIGH
        );
    }
}

// ============================================================
// WEB PAGE
// ============================================================

String createStatusPage()
{
    String html;

    html += "<!DOCTYPE html>";
    html += "<html>";
    html += "<head>";

    html += "<meta name='viewport' ";
    html += "content='width=device-width, initial-scale=1'>";

    html += "<title>RelayX Status</title>";

    html += "<style>";

    html += "body{";
    html += "font-family:Arial;";
    html += "background:#f4f6f8;";
    html += "padding:20px;";
    html += "}";

    html += ".card{";
    html += "background:white;";
    html += "padding:20px;";
    html += "border-radius:12px;";
    html += "max-width:500px;";
    html += "margin:auto;";
    html += "box-shadow:0 2px 10px #ccc;";
    html += "}";

    html += ".title{";
    html += "font-size:28px;";
    html += "font-weight:bold;";
    html += "}";

    html += ".item{";
    html += "padding:12px 0;";
    html += "border-bottom:1px solid #ddd;";
    html += "}";

    html += "</style>";

    html += "</head>";

    html += "<body>";

    html += "<div class='card'>";

    html += "<div class='title'>RelayX</div>";

    html += "<p>Portable Wi-Fi Relay</p>";

    html += "<div class='item'>";
    html += "<b>Upstream:</b> ";

    if (upstreamConnected)
        html += "CONNECTED";
    else
        html += "DISCONNECTED";

    html += "</div>";

    html += "<div class='item'>";
    html += "<b>Router RSSI:</b> ";

    if (upstreamConnected)
    {
        html += String(
            WiFi.RSSI()
        );

        html += " dBm";
    }
    else
    {
        html += "--";
    }

    html += "</div>";

    html += "<div class='item'>";
    html += "<b>Placement:</b> ";
    html += getPlacementStatus();
    html += "</div>";

    html += "<div class='item'>";
    html += "<b>Battery:</b> ";
    html += String(
        readBatteryVoltage(),
        2
    );

    html += " V (";
    html += String(
        batteryPercentage()
    );

    html += "%)</div>";

    html += "<div class='item'>";
    html += "<b>Relay Network:</b> ";
    html += RELAY_SSID;
    html += "</div>";

    html += "<div class='item'>";
    html += "<b>Connected Users:</b> ";
    html += String(
        WiFi.AP.stationCount()
    );
    html += "</div>";

    html += "<div class='item'>";
    html += "<b>NAPT:</b> ";

    if (naptEnabled)
        html += "ENABLED";
    else
        html += "DISABLED";

    html += "</div>";

    html += "</div>";

    html += "</body>";
    html += "</html>";

    return html;
}

// ============================================================
// WEB SERVER HANDLER
// ============================================================

void handleRoot()
{
    server.send(
        200,
        "text/html",
        createStatusPage()
    );
}

// ============================================================
// WIFI EVENT HANDLER
// ============================================================

void onWiFiEvent(
    arduino_event_id_t event,
    arduino_event_info_t info
)
{
    switch (event)
    {
        case ARDUINO_EVENT_WIFI_STA_START:

            Serial.println(
                "[WiFi] STA started"
            );

            break;

        case ARDUINO_EVENT_WIFI_STA_CONNECTED:

            Serial.println(
                "[WiFi] Connected to main router"
            );

            break;

        case ARDUINO_EVENT_WIFI_STA_GOT_IP:

            Serial.println(
                "[WiFi] Upstream IP received"
            );

            Serial.print(
                "[WiFi] IP: "
            );

            Serial.println(
                WiFi.localIP()
            );

            upstreamConnected = true;

            /*
             * Enable NAPT so devices connected
             * to the RelayX AP can access the
             * upstream network through the ESP32.
             */

            if (WiFi.AP.enableNAPT(true))
            {
                naptEnabled = true;

                Serial.println(
                    "[RelayX] NAPT enabled"
                );
            }
            else
            {
                Serial.println(
                    "[RelayX] Failed to enable NAPT"
                );
            }

            break;

        case ARDUINO_EVENT_WIFI_STA_LOST_IP:

            Serial.println(
                "[WiFi] Lost upstream IP"
            );

            upstreamConnected = false;

            if (WiFi.AP.enableNAPT(false))
            {
                naptEnabled = false;
            }

            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:

            Serial.println(
                "[WiFi] Main router disconnected"
            );

            upstreamConnected = false;

            if (WiFi.AP.enableNAPT(false))
            {
                naptEnabled = false;
            }

            break;

        case ARDUINO_EVENT_WIFI_AP_START:

            Serial.println(
                "[RelayX] Access Point started"
            );

            Serial.print(
                "[RelayX] AP IP: "
            );

            Serial.println(
                WiFi.AP.localIP()
            );

            break;

        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:

            Serial.println(
                "[RelayX] User connected"
            );

            break;

        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:

            Serial.println(
                "[RelayX] User disconnected"
            );

            break;

        default:

            break;
    }
}

// ============================================================
// START ACCESS POINT
// ============================================================

void startRelayAP()
{
    Serial.println(
        "[RelayX] Starting portable AP..."
    );

    /*
     * Configure AP network.
     */

    WiFi.AP.begin();

    WiFi.AP.config(
        AP_IP,
        AP_GATEWAY,
        AP_SUBNET,
        AP_IP,
        AP_DNS
    );

    /*
     * Create RelayX Wi-Fi network.
     */

    if (!WiFi.AP.create(
            RELAY_SSID,
            RELAY_PASSWORD,
            1,
            0,
            8))
    {
        Serial.println(
            "[RelayX] AP creation failed"
        );

        return;
    }

    if (!WiFi.AP.waitStatusBits(
            ESP_NETIF_STARTED_BIT,
            1000))
    {
        Serial.println(
            "[RelayX] AP failed to start"
        );

        return;
    }

    Serial.println(
        "[RelayX] AP ready"
    );

    Serial.print(
        "[RelayX] SSID: "
    );

    Serial.println(
        RELAY_SSID
    );

    Serial.print(
        "[RelayX] Password: "
    );

    Serial.println(
        RELAY_PASSWORD
    );

    Serial.print(
        "[RelayX] IP: "
    );

    Serial.println(
        WiFi.AP.localIP()
    );
}

// ============================================================
// CONNECT TO MAIN ROUTER
// ============================================================

void connectToMainRouter()
{
    if (WiFi.status() == WL_CONNECTED)
        return;

    Serial.println(
        "[RelayX] Connecting to main router..."
    );

    WiFi.begin(
        MAIN_WIFI_SSID,
        MAIN_WIFI_PASSWORD
    );
}

// ============================================================
// SERIAL STATUS
// ============================================================

void printStatus()
{
    Serial.println();
    Serial.println(
        "========== RELAYX STATUS =========="
    );

    Serial.print(
        "Upstream: "
    );

    if (upstreamConnected)
        Serial.println("CONNECTED");
    else
        Serial.println("DISCONNECTED");

    if (upstreamConnected)
    {
        Serial.print(
            "Router RSSI: "
        );

        Serial.print(
            WiFi.RSSI()
        );

        Serial.println(
            " dBm"
        );

        Serial.print(
            "Placement: "
        );

        Serial.println(
            getPlacementStatus()
        );

        Serial.print(
            "Router IP: "
        );

        Serial.println(
            WiFi.localIP()
        );
    }

    Serial.print(
        "Relay IP: "
    );

    Serial.println(
        WiFi.AP.localIP()
    );

    Serial.print(
        "Connected users: "
    );

    Serial.println(
        WiFi.AP.stationCount()
    );

    Serial.print(
        "Battery: "
    );

    Serial.print(
        readBatteryVoltage(),
        2
    );

    Serial.print(
        " V / "
    );

    Serial.print(
        batteryPercentage()
    );

    Serial.println(
        "%"
    );

    Serial.print(
        "NAPT: "
    );

    Serial.println(
        naptEnabled
        ? "ENABLED"
        : "DISABLED"
    );

    Serial.println(
        "==================================="
    );
}

// ============================================================
// SETUP
// ============================================================

void setup()
{
    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println(
        "==================================="
    );

    Serial.println(
        "          RELAYX STARTING"
    );

    Serial.println(
        " Portable Wi-Fi Relay Prototype"
    );

    Serial.println(
        "==================================="
    );

    // Status LED
    pinMode(
        STATUS_LED_PIN,
        OUTPUT
    );

    digitalWrite(
        STATUS_LED_PIN,
        LOW
    );

    // Battery ADC
    analogReadResolution(12);

    // Wi-Fi event handler
    Network.onEvent(
        onWiFiEvent
    );

    /*
     * Configure ESP32 for simultaneous
     * Station + Access Point operation.
     */

    WiFi.mode(
        WIFI_MODE_APSTA
    );

    /*
     * Start RelayX network.
     */

    startRelayAP();

    /*
     * Connect to existing router.
     */

    connectToMainRouter();

    /*
     * Status web page.
     */

    server.on(
        "/",
        handleRoot
    );

    server.begin();

    Serial.println(
        "[RelayX] Status server started"
    );

    Serial.println(
        "[RelayX] Setup complete"
    );
}

// ============================================================
// MAIN LOOP
// ============================================================

void loop()
{
    /*
     * Process web requests.
     */

    server.handleClient();

    /*
     * Update status LED.
     */

    updateStatusLED();

    /*
     * Reconnect to the main router
     * if the connection is lost.
     */

    if (
        !upstreamConnected &&
        millis() - lastReconnectAttempt
            >= RECONNECT_INTERVAL
    )
    {
        lastReconnectAttempt =
            millis();

        connectToMainRouter();
    }

    /*
     * Print status periodically.
     */

    if (
        millis() - lastStatusPrint
            >= STATUS_INTERVAL
    )
    {
        lastStatusPrint =
            millis();

        printStatus();
    }

    delay(10);
}
```
