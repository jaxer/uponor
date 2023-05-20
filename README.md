# What is this

Custom ESPHome component that allows connecting Uponor floor heating to Home Assistant.

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

There are two separate controllers at my property, each has 4 thermoemters connected to and a set of actuators.
So i had to create two individual ESPHome devices to hook both controllers to Home Assistant.

First device is running ESP-01 and second running ESP-32. So it is not important which ESP device you choose.
Both are connected to rs485 [module from aliexpress](https://www.aliexpress.com/item/32817720482.html?spm=a2g0o.order_list.order_list_main.30.789f180253LHvf). You dont need to choose this one specifically, there are plenty of options our there.

![IMG-1880](https://github.com/jaxer/uponor/assets/202096/d74dfc7d-894c-4b46-8b61-cf754bc57608)

All thermometers are connected to same bus consisting of 4 wires:
- A-rs485
- B-rs485
- +5v
- -5v

So you take rs485 module, connect it to UART of ESP device and to A/B data lines. You can use those 5 volts from the Uponor bus (making whole thing very compact). Make sure your controller is 5 volts safe.

