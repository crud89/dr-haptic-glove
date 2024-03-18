# Haptic Glove for Interactive Stereoscopic Diminished Reality

This project contains the sources for our tactile feedback glove for diminished reality. The `sketch` directory contains the sources for programming the glove controller. It has been tested on an Arduino Uno R4 Wifi. The `clients` directory contain sources for different clients. The `serialcom` client can send packages over a serial interface (USB) for testing. The `gattcom` interface requires a Bluetooth Low Energy (BLE) compatible interface on the system. It attempts to find the device multiple times before establishing a connection, after which it sends out random package payloads to the glove. Pressing `Esc` will exit the client.

## License

The project is licensed under [MIT conditions](./LICENSE).