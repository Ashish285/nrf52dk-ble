/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __BLE_TEMP_HUM_SERVICE_H__
#define __BLE_TEMP_HUM_SERVICE_H__

#include "ble/BLE.h"

/**
* @class EnvironmentalService
* @brief BLE TemHum Service. This service provides temperature, humidity measurement.
*/
class TempHumService {
public:

    typedef uint16_t HumidityType_t;
    typedef int16_t TemperatureType_t;
    /**
     * @brief   TempHum constructor.
     * @param   _ble Reference to BLE device.
     */
    TempHumService(BLE& _ble) :
        ble(_ble),
        temperatureCharacteristic(GattCharacteristic::UUID_TEMPERATURE_CHAR, &temperature, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY),
        humidityCharacteristic(GattCharacteristic::UUID_HUMIDITY_CHAR, &humidity, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY)
    {
        static bool serviceAdded = false; /* We should only ever need to add the information service once. */
        if (serviceAdded) {
            return;
        }

        GattCharacteristic *charTable[] = { &humidityCharacteristic,
                                            &temperatureCharacteristic };

        GattService TempHumService(0x2000, charTable, sizeof(charTable) / sizeof(GattCharacteristic *));

        ble.gattServer().addService(TempHumService);
        serviceAdded = true;
    }

    /**
     * @brief   Update humidity characteristic.
     * @param   newHumidityVal New humidity measurement.
     */
    void updateHumidity(float newHumidityVal)
    {
        humidity = (HumidityType_t)(newHumidityVal * 100);
        ble.gattServer().write(humidityCharacteristic.getValueHandle(), (uint8_t *)&humidity, sizeof(HumidityType_t));
    }

    /**
     * @brief   Update temperature characteristic.
     * @param   newTemperatureVal New temperature measurement.
     */
    void updateTemperature(float newTemperatureVal)
    {
        temperature = (TemperatureType_t)(newTemperatureVal * 100);
        ble.gattServer().write(temperatureCharacteristic.getValueHandle(), (uint8_t *)&temperature, sizeof(TemperatureType_t));
    }

private:
    BLE& ble;

    TemperatureType_t temperature;
    HumidityType_t humidity;

    ReadOnlyGattCharacteristic<TemperatureType_t>    temperatureCharacteristic;   
    ReadOnlyGattCharacteristic<HumidityType_t>    humidityCharacteristic;
};

#endif /* #ifndef __BLE_TEMP_HUM_SERVICE_H__*/