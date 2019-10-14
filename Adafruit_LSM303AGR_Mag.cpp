/***************************************************************************
  This is a library for the LSM303AGR Magnentometer/compass

  Designed specifically to work with the Adafruit LSM303AGR Breakout

  These displays use I2C to communicate, 2 pins are required to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Bryan Siepert for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <Wire.h>

#include <limits.h>

#include "Adafruit_LSM303AGR_Mag.h"

/***************************************************************************
 MAGNETOMETER
 ***************************************************************************/
/***************************************************************************
 PRIVATE FUNCTIONS
 ***************************************************************************/

/**************************************************************************/
/*!
    @brief  Reads the raw data from the sensor
*/
/**************************************************************************/
void Adafruit_LSM303AGR_Mag_Unified::read() {

  Adafruit_BusIO_Register data_reg =
      Adafruit_BusIO_Register(i2c_dev, LSM303AGR_OUTX_L_REG, 6);

  uint16_t buffer[3];
  data_reg.read((uint8_t *)buffer, 6);

  raw.x = buffer[0];
  raw.y = buffer[1];
  raw.z = buffer[2];
}

/***************************************************************************
 CONSTRUCTOR
 ***************************************************************************/

/**************************************************************************/
/*!
    @brief  Instantiates a new Adafruit_LSM303 class
    @param sensorID an option ID to differentiate the sensor from others
*/
/**************************************************************************/
Adafruit_LSM303AGR_Mag_Unified::Adafruit_LSM303AGR_Mag_Unified(
    int32_t sensorID) {
  _sensorID = sensorID;

  // Clear the raw mag data
  raw.x = 0;
  raw.y = 0;
  raw.z = 0;
}

/***************************************************************************
 PUBLIC FUNCTIONS
 ***************************************************************************/
/*!
 *    @brief  Sets up the hardware and initializes I2C
 *    @param  i2c_address
 *            The I2C address to be used.
 *    @param  wire
 *            The Wire object to be used for I2C connections.
 *    @return True if initialization was successful, otherwise false.
 */
bool Adafruit_LSM303AGR_Mag_Unified::begin(uint8_t i2c_address, TwoWire *wire)

{
  // Enable I2C
  i2c_dev = new Adafruit_I2CDevice(i2c_address, wire);

  if (!i2c_dev->begin()) {
    Serial.println("Failed to init i2c address");
    return false;
  }

  // Check connection
  Adafruit_BusIO_Register chip_id =
      Adafruit_BusIO_Register(i2c_dev, LSM303AGR_WHO_AM_I, 1);

  // make sure we're talking to the right chip
  if (chip_id.read() != _CHIP_ID) {
    Serial.print("chip id is ");
    Serial.println(chip_id.read());
    // No LSM303AGR detected ... return false
    return false;
  }

  config_a = new Adafruit_BusIO_Register(i2c_dev, LSM303AGR_CFG_REG_A, 1);

  // enable int latching
  reset();

  return true;
}
/*!
 *    @brief  Resets the sensor to an initial state
 */
void Adafruit_LSM303AGR_Mag_Unified::reset(void) {

  Adafruit_BusIO_Register config_c =
      Adafruit_BusIO_Register(i2c_dev, LSM303AGR_CFG_REG_C, 1);

  Adafruit_BusIO_RegisterBits reset =
      Adafruit_BusIO_RegisterBits(config_a, 1, 5);

  Adafruit_BusIO_RegisterBits reboot =
      Adafruit_BusIO_RegisterBits(config_a, 1, 6);

  Adafruit_BusIO_RegisterBits mode =
      Adafruit_BusIO_RegisterBits(config_a, 1, 0);

  Adafruit_BusIO_RegisterBits data_rate =
      Adafruit_BusIO_RegisterBits(config_a, 2, 2);

  Adafruit_BusIO_RegisterBits temp_compensation =
      Adafruit_BusIO_RegisterBits(config_a, 1, 7);

  Adafruit_BusIO_RegisterBits bdu =
      Adafruit_BusIO_RegisterBits(&config_c, 1, 4);

  reset.write(1);
  delay(100);
  reboot.write(1);
  delay(100);
  bdu.write(1);
  temp_compensation.write(1);

  mode.write(0x00); // set to continuous mode

  // self._int_latched = True
  // self._int_reg_polarity = True
  // self._int_iron_off = False
  // self._interrupt_pin_putput = True
}

/**************************************************************************/
/*!
    @brief  Sets the magnetometer's update rate
    @param rate The new `lsm303AGRMagRate` to set
*/
/**************************************************************************/
void Adafruit_LSM303AGR_Mag_Unified::setDataRate(lsm303AGRMagRate rate) {
  Adafruit_BusIO_RegisterBits data_rate =
      Adafruit_BusIO_RegisterBits(config_a, 2, 2);

  data_rate.write(rate);
}

/**************************************************************************/
/*!
    @brief  Gets the magnetometer's update rate
    @returns The current data rate as a `lsm303AGRMagRate`
*/
/**************************************************************************/
lsm303AGRMagRate Adafruit_LSM303AGR_Mag_Unified::getDataRate(void) {
  Adafruit_BusIO_RegisterBits data_rate =
      Adafruit_BusIO_RegisterBits(config_a, 2, 2);

  return (lsm303AGRMagRate)data_rate.read();
}

/**************************************************************************/
/*!
    @brief  Gets the most recent sensor event
    @param event The `sensors_event_t` to fill with event data
    @returns true, always
*/
/**************************************************************************/
bool Adafruit_LSM303AGR_Mag_Unified::getEvent(sensors_event_t *event) {

  /* Clear the event */
  memset(event, 0, sizeof(sensors_event_t));

  /* Read new data */
  read();

  event->version = sizeof(sensors_event_t);
  event->sensor_id = _sensorID;
  event->type = SENSOR_TYPE_MAGNETIC_FIELD;
  event->timestamp = millis();
  event->magnetic.x =
      (float)raw.x * LSM303AGR_MAG_LSB * LSM303AGR_MILLIGAUSS_TO_MICROTESLA;
  event->magnetic.y =
      (float)raw.y * LSM303AGR_MAG_LSB * LSM303AGR_MILLIGAUSS_TO_MICROTESLA;
  event->magnetic.z =
      (float)raw.z * LSM303AGR_MAG_LSB * LSM303AGR_MILLIGAUSS_TO_MICROTESLA;

  return true;
}

/**************************************************************************/
/*!
    @brief  Gets the sensor_t data
*/
/**************************************************************************/
void Adafruit_LSM303AGR_Mag_Unified::getSensor(sensor_t *sensor) {
  /* Clear the sensor_t object */
  memset(sensor, 0, sizeof(sensor_t));

  /* Insert the sensor name in the fixed length char array */
  strncpy(sensor->name, "LSM303AGR Mag", sizeof(sensor->name) - 1);
  sensor->name[sizeof(sensor->name) - 1] = 0;
  sensor->version = 1;
  sensor->sensor_id = _sensorID;
  sensor->type = SENSOR_TYPE_MAGNETIC_FIELD;
  sensor->min_delay = 0;
  sensor->max_value = 0.0F;  // TBD
  sensor->min_value = 0.0F;  // TBD
  sensor->resolution = 0.0F; // TBD
}