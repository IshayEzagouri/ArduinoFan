
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>

const char* ssid = "ESP8266";
const char* password = "1991";

ESP8266WebServer server(80);
IPAddress local_IP(192, 168, 82, 66);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

#define redBtn D5
#define greenBtn D6
#define fan D2
#define dhtPin D4

#define dhtType DHT11
DHT dht(dhtPin, dhtType);

float temp;
float setTemp = 25.0;

bool mode = false;
bool runOnce = false;
String stat = "OFF";
int status = 0;
int count = 0;
unsigned long timeNow = 0;

const char HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
  <body>
    <center>
      <h1 style='margin: 1em'>Fan Status:</h1>
      <h1 id = 'status'>OFF</h1>
      <div>
        <form action="/fan">
          <button style='margin: 2em; padding: 0em 1em' name='state' value=1><h1>On</h1></button>
          <button style='margin: 2em; padding: 0em 1em' name='state' value=0><h1>Off</h1></button>
        </form>
      </div>
      <form action="/value">
        <h1>Set Temperature:</h1><br>
        <input type='number' step='0.01' name='temp' style='font-size: 2em; padding: 0.25em;'>
        <br><br>
        <button style='margin: 2em; padding: 0em 1em;' type="submit"><h1>Submit</h1></button>
      </form>
    </center>
    <script>
      setInterval(
        function() {
          var xhr = new XMLHttpRequest();
          xhr.onreadystatechange = function() {
            if (this.readyState == 4 && this.status == 200) {
              document.getElementById('status').innerHTML = this.responseText;
            }
          };
          xhr.open('GET', '/status', true);
          xhr.send();
        },
      100);  
    </script>
  </body>
</html>
)=====";

void handleRoot() {
  server.send(200, "text/html", HTML);
}

void handleStatus() {
  server.send(200, "text/html", stat);
}

void handleValue() {
    setTemp = server.arg("temp").toFloat();
  server.send(200, "text/html", HTML);

  Serial.print("New Temperature: ");
  Serial.print(setTemp);
  Serial.println(F("°C"));
}

void handleFan() {
  int state = server.arg("state").toInt();
  server.send(200, "text/html", HTML);

  if (state == 1){
    digitalWrite(fan, LOW);
    stat = "ON";
  }
  else{
    digitalWrite(fan, HIGH);
    stat = "OFF";
  }
  Serial.print("Fan ");
  Serial.println(stat);
}

void handleNotFound() {
  server.send(404, "text/plain", "The content you are looking for was not found.");
}

void setup() {
  Serial.begin(115200);
  pinMode(redBtn, INPUT_PULLUP);
  pinMode(greenBtn, INPUT_PULLUP);
  pinMode(fan, OUTPUT);
  digitalWrite(fan, HIGH);

  dht.begin();

  Serial.println("Setting up Access point...");
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);

  Serial.println();
  Serial.print("Access point active on SSID: ");
  Serial.print(ssid);
  Serial.print(" & Password: ");
  Serial.println(password);

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/value", handleValue);
  server.on("/fan", handleFan);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.print("server active at ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();

  if (digitalRead(redBtn) == LOW && digitalRead(greenBtn) == LOW) {
    if (mode) {
      mode = false;
      Serial.println("Normal Mode");
    }
    else {
      mode = true;
      Serial.println("Temp Mode");
    }
    delay(1000);
  }

  else if (digitalRead(redBtn) == LOW && digitalRead(greenBtn) == HIGH) {
    Serial.println("Red Button Pressed");
    digitalWrite(fan, HIGH);
    status = 0;
    count = 0;
    stat = "OFF";
    delay(250);
    mode = false;
  }

  else if (digitalRead(greenBtn) == LOW && digitalRead(redBtn) == HIGH) {
    Serial.println("Green Button Pressed");
    digitalWrite(fan, LOW);
    status = 1;
    count = 0;
    stat = "ON";
    delay(250);
    mode = false;
  }

  if (millis() - timeNow >= 1000) {
    timeNow = millis();
    if (mode) {
      temp = dht.readTemperature();
      Serial.print(F("Temperature: "));
      Serial.print(temp);
      Serial.print(F("°C - "));

      if (temp > setTemp && !runOnce) {
        runOnce = true;
        Serial.print("HIGH");
        digitalWrite(fan, LOW);
        status = 1;
        count = 0;
        stat = "ON";
      }
      if (temp < setTemp) {
        runOnce = false;
        Serial.print("LOW");
        digitalWrite(fan, HIGH);
        status = 0;
        count = 0;
        stat = "OFF";
      }

      Serial.println();
    }

    if (status == 1) {
      Serial.println("Fan ON");
      count = count + 1;
      if (count >= 2) {
        digitalWrite(fan, HIGH);
        status = 2;
        stat = "OFF";
        count = 0;
      }
    }
    if (status == 2) {
      Serial.println("Fan OFF");
      count = count + 1;
      if (count >= 4) {
        digitalWrite(fan, LOW);
        status = 1;
        stat = "ON";
        count = 0;
      }
    }
  }
}
