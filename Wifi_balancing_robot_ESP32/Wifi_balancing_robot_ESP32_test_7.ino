//ESP32-WROOM-DA-Module
#include <WiFi.h>
#include <Wire.h>
 
// Define Slave I2C Address
#define SLAVE_ADDR 0x66

int currentRPM;
int previousRPM;

// Network credentials
const char* ssid = "WIN-CEO5OMC2UQS 2876";
const char* password= "0638/O3c";

// Web server on port 80 (http)
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Decode HTTP GET values
String posValue = String(0); // Default servo position
String rpmValue = String(0); // Default RPM value
String pValue = String(80);  // Default P value
String iValue = String(30);  // Default I value
String dValue = String(300); // Default D value

uint16_t sentAngle, senttorque, sentPreal, sentIreal, sentDreal;
int torque, Preal, Ireal, Dreal;
float realAngle;

uint8_t sendServo = 41;
uint8_t sendRPM = 126;
uint8_t sendP = 80;
uint8_t sendI = 30;
uint8_t sendD = 300;

/*float realAngle = 1.0;
float torque = 0.3;
float Preal = 0.5;
float Ireal = 3.7;
float Dreal = 8.7;*/

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;


void setup() {
  // Start serial
  Serial.begin(115200);

  // Initialize I2C communications as Slave
  Wire.begin(SLAVE_ADDR);
   // Function to run when data requested from master
  Wire.onRequest(requestEvent); 
  // Function to run when data received from master
  Wire.onReceive(receiveEvent);
  
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void receiveEvent(int wireSize){

  Serial.println("Recive event aktywowany");

 // (void)wireSize;
  while (Wire.available() < 5);

  Serial.println("Wire.availabe poszedł");
  
  sentAngle = Wire.read();
  senttorque = Wire.read();
  sentPreal = Wire.read();
  sentIreal = Wire.read();
  sentDreal = Wire.read();
  
  realAngle = (sentAngle - 126) * 0.05;
  torque = (senttorque - 126) * 5;
  Preal = (sentPreal - 126) * 5;
  Ireal = (sentIreal - 126) * 5;
  Dreal = (sentDreal - 126) * 5;

  Serial.println("Odebrano:");
  Serial.println(realAngle);
  Serial.println(sentPreal);
}
 
void requestEvent() {
  Wire.write(sendServo);
  Wire.write(sendRPM);
  Wire.write(sendP);
  Wire.write(sendI);
  Wire.write(sendD);
  Serial.println("Wysłano:");
  Serial.println(sendServo);
  Serial.println(sendP);
}

void loop() {

  // Listen for incoming clients
  WiFiClient client = server.available();

  // Client Connected
  if (client) {
    // Set timer references
    currentTime = millis();
    previousTime = currentTime;

    // Print to serial port
    Serial.println("New Client.");

    // String to hold data from client
    String currentLine = "";

    // Do while client is connected
    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {

            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK) and a content-type
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Display the HTML web page

            // HTML Header
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<meta http-equiv=\"refresh\" content=\"1.5\">"); // <----------------------------------------- Dodane
            client.println("<link rel=\"icon\" href=\"data:,\">");

            // CSS - Modify as desired
            client.println("<style>body { text-align: center; font-family: \"Trebuchet MS\", Arial; font-size: 18px; line-height: 12px; margin-left:auto; margin-right:auto; }");
            client.println(".slider { -webkit-appearance: none; width: 300px; height: 10px; border-radius: 10px; background: #ffffff; outline: none;  opacity: 0.7;-webkit-transition: .2s;  transition: opacity .2s;}");
            client.println(".slider::-webkit-slider-thumb {-webkit-appearance: none; appearance: none; width: 20px; height: 20px; border-radius: 50%; background: #ff3410; cursor: pointer; }</style>");

            // Get JQuery
            client.println("<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js\"></script>");

            // Page title
            client.println("</head><body style=\"background-color:#70cfff;\">"); // usunąłem <h1 style=\"color:#ff3410;\">Servo Control</h1>

            // Display realAngle, torque, Preal, Ireal, and Dreal at the top of the page
            client.print("<h2 style=\"color:#ffffff;\">__Angle:  ");
            client.print(realAngle);
            client.println("&#176;</h2>");
            
            client.print("<h2 style=\"color:#ffffff;\">Torque: ");
            client.print(torque);
            client.println("</h2>");
            
            client.print("<h2 style=\"color:#ffffff;\">     _____P     : ");
            client.print(Preal);
            client.println("</h2>");
            
            client.print("<h2 style=\"color:#ffffff;\">     0.00_I     : ");
            client.print(Ireal);
            client.println("</h2>");
            
            client.print("<h2 style=\"color:#ffffff;\">     _____D     : ");
            client.print(Dreal);
            client.println("</h2>");

            // Servo position display
            client.println("<div class='slider-container'>");
            client.println("<h2 style=\"color:#ffffff;\"> Skret: <span id=\"servoPos\"></span>&#176;</h2>");
            client.println("<input type=\"range\" min=\"-40\" max=\"40\" class=\"slider\" id=\"servoSlider\" onchange=\"updateServoPosition(this.value)\" value=\"" + posValue + "\"/>");
            client.println("</div>");

            // RPM control
            client.println("<div class='slider-container'>");
            client.println("<h2 style=\"color:#ffffff;\">+/-kZer: <span id=\"rpmValue\"></span></h2>");
            client.println("<input type=\"range\" min=\"-250\" max=\"250\" class=\"slider\" id=\"rpmSlider\" onchange=\"updateRPMValue(this.value)\" value=\"" + rpmValue + "\"/>");
            client.println("</div>");

            // P control
            client.println("<div class='slider-container'>");
            client.println("<h2 style=\"color:#ffffff;\">     P     : <span id=\"pValue\"></span></h2>");
            client.println("<input type=\"range\" min=\"0\" max=\"150\" class=\"slider\" id=\"pSlider\" onchange=\"updatepValue(this.value)\" value=\"" + pValue + "\"/>");
            client.println("</div>");

            // I control
            client.println("<div class='slider-container'>");
            client.println("<h2 style=\"color:#ffffff;\">     I     : <span id=\"iValue\"></span></h2>");
            client.println("<input type=\"range\" min=\"0\" max=\"500\" class=\"slider\" id=\"iSlider\" onchange=\"updateiValue(this.value)\" value=\"" + iValue + "\"/>");
            client.println("</div>");

            // D control
            client.println("<div class='slider-container'>");
            client.println("<h2 style=\"color:#ffffff;\">     D     : <span id=\"dValue\"></span></h2>");
            client.println("<input type=\"range\" min=\"0\" max=\"400\" class=\"slider\" id=\"dSlider\" onchange=\"updatedValue(this.value)\" value=\"" + dValue + "\"/>");
            client.println("</div>");

            // Javascript
            client.println("<script>var servoSlider = document.getElementById(\"servoSlider\");");
            client.println("var servoP = document.getElementById(\"servoPos\"); servoP.innerHTML = servoSlider.value;");
            client.println("servoSlider.oninput = function() { servoSlider.value = this.value; servoP.innerHTML = this.value; }");
            
            client.println("var rpmSlider = document.getElementById(\"rpmSlider\");");
            client.println("var rpmP = document.getElementById(\"rpmValue\"); rpmP.innerHTML = rpmSlider.value;");
            client.println("rpmSlider.oninput = function() { rpmSlider.value = this.value; rpmP.innerHTML = this.value; }");

            client.println("var pSlider = document.getElementById(\"pSlider\");");
            client.println("var pP = document.getElementById(\"pValue\"); pP.innerHTML = pSlider.value;");
            client.println("pSlider.oninput = function() { pSlider.value = this.value; pP.innerHTML = this.value; }");

            client.println("var iSlider = document.getElementById(\"iSlider\");");
            client.println("var iP = document.getElementById(\"iValue\"); iP.innerHTML = iSlider.value;");
            client.println("iSlider.oninput = function() { iSlider.value = this.value; iP.innerHTML = this.value; }");

            client.println("var dSlider = document.getElementById(\"dSlider\");");
            client.println("var dP = document.getElementById(\"dValue\"); dP.innerHTML = dSlider.value;");
            client.println("dSlider.oninput = function() { dSlider.value = this.value; dP.innerHTML = this.value; }");
            
            client.println("$.ajaxSetup({timeout:2000}); function updateServoPosition(value) { ");
            client.println("$.get(\"/servo?value=\" + value + \"&\"); {Connection: close};}");
            
            client.println("function updateRPMValue(value) {");
            client.println("$.get(\"/rpm?value=\" + value + \"&\"); {Connection: close};}");

            client.println("function updateiValue(value) {");
            client.println("$.get(\"/i?value=\" + value + \"&\"); {Connection: close};}");

            client.println("function updatedValue(value) {");
            client.println("$.get(\"/d?value=\" + value + \"&\"); {Connection: close};}");

            client.println("function updatepValue(value) {");
            client.println("$.get(\"/p?value=\" + value + \"&\"); {Connection: close};}</script>");

            // End page
            client.println("</body></html>");

            // GET data for servo control
            if (header.indexOf("GET /servo?value=") >= 0) {
              posValue = header.substring(header.indexOf('=') + 1, header.indexOf('&'));

              sendServo = posValue.toInt() + 41;
              
              Serial.print("Servo Position: ");
              Serial.println(posValue);
            }

            // GET data for RPM control
            if (header.indexOf("GET /rpm?value=") >= 0) {
              rpmValue = header.substring(header.indexOf('=') + 1, header.indexOf('&'));
              
              
              // Write a charatre to the Slave             
              sendRPM = rpmValue.toInt()/2 + 126;;
              Serial.print("RPM Value: ");
              Serial.println(rpmValue);
/*              if(previousRPM != currentRPM){
                Wire.beginTransmission(SLAVE_ADDR);
                Wire.write(currentRPM);
                Wire.endTransmission();
                Serial.print("RPM Value: ");
              }
              previousRPM = currentRPM;*/
            }

            if (header.indexOf("GET /p?value=") >= 0) {
              pValue = header.substring(header.indexOf('=') + 1, header.indexOf('&'));

              sendP = pValue.toInt();
    
              Serial.print("P Value: ");
              Serial.println(pValue);
             
            }

            if (header.indexOf("GET /i?value=") >= 0) {
              iValue = header.substring(header.indexOf('=') + 1, header.indexOf('&'));

              sendI = iValue.toInt();

              Serial.print("I Value: ");
              Serial.println(iValue);
             
            }

            if (header.indexOf("GET /d?value=") >= 0) {
              dValue = header.substring(header.indexOf('=') + 1, header.indexOf('&'));

              sendD = dValue.toInt();

              Serial.print("D Value: ");
              Serial.println(dValue);
             
            }

            // The HTTP response ends with another blank line
            client.println();

            // Break out of the while loop
            break;

          } else {
            // New line is received, clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
