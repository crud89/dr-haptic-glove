#define USE_BINARY_INTERFACE 1          // Set to 0 to use text-based interface for serial command line.
#define TEXT_INTERFACE_BUFFER_SIZE 255  // Buffer size to parse text interface buffers.
#define WRITE_DEBUG_OUTPUT 1            // Set to 0 to disable writing debug to bus.
#define CONNECTED_MOTORS 2              // Set to 12 for full set of motors.
#define USE_BLUETOOTH_LOW_ENERGY 1      // Set to 1 to support establishing wireless BLE connections.

/*
 * Input formats:
 * ASCII/Text: "strength duration strength duration ..." with implicit indexing. Use '\n' to end a line.
 * Binary:
 * [0x00] 4b   - Number of following elements (`n`).
 * [0x04] n*4b - Masked motor setting (see below).
 *
 * Mask: 
 * [0xFF000000] - Motor ID (ignored if invalid)
 * [0x00FF0000] - Vibration strength
 * [0x0000FFFF] - Duration in ms
 */

#if USE_BLUETOOTH_LOW_ENERGY
#include <ArduinoBLE.h>

const char* hapticGloveServiceId = "141806C2-081D-4197-FFFF-98D46AC994BE";
const char* hapticGloveRightHandId = "141806C2-081D-4197-0001-98D46AC994BE";
//const char* hapticGloveLeftHandId = "141806C2-081D-4197-0002-98D46AC994BE";

#if USE_BINARY_INTERFACE
BLECharacteristic hapticGloveRightHand(hapticGloveRightHandId, BLEWrite | BLEIndicate, CONNECTED_MOTORS * 4 + 4, false);
//BLECharacteristic hapticGloveLeftHand(hapticGloveLeftHandId, BLEWrite | BLEIndicate, CONNECTED_MOTORS * 4 + 4, false);
#else // USE_BINARY_INTERFACE
BLEStringCharacteristic hapticGloveRightHand(hapticGloveRightHandId, BLEWrite | BLEIndicate, TEXT_INTERFACE_BUFFER_SIZE);
//BLEStringCharacteristic hapticGloveLeftHand(hapticGloveLeftHandId, BLEWrite | BLEIndicate, TEXT_INTERFACE_BUFFER_SIZE);
#endif // USE_BINARY_INTERFACE

BLEService hapticGloveService(hapticGloveServiceId); 
#endif // USE_BLUETOOTH_LOW_ENERGY

