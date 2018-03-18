#include <OBD.h>
#include <TFT_HX8357.h>
#include "Free_Fonts.h"
#include "Graph.h"

TFT_HX8357 tft = TFT_HX8357();
Graph graph = Graph();

COBD obd = COBD();

#define TFT_GREY 0x5AEB

#define BT_PWR 20 // Used to power up/down the BT module
#define BT_ENABLE 21 // Not really used for anything in my experience but here for completeness
#define BT_STATE 17 // The "state" pin of the HC-06. Is pulled high when the module is connected

// Initialize some values
int rpm = 0, ect = 0, speed = 0, maf = 0, load = 0;
int prevRpm = 1, prevEct = 1, prevSpeed = 1;
double cons = 0;
double cons1 = 0, cons2 = 0, consavg = 0;
unsigned long mpoints = 0;

void initialize();

void setup() {
  pinMode(BT_STATE, INPUT);
  pinMode(BT_PWR, OUTPUT);
  pinMode(BT_ENABLE, OUTPUT);

  // Start the serial debugging port
  Serial.begin(38400);

  // Initialize the TFT
  tft.init();
  tft.setRotation(1);

  // Try to initialize a BT and OBD-2 connection
  initialize();
}

void loop() {
  obd.readPID(PID_RPM, rpm);
  if (rpm != prevRpm) {
    prevRpm = rpm;
    char buf[15];
    sprintf(buf, "RPM: %i      ", rpm);
    tft.drawString(buf, 0, (4 * tft.fontHeight(4)), 4);
  }

  obd.readPID(PID_COOLANT_TEMP, ect);
  if (ect != prevEct) {
    prevEct = ect;
    char buf2[15];
    sprintf(buf2, "ECT: %i C    ", ect);
    tft.drawString(buf2, 0, (5 * tft.fontHeight(4)), 4);
  }

  obd.readPID(PID_SPEED, speed);
  if (speed != prevSpeed) {
    prevSpeed = speed;
    char buf3[18];
    sprintf(buf3, "SPD: %i KM/h     ", speed);
    tft.drawString(buf3, 0, (6 * tft.fontHeight(4)), 4);
  }

  if (speed > 0) {
    obd.readPID(PID_MAF_FLOW, maf);
    obd.readPID(PID_ENGINE_LOAD, load);  
    /*
     * Calculate consumption. This is done using the following formula:
     * FuelFlow (Litres/Hour) =  a * (Airflow (g/s) * Load (%)) + b
     * Where A and B are calibration coefficients, AirFlow is taken from the ECU (MAF sensor) and Load is calculated/approximated by the ECU.
     * A and B can be obtained by measuring the actual FuelFlow and comparing with the results of this formula with A and B both zero.
     * 
     * This formula (and the calibration values) were taken from https://www3.epa.gov/ttn/chief/conference/ei20/session8/aalessandrini.pdf Page 11
     */
    cons = (double)((0.0023 * ((double) maf * (double) load) + 0.55) / speed) * 100;

    if (cons1 == 0) {
      cons1 = cons;
      mpoints++;
    } else if (cons2 == 0) {
      cons2 = cons;
      consavg = (cons1 + cons2) / 2;
      mpoints++;
    } else {
      consavg = ((consavg * mpoints) + cons) / (mpoints + 1);
      mpoints++;
    }
    /*
     * So consumption is calculated in Litres/Hour (L/h) from the above equation (a * (Airflow (g/s) * Load(%)) + b)
     * Calculating Liters/100KM or KM/Litre is trivial now:
     * Consumption (L/100KM) = (Consumption (L/h) / Speed (KM/h)) * 100
     * Consumption (KM/L) = 100 / Consumption (L/100KM)
     */
     int consX = tft.textWidth("CONS: ", 4);
     int consY = 7 * tft.fontHeight(4);
     int avgX = tft.textWidth("AVG: ", 4);
     int avgY = 8 * tft.fontHeight(4);
     
     tft.drawString("CONS:            L/100KM", 0, consY, 4);
     tft.drawFloat((float) cons, 2, consX, consY, 4);
     tft.drawString("AVG:             L/100KM", 0, avgY, 4);
     tft.drawFloat((float) consavg, 2, avgX, avgY, 4);
     graph.plot((uint16_t) cons);
  } else {
    tft.drawString("CONS: ---------- L/100KM", 0, (7 * tft.fontHeight(4)), 4);
    graph.plot(0);
  }

  if (digitalRead(BT_STATE) == 0) {
    tft.fillScreen(TFT_BLACK);
    initialize();
  }
}

void initialize() {
  // Clear the screen
  int xpos = 0;
  int ypos = tft.fontHeight(GFXFF) * 2;

  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);

  // Initializes the graph's axes
  graph.init(0, (8 * tft.fontHeight(4)), tft.width(), tft.height(), tft);
  
  tft.setCursor(xpos, ypos);

  tft.setCursor(0, 0);
  tft.print("Initializing Bluetooth: ");

  // Power up the HC-06 module
  digitalWrite(BT_ENABLE, HIGH);
  digitalWrite(BT_PWR, HIGH);
  delay(1000);

  // Wait for a BT connection
  while (digitalRead(BT_STATE) == 0) {
    delay(5);
  }

  tft.println("OK   ");

  // Start the OBD-2 serial connection and try to connect to the ECU
  obd.begin();
  bool success = false;
  while (!success) {
    tft.setCursor(xpos, ypos);
    tft.print(F("OBD-II Connection initializing: "));
    success = obd.init(PROTO_KWP2000_FAST); // This should be the protocol your car uses, or empty for auth-search
    if (success) {
      tft.println("OK   ");
    } else {
      tft.println("ERROR");
    }
    // If we lose the BT connection, re-init
    if (digitalRead(BT_STATE) == 0) {
      tft.fillScreen(TFT_BLACK);
      initialize();
    }
  }

  // We're connected! Request the used protocol from the ELM-327
  char proto[32];
  obd.sendCommand("ATDP\r", proto, sizeof(proto) * 12);
  tft.print(F("OBD-II Connection initialized: "));
  tft.println(proto);
}

