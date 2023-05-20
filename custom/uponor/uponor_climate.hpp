class UponorClimate : public Climate, public UponorBaseDevice {
private:
  UponorBus *bus;
  bool rft = false;
  float floor_temp = 0;
  float air_temp = 0;
  float target = 0;
  float target_min = 0;
  float target_max = 0;

public:
  UponorClimate(uint16_t address, UponorBus *bus_)
      : Climate(), UponorBaseDevice(address), bus(bus_) {
    bus->register_device(this);
  }

  void control(const ClimateCall &call) override {
    if (call.get_target_temperature().has_value()) {
      // User requested target temperature change
      float temp = *call.get_target_temperature();

      ESP_LOGD(TAG, "Got request to change target temp for device %u to %f",
               address, temp);

      command_packet_t packet;

      add_command_defaults(packet);

      packet.sensor_address = address;
      packet.value2 = celsius_to_raw(temp);

      add_command_crc(packet);

      std::vector<uint8_t> raw_packet = create_raw_packet(packet);

      bus->send_raw_packet(raw_packet);
    }
  }

  ClimateTraits traits() override {
    auto traits = climate::ClimateTraits();
    traits.set_supports_current_temperature(true);
    traits.set_supports_action(true);
    traits.set_supported_modes({CLIMATE_MODE_HEAT});
    return traits;
  }

  void process_current_temperature(const float value) override {
    air_temp = value;
  }

  void process_target_temperature(const float value) override {
    target = value;
  }

  void process_target_min_temperature(const float value) override {
    target_min = value;
  }

  void process_target_max_temperature(const float value) override {
    target_max = value;
  }

  void process_heating_state(const bool heating) override {
    action = heating ? CLIMATE_ACTION_HEATING : CLIMATE_ACTION_OFF;
  }

  void process_floor_temperature(const float value) override {
    floor_temp = value;
  }

  void process_flags(const uint16_t flags) override { rft = (flags & 3) == 1; }

  void do_publish() override {
    mode = CLIMATE_MODE_HEAT;

    current_temperature = air_temp;
    target_temperature = target;

    publish_state();
  }
};
