# What is this

Custom ESPHome component that allows connecting Uponor floor heating to Home Assistant.

![image](https://github.com/jaxer/uponor/assets/202096/57bf9eca-d867-4679-8d18-2524251c8cc0)


# Credits

- https://www.eevblog.com/forum/projects/figuring-out-an-rs485-protocol/
- https://github.com/ddsuc3m/uponor-floorheating

# Disclaimers

**Safety:** Working with 220-volt devices can be dangerous. Handle with care and expertise. We're not responsible for any injuries or damages.

**IPR:** This project is purely academic. If any content breaches your Intellectual Property Rights, please let us know. We respect IPR and will promptly address your concerns.

**Warranty:** Implementing the procedures in this project may void your device's warranty. Proceed with caution and at your own risk.

# Hardware that works

I have the followng Uponor controllers and thermometers:
![IMG-1878](https://github.com/jaxer/uponor/assets/202096/ea6d0e04-f4c3-4eab-9e98-0ccf560d3e62)
![IMG-1879](https://github.com/jaxer/uponor/assets/202096/1196b35e-383a-4d22-8dd2-d5e48add2870)

# How its connected

There are two separate controllers at my property, each has 4-5 thermometers connected to and a set of actuators.
So I had to create two individual ESPHome devices to hook both controllers to Home Assistant.

First device is running ESP-01 and second running ESP-32. So it is not important which ESP device you choose.
Both are connected to rs485 [module from aliexpress](https://www.aliexpress.com/item/32817720482.html?spm=a2g0o.order_list.order_list_main.30.789f180253LHvf). You dont need to choose this one specifically, there are plenty of options out there.

![IMG-1880](https://github.com/jaxer/uponor/assets/202096/d74dfc7d-894c-4b46-8b61-cf754bc57608)

All thermometers are connected to same bus consisting of 4 labeled wires:
- A-rs485
- B-rs485
- +5v
- -5v

So you take rs485 module, connect it to UART of ESP device and to A/B data lines. You can use those 5 volts from the Uponor bus (making whole thing very compact). Make sure your controller is 5 volts safe.

# Example of ESPHome configuration

Copy files from this repo into config/esphome and create a new device yaml file similar to the following.
Note that device ID's (0x2BBA, 0x73C5, 0x4DC5, etc) are installation specific. Library is logging a lot of stuff, so you can find those from there.

```yaml
esphome:
  name: uponor_living
  platform: ESP8266
  board: esp01_1m
  includes:
    - custom/uponor/uponor_base_device.hpp
    - custom/uponor/uponor_bus.hpp
    - custom/uponor/uponor_climate.hpp
    - custom/uponor/uponor_sensors.hpp

wifi:
  ...
api:
  ...
ota:
  ...

logger:
  baud_rate: 0

  logs:
    sensor: ERROR

uart:
  id: uart_bus
  tx_pin: 1
  rx_pin: 3
  baud_rate: 19200

custom_component:
  - lambda: |-
      auto bus = new UponorBus(id(uart_bus));
      return {bus};
    components:
      - id: uponor_bus

climate:
  platform: custom
  lambda: |-
    auto cabinet = new UponorClimate(0x2BBA, (UponorBus*) id(uponor_bus));
    auto corridor = new UponorClimate(0x73C5, (UponorBus*) id(uponor_bus));
    auto shower = new UponorClimate(0x4DC5, (UponorBus*) id(uponor_bus));
    auto garage = new UponorClimate(0x13BA, (UponorBus*) id(uponor_bus));
    auto living = new UponorClimate(0x61C5, (UponorBus*) id(uponor_bus));

    return {
      cabinet, corridor, shower, garage, living
    };
  climates:
    - name: "Cabinet Heating"
    - name: "Corridor Heating"
    - name: "Shower Heating"
    - name: "Garage Heating"
    - name: "Living Heating"

sensor:
  platform: custom
  lambda: |-
    auto cabinet = new UponorSensors(0x2BBA, (UponorBus*) id(uponor_bus));
    auto corridor = new UponorSensors(0x73C5, (UponorBus*) id(uponor_bus));
    auto shower = new UponorSensors(0x4DC5, (UponorBus*) id(uponor_bus));
    auto garage = new UponorSensors(0x13BA, (UponorBus*) id(uponor_bus));
    auto living = new UponorSensors(0x61C5, (UponorBus*) id(uponor_bus));
    
    return {
      cabinet->room_temp,cabinet->floor_temp,cabinet->set_temp,cabinet->heating,
      corridor->room_temp,corridor->floor_temp,corridor->set_temp,corridor->heating,
      shower->room_temp,shower->floor_temp,shower->set_temp,shower->heating,
      garage->room_temp,garage->floor_temp,garage->set_temp,garage->heating,
      living->room_temp,living->floor_temp,living->set_temp,living->heating,
    };
  sensors:
    - name: "Cabinet Room Temperature"
      unit_of_measurement: °C
      accuracy_decimals: 1
      device_class: temperature
    - name: "Cabinet Floor Temperature"
      unit_of_measurement: °C
      accuracy_decimals: 1
      device_class: temperature
    - name: "Cabinet Set Temperature"
      unit_of_measurement: °C
      accuracy_decimals: 1
      device_class: temperature
    - name: "Cabinet Heating On"

    - name: "Corridor Room Temperature"
      unit_of_measurement: °C
      accuracy_decimals: 1
      device_class: temperature
    - name: "Corridor Floor Temperature"
      unit_of_measurement: °C
      accuracy_decimals: 1
      device_class: temperature
    - name: "Corridor Set Temperature"
      unit_of_measurement: °C
      accuracy_decimals: 1
      device_class: temperature
    - name: "Corridor Heating On"

    - name: "Shower Room Temperature"
      unit_of_measurement: °C
      accuracy_decimals: 1
      device_class: temperature
    - name: "Shower Floor Temperature"
      unit_of_measurement: °C
      accuracy_decimals: 1
      device_class: temperature
    - name: "Shower Set Temperature"
      unit_of_measurement: °C
      accuracy_decimals: 1
      device_class: temperature
    - name: "Shower Heating On"

    - name: "Garage Room Temperature"
      unit_of_measurement: °C
      accuracy_decimals: 1
      device_class: temperature
    - name: "Garage Floor Temperature"
      unit_of_measurement: °C
      accuracy_decimals: 1
      device_class: temperature
    - name: "Garage Set Temperature"
      unit_of_measurement: °C
      accuracy_decimals: 1
      device_class: temperature
    - name: "Garage Heating On"

    - name: "Living Room Temperature"
      unit_of_measurement: °C
      accuracy_decimals: 1
      device_class: temperature
    - name: "Living Floor Temperature"
      unit_of_measurement: °C
      accuracy_decimals: 1
      device_class: temperature
    - name: "Living Set Temperature"
      unit_of_measurement: °C
      accuracy_decimals: 1
      device_class: temperature
    - name: "Living Heating On"
```

You can read more about how custom components work [in ESPHome documentation](https://esphome.io/custom/index.html)
