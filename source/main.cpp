/* mbed Microcontroller Library
 * Copyright (c) 2006-2015 ARM Limited
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

#include <events/mbed_events.h>
#include <mbed.h>
#include "ble/BLE.h"
#include "ble/Gap.h"
#include "ble/services/HeartRateService.h"
#include "ble/services/HealthThermometerService.h"
#include "temp_hum.h"


#define I2C_SDA p26
#define I2C_SCL p27

DigitalOut led1(LED1, 1);
I2C i2c(I2C_SDA, I2C_SCL);
Serial pc(USBTX, USBRX);

const int addr8bit = 0x40 << 1;
const char TempCmd = 0xF3;
const char HumCmd = 0xF5;
const char WriteRegCmd = 0xE6;
const char ReadRegCmd = 0xE7;
const char SoftResetCmd = 0xFE;
char data[3];

const static char     DEVICE_NAME[] = "HRM";
static const uint16_t uuid16_list[] = {GattService::UUID_HEALTH_THERMOMETER_SERVICE};                              //UUID_HEART_RATE_SERVICE

static TempHumService *temphumServicePtr;
static float tempCounter = 20;

static EventQueue eventQueue(/* event count */ 16 * EVENTS_EVENT_SIZE);

void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *params)
{
    BLE::Instance().gap().startAdvertising(); // restart advertising
}

float MeasureTemp()
{
    int temp_meas;
    float temp;
    data[0] = TempCmd;
    i2c.write(addr8bit, data, 1,true);
    wait(0.5);
    i2c.read(addr8bit, data, 3);
    temp_meas = (data[0] << 8) | data[1];
    temp = -46.85 + (175.72*((float)temp_meas/65536.00));
    pc.printf("Temp is %02f\r\n",temp);
    return temp;
}

float MeasureHumidity()
{
    int hum_meas;
    float humidity;
    data[0] = HumCmd;
    i2c.write(addr8bit, data, 1,true);
    wait(0.5);
    i2c.read(addr8bit, data, 3);
    hum_meas = (data[0] << 8) | data[1];
    humidity = -6 + (125*((float)hum_meas/65536.00));
    pc.printf("Humidity is %02f\r\n",humidity);
    return humidity;
}

void updateSensorValue() {
    float temp;
    float humidity;
    temp = MeasureTemp();
    humidity = MeasureHumidity();
    temphumServicePtr->updateHumidity(humidity);
    temphumServicePtr->updateTemperature(temp);
}

void periodicCallback(void)
{
    led1 = !led1; /* Do blinky on LED1 while we're waiting for BLE events */

    if (BLE::Instance().getGapState().connected) {                         //
        eventQueue.call(updateSensorValue);
    }
}

void onBleInitError(BLE &ble, ble_error_t error)
{
    (void)ble;
    (void)error;
   /* Initialization error handling should go here */
}

void printMacAddress()
{
    /* Print out device MAC address to the console*/
    Gap::AddressType_t addr_type;
    Gap::Address_t address;
    BLE::Instance().gap().getAddress(&addr_type, address);
    printf("DEVICE MAC ADDRESS: ");
    for (int i = 5; i >= 1; i--){
        printf("%02x:", address[i]);
    }
    printf("%02x\r\n", address[0]);
}

void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
    BLE&        ble   = params->ble;
    ble_error_t error = params->error;

    if (error != BLE_ERROR_NONE) {
        onBleInitError(ble, error);
        return;
    }

    if (ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
        return;
    }

    ble.gap().onDisconnection(disconnectionCallback);

    /* Setup primary service. */
    temphumServicePtr = new TempHumService(ble);

    /* Setup advertising. */
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);                                      //Flags
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid16_list, sizeof(uuid16_list));                           //Data Type
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::GENERIC_THERMOMETER);                                                                                    //Appearance    //GENERIC_HEART_RATE_SENSOR,
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));                                       //Data Type
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.gap().setAdvertisingInterval(1000); /* 1000ms */
    ble.gap().startAdvertising();

    printMacAddress();
}

void scheduleBleEventsProcessing(BLE::OnEventsToProcessCallbackContext* context) {
    BLE &ble = BLE::Instance();
    eventQueue.call(Callback<void()>(&ble, &BLE::processEvents));
}

void reset_sensor()
{
    data[0] = SoftResetCmd;
    i2c.write(addr8bit, data, 1);
}

char read_reg()
{
    data[0] = ReadRegCmd;
    i2c.write(addr8bit, data, 1);    
    i2c.read(addr8bit, data, 1);
    return data[0];
}   

void write_reg(char rec)
{
    data[1] = rec & 0x7E;
    data[0] = WriteRegCmd;
    i2c.write(addr8bit, data, 2);
}

void init_sensor()
{
    char receive;
    
    wait(1);      
    reset_sensor();       
    wait(1);
    
    receive = read_reg();
    write_reg(receive);
}

int main()
{
        
    init_sensor();
    eventQueue.call_every(500, periodicCallback);

    BLE &ble = BLE::Instance();
    ble.onEventsToProcess(scheduleBleEventsProcessing);
    ble.init(bleInitComplete);

    eventQueue.dispatch_forever();

    return 0;
}
