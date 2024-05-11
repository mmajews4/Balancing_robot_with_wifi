#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <MPU6050.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
MPU6050 mpu;
 
// Setup timers and temp variables
volatile bool flag = false;
int temp;

// MPU6050 vairables
int16_t ax, ay, az, gx, gy, gz;
 
// Display counter
int displaycount = 0;

// Motor Connections (ENA & ENB must use PWM pins)
const int IN1 = 9;
const int IN2 = 8;
const int IN3 = 7;
const int IN4 = 6;
const int ENA = 10;
const int ENB = 5;

// PID variables
float vP = 80;     
float vI = 0.005;                     //Niewgrywane oryginalnie 0.1
float vD = 300;
float targetValue = 0;
float xP, xI, xD, currentValue, integralSum, currError, lastError;
float sum, throttle, throttleA, throttleB; 
float duration;

// ESP32 sending variables
int turn = 0;
float angleCompensation;
uint8_t sentServo = 41; 
uint8_t sentRPM = 126; 
uint8_t sentP, sentI, sentD;
uint16_t sendCurrentValue, sendthrottle, sendxP, sendxI, sendxD;

void IRAM_ATTR onTimer() {
  flag = true; // Set flag to indicate it's time to take a measurement
}

void setup() {

  // Start Serial Monitor                                                 
  Serial.begin(115200);
  Wire.begin();

  lcd.setCursor(12,0);                                                  // Status check 0
  lcd.print("o"); 

  // MPU init
  mpu.initialize();
  mpu.setDLPFMode(6); // Set digital low-pass filter (DLPF) mode
  mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_250); // Set gyro full scale range

  lcd.setCursor(12,0);                                                  // Status check 1 - mpu
  lcd.print("m"); 

  // Lcd init
  lcd.init();
  lcd.backlight();
  lcd.clear();

  // Timer init
  // Set up Timer1 for 10ms interrupt
  TCCR1A = 0; // Reset timer control register A
  TCCR1B = 0; // Reset timer control register B
  TCNT1 = 0;  // Reset timer counter
  OCR1A = 3125; // Set compare match register for 10ms @ 16MHz -10ms 1562  -20ms 3125
  TCCR1B |= (1 << WGM12); // Enable CTC mode
  TCCR1B |= (1 << CS11) | (1 << CS10); // Set prescaler to 64 and start the timer
  TIMSK1 |= (1 << OCIE1A); // Enable timer compare interrupt

  // Set motor connections as outputs
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
 
  // Start with motors off
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);

  lcd.setCursor(12,0);                                                  // Status check 2 - all initilized
  lcd.print("i");                                          
}

//  ---------------------------------------------------------   E S P     C O M M U N I C A T I O N   -----
void read_ESP32(){ 

  if(currentValue < -12.5) currentValue = -12.5;
  if(currentValue > 12.5) currentValue = 12.5;

  sendCurrentValue = currentValue * 100 / 5 + 126 ;
  sendthrottle = throttle/5 + 126;
  sendxP = xP/5 + 126;
  sendxI = xI/5 + 126;
  sendxD = xD/5 + 126;

  //Request 5 bytes from esp32                                 
  Wire.requestFrom(0x66,5);    
  //Wait until all the bytes are received                                       
  while(Wire.available() < 5);
 
  sentServo = Wire.read();                      
  sentRPM = Wire.read();                       
  sentP = Wire.read();                    
  sentI = Wire.read();                      
  sentD = Wire.read();  
  
  turn = sentServo - 41;
  angleCompensation = (sentRPM - 126)*0.1;
  vP = sentP;  
  vI = sentI * 0.001;
  vD = sentD * 5;
                                 
  Wire.beginTransmission(0x66);  
  //Send the requested starting register                                      
  Wire.write(sendCurrentValue);
  Wire.write(sendthrottle);
  Wire.write(sendxP);
  Wire.write(sendxI);
  Wire.write(sendxD);
  //End the transmission                                                    
  Wire.endTransmission(); 
  
}