// Setup the global variables for vibration strength and duration
int vibrationStrength[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int vibrationDuration[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int motorPinMap[]       = { 5, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

// Time stamp of last iteration.
unsigned long lastTimeStamp = millis();

#if USE_BINARY_INTERFACE
void parseInput(uint32_t motorsInPackage, const uint32_t* inputBuffer)
{
#if WRITE_DEBUG_OUTPUT
  Serial.print("Received package with ");
  Serial.print(motorsInPackage);
  Serial.println(" motors:");
#endif

  // Package has been fully read. Now parse the values.
  for (int i = 0; i < motorsInPackage; ++i) 
  {
    // Parse by masking out the individual components.
    uint32_t motorIndex = (0xFF000000 & inputBuffer[i]) >> 24u;
    uint32_t strength   = (0x00FF0000 & inputBuffer[i]) >> 16u;
    uint32_t duration   = (0x0000FFFF & inputBuffer[i]);

#if WRITE_DEBUG_OUTPUT
  Serial.print("Motor ");
  Serial.print(motorIndex);
  Serial.print(": Strength = ");
  Serial.print(strength);
  Serial.print(", Duration = ");
  Serial.println(duration);
#endif

    // Ignore invalid motor indices.
    if (motorIndex > CONNECTED_MOTORS)
      continue;

    vibrationStrength[motorIndex] = strength;
    vibrationDuration[motorIndex] = duration;
  }
}

inline void readSerialInputAsync()
{
  // Define input buffer. The maximum size is defined by the number of connected motors.
  static uint32_t motorsInPackage = 0;
  static uint32_t inputBuffer[CONNECTED_MOTORS];
  static int lastMotorRead = 0;

  // If no package is currently read, start by reading the number of motors in the package.
  if (motorsInPackage == 0)
  {
    // If there are less than 4 bytes available, don't read anything yet.
    if (Serial.available() < 4)
      return;

    Serial.readBytes(reinterpret_cast<char*>(&motorsInPackage), 4u);
  }

  // Read until either the buffer is full or there is a newline detected.
  for (lastMotorRead; lastMotorRead < motorsInPackage; ++lastMotorRead)
  {
    // Same as above. If there are less than 4 bytes available, wait for the buffer to fill with the next iteration.
    if (Serial.available() < 4)
      return;
     
    Serial.readBytes(reinterpret_cast<char*>(&inputBuffer[lastMotorRead]), 4u);
  }

  // Parse the input.
  parseInput(motorsInPackage, inputBuffer);
  
  // Reset input buffer for new readings.
  memset(inputBuffer, 0u, CONNECTED_MOTORS);
  lastMotorRead = 0;
  motorsInPackage = 0;
}
#else // USE_BINARY_INTERFACE
void parseInput(char* inputBuffer)
{
#if WRITE_DEBUG_OUTPUT
  Serial.print("Received text input: ");
  Serial.println(inputBuffer);
#endif

  // Parse the line.
  char* token = strtok(inputBuffer, " ");
  
  for (int i = 0; i < CONNECTED_MOTORS; ++i) 
  {
    // Clamp the strength to a range [0..255].
    if (token == NULL)
      break;

    vibrationStrength[i] = constrain(atoi(token), 0, 255);
    token = strtok(NULL, " ");

    // Parse duration token.
    if (token == NULL)
      break;

    vibrationDuration[i] = atoi(token);
    token = strtok(NULL, " ");
  }
}

inline void readSerialInputAsync()
{
  // Define input buffer.
  static char inputBuffer[TEXT_INTERFACE_BUFFER_SIZE];
  static int lastReadPos = 0;

  // Read until either the buffer is full or there is a newline detected.
  for (lastReadPos; lastReadPos < TEXT_INTERFACE_BUFFER_SIZE; ++lastReadPos)
  {
    if (Serial.available() > 0)
    {
      char input = Serial.read();

      // No need to attach the delimiter, only parse the input.
      if (input == '\n')
        break;
      else
        inputBuffer[lastReadPos] = input;
    }
    else
    {
      // Return in order to continue loop, but maybe come back later if there's more data available.
      return;
    }
  }

  // Parse text input.
  parseInput(inputBuffer);

  // Reset input buffer for new readings.
  memset(inputBuffer, '\0', TEXT_INTERFACE_BUFFER_SIZE);
  lastReadPos = 0;
}
#endif // USE_BINARY_INTERFACE

#if USE_BLUETOOTH_LOW_ENERGY
void onBluetoothDeviceConnected(BLEDevice central) 
{
#if WRITE_DEBUG_OUTPUT
  Serial.print("Connected to device: ");
  Serial.println(central.address());
#endif
}

void onBluetoothDeviceDisconnected(BLEDevice central) 
{
#if WRITE_DEBUG_OUTPUT
  Serial.print("Disconnected from device: ");
  Serial.println(central.address());
#endif
}

void onBluetoothDataWritten(BLEDevice central, BLECharacteristic characteristic)
{
#if USE_BINARY_INTERFACE
  // Ignore everything below 4 bytes and everything not a multiple of 4 bytes.
  if (characteristic.valueLength() < 4 || (characteristic.valueLength() % 4) != 0)
  {
#if WRITE_DEBUG_OUTPUT
    Serial.print("Received invalid package. Size was ");
    Serial.print(characteristic.valueLength());
    Serial.println(" bytes.");
#endif

    return;
  }

  // Enable LED.
  digitalWrite(LED_BUILTIN, HIGH);

  const uint32_t* inputBuffer = reinterpret_cast<const uint32_t*>(characteristic.value());
  const uint32_t motorsInPackage = inputBuffer[0];
  inputBuffer++; // Skip counter variable.

  // Parse the input.
  parseInput(motorsInPackage, inputBuffer);
#else // USE_BINARY_INTERFACE
  // Ignore packages that are too large.
  if (characteristic.valueLength() >= TEXT_INTERFACE_BUFFER_SIZE)
  {
#if WRITE_DEBUG_OUTPUT
    Serial.print("Received invalid package. Size was ");
    Serial.print(characteristic.valueLength());
    Serial.println(" characters.");
#endif

    return;
  }

  // Enable LED.
  digitalWrite(LED_BUILTIN, HIGH);
    
  // Read the package.
  char* inputBuffer = strdup(reinterpret_cast<const char*>(characteristic.value()));

  // Parse text input and release the buffer.
  parseInput(inputBuffer);
  free(inputBuffer);
#endif // USE_BINARY_INTERFACE
}
#endif // USE_BLUETOOTH_LOW_ENERGY

void setup()
{
  // Turn on builtin LED to indicate loading.
  // NOTE: If the LED stays turned on, there's an error duing setup.
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Initialize bus at a 9600 Hz baud rate.
  Serial.begin(9600);

  // Set pins for prototype vibration motors and disable them initially.
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  analogWrite(5, 0);
  analogWrite(6, 0);

#if USE_BLUETOOTH_LOW_ENERGY
  // Start Bluetooth service.
  if (!BLE.begin())
  {
    Serial.println("Unable to start BLE module");
    while(1);
  }

  // Initialize and advertise the service.
  BLE.setLocalName("TUC Haptic Glove");
  BLE.setAdvertisedService(hapticGloveService);
  hapticGloveService.addCharacteristic(hapticGloveRightHand);
  //hapticGloveService.addCharacteristic(hapticGloveLeftHand);
  BLE.addService(hapticGloveService);

  // Setup advertisement.
  BLE.setAdvertisedService(hapticGloveService);
  BLE.setAdvertisingInterval(80);
  BLE.setAppearance(0x03C0); // Generic Human Interface Device (HID).
  BLE.advertise();
  
  // Assign event handlers.
  BLE.setEventHandler(BLEConnected, onBluetoothDeviceConnected);
  BLE.setEventHandler(BLEDisconnected, onBluetoothDeviceDisconnected);
  hapticGloveRightHand.setEventHandler(BLEWritten, onBluetoothDataWritten);
#endif

  // Turn of builtin LED.
  digitalWrite(LED_BUILTIN, LOW);
}

void loop()
{
#if USE_BLUETOOTH_LOW_ENERGY
  BLE.poll();
#endif // USE_BLUETOOTH_LOW_ENERGY

  if (Serial.available() > 0)
  {
    // Enable LED.
    digitalWrite(LED_BUILTIN, HIGH);

#if USE_BINARY_INTERFACE
    readSerialInputAsync();
#else
    readSerialInputAsync();
#endif
  }
  
  // Disable LED.
  digitalWrite(LED_BUILTIN, LOW);

#if WRITE_DEBUG_OUTPUT
  /*
  // Print debug output for motor strength and remaining duration.
  for (int i = 0; i < CONNECTED_MOTORS; ++i)
  {
    if (vibrationDuration[i] > 0) 
    {
      Serial.print("Motor ");
      Serial.print(i);
      Serial.print(": Strength = ");
      Serial.print(vibrationStrength[i]);
      Serial.print("\t Duration = ");
      Serial.println(vibrationDuration[i]);
    }
  }
  */
#endif

  // Set motor intensities.
  for (int i = 0; i < CONNECTED_MOTORS; ++i)
  {
    if (vibrationDuration[i] > 0)
      analogWrite(motorPinMap[i], vibrationStrength[0]);
    else
      analogWrite(motorPinMap[i], 0);
  }

  // Update time stamps.
  unsigned long currentTimeStamp = millis();
  unsigned long deltaTime = currentTimeStamp - lastTimeStamp;
  lastTimeStamp = currentTimeStamp;
  
  // Set motor intensities.
  for (int i = 0; i < CONNECTED_MOTORS; ++i)
  {
    if (vibrationDuration[i] > 0)
      vibrationDuration[i] -= deltaTime;
  }

  // Flush bus and delay for 10 ms.
#if WRITE_DEBUG_OUTPUT
  Serial.flush();
#endif

  delay(10);
}