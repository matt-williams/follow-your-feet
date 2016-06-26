#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

const char* ssid = "FabLab-Guest";
const char* password = "fablab!guest";

const int NUM_CHANS = 6;
const int CHAN_PORT[NUM_CHANS] = {2, 0, 4, 5, 12, 14};


String ipStr;

ESP8266WebServer server(80);

int dec2hex(char hex) {
  if ((hex >= '0') && (hex <= '9')) {
    return hex - '0';
  } else if ((hex >= 'A') && (hex <= 'F')) {
    return hex - 'A' + 10;
  } else if ((hex >= 'a') && (hex <= 'f')) {
    return hex - 'a' + 10;
  } else {
    return 0;
  }
}

void handleRoot() {
  String content = "<html><body>";
  content += "<h1>Follow-Your-Feet</h1>";
  content += "<h2>Usage</h2>";
  content += "<p>Send a request to <a href=\"http://" + ipStr + "/pulse\">http://" + ipStr + "/pulse</a> with the following parameters.</p>";
  content += "<ul>";
  content += "<li><code>t</code> - base period (microseconds)</li>";
  content += "<li><code>s</code> - number of cycles per sample</li>";
  for (int i = 0; i < NUM_CHANS; i++) {
    content += "<li><code>" + String((char)('a' + i)) + "</code> - channel " + String((char)('A' + i)) + " samples (hexadecimal)</li>";
  }
  content += "</ul>";
  content += "<h2>Examples</h2>";
  content += "<ul>";
  content += "<li><a href=\"/pulse?t=20&s=1000&a=f\">A</a></li>";
  content += "<li><a href=\"/pulse?t=20&s=1000&b=f\">B</a></li>";
  content += "<li><a href=\"/pulse?t=20&s=1000&c=f\">C</a></li>";
  content += "<li><a href=\"/pulse?t=20&s=1000&d=f\">D</a></li>";
  content += "<li><a href=\"/pulse?t=20&s=1000&e=f\">E</a></li>";
  content += "<li><a href=\"/pulse?t=20&s=1000&f=f\">F</a></li>";
  content += "<li><a href=\"/pulse?t=20&s=400&a=f0000000000&b=00f00000000&c=0000f000000&d=000000f0000&e=00000000f00&f=0000000000f\">A...B...C...D...E...F</a></li>";
  content += "<li><a href=\"/pulse?t=20&s=50&a=123456789abcdefedcba9876543210000000&b=0000000123456789abcdefedcba987654321\">A-&gt;B</a></li>";
  content += "<li><a href=\"/pulse?t=20&s=50&a=0000000123456789abcdefedcba987654321&b=123456789abcdefedcba9876543210000000\">B-&gt;A</a></li>";
  content += "<li><a href=\"/pulse?t=20&s=50&a=123456789abcdefedcba98765432100000000000000000000000000000000000&b=0000000123456789abcdefedcba9876543210000000000000000000000000000&c=00000000000000123456789abcdefedcba987654321000000000000000000000&d=000000000000000000000123456789abcdefedcba98765432100000000000000&e=0000000000000000000000000000123456789abcdefedcba9876543210000000&f=00000000000000000000000000000000000123456789abcdefedcba987654321&\">A-&gt;B-&gt;C-&gt;D-&gt;E-&gt;F</a></li>";
  content += "<li><a href=\"/pulse?t=20&s=50&a=00000000000000000000000000000000000123456789abcdefedcba987654321&b=0000000000000000000000000000123456789abcdefedcba9876543210000000&c=000000000000000000000123456789abcdefedcba98765432100000000000000&d=00000000000000123456789abcdefedcba987654321000000000000000000000&e=0000000123456789abcdefedcba9876543210000000000000000000000000000&f=123456789abcdefedcba98765432100000000000000000000000000000000000&\">F-&gt;E-&gt;D-&gt;C-&gt;B-&gt;A</a></li>";
  content += "</ul>";
  content += "</html></body>";
  server.send(200, "text/html", content);
}

void handlePulse() {
  int t = 20;
  if (server.hasArg("t")) {
    t = server.arg("t").toInt();
  }

  int s = 20;
  if (server.hasArg("s")) {
    t = server.arg("s").toInt();
  }

  int maxLen = 0;
  String samples[NUM_CHANS];
  for (int i = 0; i < NUM_CHANS; i++) {
    String param = String((char)('a' + i));
    if (server.hasArg(param)) {
      samples[i] = server.arg(param);
      Serial.printf("Channel ");
      Serial.print(param);
      Serial.print(": ");
      Serial.println(samples[i]);
      if (samples[i].length() > maxLen) {
        maxLen = samples[i].length();
      }
    } else {
      samples[i] = "";
    }
  }

  for (int i = 0; i < maxLen; i++) {
    for (int j = 0; j < s; j++) {
      int waitSoFar = 0;
      do {
        int nextWait = 15;
        for (int k = 0; k < NUM_CHANS; k++) {
          int amplitude = (i < samples[k].length()) ? dec2hex(samples[k][i]) : 0  ;
          digitalWrite(CHAN_PORT[k], (amplitude > waitSoFar) ? HIGH : LOW);
          nextWait = ((amplitude > waitSoFar) && (amplitude < nextWait)) ? amplitude : nextWait;
        }
        delayMicroseconds(t * nextWait);
        waitSoFar += nextWait;
      } while (waitSoFar < 15);
    }
  }
  for (int i = 0; i < NUM_CHANS; i++) {
    digitalWrite(CHAN_PORT[i], LOW);
  }
  Serial.println("Done");

  //String header = "HTTP/1.1 302 Found\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
  //server.sendContent(header);

  server.send(200, "text/html", "");
}

void setup(void){
  Serial.begin(9600);

  for (int i = 0; i < NUM_CHANS; i++) {
    pinMode(CHAN_PORT[i], OUTPUT);
    digitalWrite(CHAN_PORT[i], LOW);
  }

  WiFi.begin(ssid, password);
  Serial.println("");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
    
  IPAddress ip = WiFi.localIP();
  ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(ipStr);

  if (MDNS.begin("follow-your-feet")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/pulse", handlePulse);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void){
  server.handleClient();
}
