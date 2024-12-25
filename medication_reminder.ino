#include <EtherCard.h>
#include <RTClib.h>


static byte myip[] = { 169, 254, 238, 216 };
static byte gwip[] = { 169, 254, 15, 140 };
# define Button A0
# define Buzz 7
bool reminderCleared = false;

static byte mymac[] = { 0x74, 0x69, 0x69, 0x2D, 0x30, 0x31 };
byte Ethernet::buffer[500];


RTC_DS3231 rtc;

struct MedicineReminder {
  int hour;
  int minute;
  const char *medicineName;
};

MedicineReminder reminders[] = {
  {8, 0, "Take Aspirin"},
  {12, 30, "Take Vitamin C"},
  {10, 28, "Take Blood Pressure Medication"}
};
const int reminderCount = sizeof(reminders) / sizeof(reminders[0]);

void setup() {

  Serial.begin(9600);
  pinMode(Buzz,OUTPUT);
  pinMode(Button,INPUT_PULLUP);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting the time!");

    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }


  ether.begin(sizeof Ethernet::buffer, mymac, SS);

  ether.staticSetup(myip, gwip);
}

void loop() {

  DateTime now = rtc.now();
   
  const char *currentReminder = "No reminders at this time.";
  if (!reminderCleared) {
    for (int i = 0; i < reminderCount; i++) {
      if (now.hour() == reminders[i].hour && now.minute() == reminders[i].minute) {
        currentReminder = reminders[i].medicineName;
        break;
      }
    }
  }

  if (currentReminder != "No reminders at this time.") {
    digitalWrite(Buzz, HIGH);
  } else {
    digitalWrite(Buzz, LOW);
  }

  if (digitalRead(Button) == LOW) {
    reminderCleared = true;
    currentReminder = "No reminders at this time.";
  }

  char htmlResponse[300];
  sprintf(htmlResponse, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                        "<html><body><h1>%02d:%02d</h1><p>%s</p></body></html>",
          now.hour(), now.minute(), currentReminder);

  word pos = ether.packetLoop(ether.packetReceive());
  if (pos) {
    char *data = (char *)Ethernet::buffer + pos;
    memcpy(ether.tcpOffset(), htmlResponse, strlen(htmlResponse));
    ether.httpServerReply(strlen(htmlResponse));
  }

}
