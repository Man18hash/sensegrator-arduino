#include <Servo.h>

// ---- Pins ----
const int SERVO_SORT_PIN = 6;   // sorter servo (D6)
const int SERVO_LID_PIN  = 5;   // lid servo (D5)
const int RELAY_PIN      = 7;   // relay (assumes active-HIGH)

// ---- Angles ----
const int SORT_G    = 10;       // S command target
const int SORT_S    = 85;      // G command target
const int SORT_A    = 160;      // A command target
const int SORT_HOME = 85;      // home position for sorter

const int LID_OPEN  = 90;       // open
const int LID_CLOSE = 150;      // close

// ---- Timings ----
const unsigned long WAIT_BEFORE_OPEN_MS   = 1000; // after moving sorter
const unsigned long HOLD_AFTER_OPEN_MS    = 1000; // sorter remains after opening lid
const unsigned long WAIT_BEFORE_CLOSE_MS  = 1000; // before closing lid+sorter
const unsigned long RELAY_ON_MS           = 2000; // relay pulse = 2 seconds

Servo sorter, lid;
volatile bool isBusy = false;

// Map incoming char to sorter angle
int mapCmdToSorter(char c) {
  switch (c) {
    case 'S': return SORT_S;
    case 'G': return SORT_G;
    case 'A': return SORT_A;
    default:  return -1;
  }
}

// Discard bytes up to and including next newline
void discardLine() {
  while (Serial.available()) {
    if (Serial.read() == '\n') break;
  }
}

// Flush ALL pending input (discard everything in RX buffer)
void flushInput() {
  while (Serial.available()) { Serial.read(); }
}

void doSequence(int sorterTarget) {
  isBusy = true;

  // 1) Move sorter to target
  sorter.write(sorterTarget);
  delay(WAIT_BEFORE_OPEN_MS);

  // 2) Open lid
  lid.write(LID_OPEN);
  delay(HOLD_AFTER_OPEN_MS);

  // 3) Wait then close both
  delay(WAIT_BEFORE_CLOSE_MS);
  lid.write(LID_CLOSE);
  sorter.write(SORT_HOME);

  // 4) Relay pulse (2 seconds)
  digitalWrite(RELAY_PIN, HIGH);
  delay(RELAY_ON_MS);
  digitalWrite(RELAY_PIN, LOW);

  // Discard any commands that arrived while we were busy
  flushInput();

  isBusy = false;
}

void setup() {
  Serial.begin(9600);

  // Attach with explicit pulse range for reliability
  sorter.attach(SERVO_SORT_PIN, 1000, 2000);
  lid.attach(SERVO_LID_PIN,     1000, 2000);

  // Relay init
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // OFF initially (active-HIGH assumed)

  // Safe startup positions
  sorter.write(SORT_HOME);
  lid.write(LID_CLOSE);

  Serial.println("Ready. Send one of: S, G, A (followed by newline).");
}

void loop() {
  if (!Serial.available()) return;

  // If we're busy, discard one whole line and wait
  if (isBusy) { discardLine(); return; }

  // Read command up to newline
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd.length() == 0) return;

  char c = toupper(cmd[0]);     // use only FIRST character of the line
  int target = mapCmdToSorter(c);

  if (target < 0) {
    Serial.print("Unknown command: ");
    Serial.println(c);
    return;
  }

  Serial.print("Executing command: ");
  Serial.println(c);
  doSequence(target);
  Serial.println("Done. Ready for next command.");
}
