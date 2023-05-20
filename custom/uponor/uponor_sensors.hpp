class UponorSensors : public UponorBaseDevice {
public:
  Sensor *room_temp = new Sensor();
  Sensor *set_temp = new Sensor();
  Sensor *heating = new Sensor();
  Sensor *floor_temp = new Sensor();

  UponorSensors(uint16_t address, UponorBus *bus) : UponorBaseDevice(address) {
    bus->register_device(this);
  }

  void process_current_temperature(const float value) override {
    room_temp->raw_state = value;
  }

  void process_target_temperature(const float value) override {
    set_temp->raw_state = value;
  }

  void process_target_min_temperature(const float value) override {}

  void process_target_max_temperature(const float value) override {}

  void process_heating_state(const bool on) override {
    heating->raw_state = on;
  }

  void process_floor_temperature(const float value) override {
    floor_temp->raw_state = value;
  }

  void process_flags(const uint16_t flags) override {}

  void do_publish() override {
    room_temp->publish_state(room_temp->raw_state);
    set_temp->publish_state(set_temp->raw_state);
    heating->publish_state(heating->raw_state);
    floor_temp->publish_state(floor_temp->raw_state);
  }
};