void loop(){  // I could devide it into seperat functions, but I didn't because of esier readability

  if(flag){

    // ------------------------------------------------------  A N G L E     R E A D   --------

    // Read raw accelerometer and gyroscope data
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    // Convert raw data to real-world values (acceleration in g, angular velocity in degrees per second)
    float accelX = ax / 16384.0; // 16384 LSB/g for the accelerometer
    float accelY = ay / 16384.0;
    float accelZ = az / 16384.0;
    float gyroX = gx / 131.0; // 131 LSB/(degrees/s) for the gyroscope
    float gyroY = gy / 131.0;
    float gyroZ = gz / 131.0;

    // Calculate pitch and roll angles using sensor fusion algorithm (e.g., complementary filter)
    //float pitch = atan2(-accelX, sqrt(accelY * accelY + accelZ * accelZ)) * RAD_TO_DEG;
    float roll = atan2(accelY, sqrt(accelX * accelX + accelZ * accelZ)) * RAD_TO_DEG;

    Serial.println(roll);

    // -----------------------------------------------------------------   P I D  ----------
  
    currentValue = roll;
    
    xP = (targetValue - currentValue)* vP * 0.5;
    integralSum += (targetValue - currentValue);
    xI = integralSum * vI;
    currError = (targetValue - currentValue);
    xD = (currError - lastError)* vD;
    lastError = currError;

    sum = xP + xI + xD;

    if(sum < 5 && sum > -5){
      throttle = sum;
    }else if(sum > 5){
      throttle = sum + 35;
    }else if(sum < -5){
      throttle = sum - 35;
    }

    if(throttle < -255) throttle = -255;
    if(throttle > 255) throttle = 255;

    throttleA = (throttle + turn)*0.9;  // Motor power compensation
    if(throttleA < -255) throttleA = -255;
    if(throttleA > 255) throttleA = 255;

    //lcd.setCursor(13,0); 
    //lcd.print(".");
    
    //  -----------------------------------------------------   M O T O R S     A C T I O N   ----------
    if(throttleA <= 0){
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
    } else {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
    }
    analogWrite(ENA, abs(throttleA));
    
    throttleB = throttle - turn;
    if(throttleB < -255) throttleB = -255;
    if(throttleB > 255) throttleB = 255;

    if(throttleB <= 0){
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
    } else {
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
    }
    analogWrite(ENB, abs(throttleB));

    // Increment the display counter
    displaycount = displaycount +1;
    
    //  -----------------------------------------------   L C D     S T A T U S     C H E C K   ----------
    if (displaycount > 200) {

      lcd.clear();
      
//      lcd.setCursor(0,1); 
//      lcd.print(sentP);
//      lcd.setCursor(12,1); 
//      lcd.print(xP);*/
//      lcd.setCursor(0,0); 
//      lcd.print(currentValue);
      lcd.setCursor(0,1); 
      lcd.print(throttle);
//      lcd.setCursor(5,1); 
//      lcd.print(throttleA);
//      lcd.setCursor(10,1); 
//      lcd.print(throttleB);
//      lcd.setCursor(0,0); 
//      lcd.print(turn);
//      lcd.setCursor(5,0); 
//      lcd.print(sentServo);
      lcd.setCursor(0,0); 
      lcd.print(roll);
      
    
//      lcd.setCursor(4,1); 
//      lcd.print(" ");
//      lcd.setCursor(11,1); 
//      lcd.print(" ");
//      lcd.setCursor(4,0); 
//      lcd.print(" ");
    
      //   ------------------------------------------   R E A D     E S P   --------------
      //read_ESP32();
    
      displaycount = 0;
      flag = false; // Reset the flag
    }
  }
}

// ---------------------------------------------   T I M E R     F U N C T I O N   ----------
ISR(TIMER1_COMPA_vect) {
  flag = true; // Set flag to indicate it's time to take a measurement
}
