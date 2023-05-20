class UponorBaseDevice {
public:
  typedef struct __attribute__((packed)) header_st {
    uint8_t sensor_type; // 0x10 or 0x11
    uint8_t unknown05;   // 0x05
    uint16_t sensor_address;
  } header_t;

  typedef struct __attribute__((packed)) pair_st {
    uint8_t key;
    uint16_t value;
  } pair_t;

  UponorBaseDevice(uint16_t address_) { address = address_; }

  bool process_packet(const std::vector<uint8_t> &packet,
                      const bool log = false) {
    const header_t *header = (header_t *)&packet[0];

    if (header->sensor_address != address) {
      return false;
    }

    pair_t *pair = (pair_t *)&packet[4];

    // skip station request
    if (pair->key == 0xFF) {
      if (log)
        ESP_LOGI(TAG, "Station request. Skipping.");
      return true;
    }

    uint8_t pairs_count = (packet.size() - 4 - 2) / 3;
    while (pairs_count--) {
      process_pair(pair, log);
      pair++;
    }

    return true;
  }

  virtual void do_publish();

protected:
  const char *TAG = "uponor";

  uint16_t address;

  typedef struct __attribute__((packed)) command_packet {
    uint8_t sensor_type; // 0x10 or 0x11
    uint8_t unknown05;   // 0x05
    uint16_t sensor_address;
    uint8_t command1; // 0x3B
    uint16_t value1;  // floor temp? farenheit 0x0000
    uint8_t command2; // 0x3B
    uint16_t value2;  // room temp farenheit
    uint16_t crc;
  } command_packet_t;

  virtual void process_current_temperature(const float value);
  virtual void process_floor_temperature(const float value);
  virtual void process_target_temperature(const float value);
  virtual void process_target_min_temperature(const float value);
  virtual void process_target_max_temperature(const float value);
  virtual void process_heating_state(const bool heating);
  virtual void process_flags(const uint16_t flags);

  void add_command_defaults(command_packet_t &packet) {
    packet.sensor_type = 0x11;
    packet.unknown05 = 0x05;
    packet.sensor_address = 0x0000;
    packet.command1 = 0x3B;
    packet.value1 = 0x0000;
    packet.command2 = 0x3B;
    packet.value2 = 0x0000;
    packet.crc = 0x0000;
  }

  void add_command_crc(command_packet_t &packet) {
    packet.crc = crc16((uint8_t *)&packet, sizeof(command_packet_t) - 2);
  }

  std::vector<uint8_t> create_raw_packet(command_packet_t &packet) {
    std::vector<uint8_t> v;
    auto size = sizeof(command_packet_t);
    v.reserve(size);
    uint8_t *c = (uint8_t *)&packet;
    while (size--) {
      v.push_back(*c++);
    }
    return v;
  }

  uint16_t celsius_to_raw(float celsius) {
    float farenheit = (celsius * 9.0) / 5.0 + 32;
    int dec = farenheit * 10;
    return flip_bytes(dec);
  }

private:
  void process_pair(const pair_t *pair, const bool log = false) {
    // Each pair has a key (8bit) and value (16bit).
    // Known pair types:

    // FF - Request from station, not a pair

    // 0C - 2400
    // 2D - FF7F (if r0 then FE7F)

    // 35 - 0080 = defrost-no, 0000 = defrost-yes
    // 37 - Min temp
    // 38 - Max temp
    // 3B - Set temp
    // 3C - Eco Temp: 00 01 = 1 = 0.0C; 00 09 = 9 = 0.5C; 00 12 = 18 = 1.0C; 00
    // 48 = 72 = 4.0C; 00 C6 = 198 = 11.0C
    // 3D - Heating or not? 0x4000 = yes 0x0000 = no
    // 3E - =0200 in rft. 0000 otherwise.
    // 3F - Flags (020C = 1000001100) 3rd bit from right is yes/no defrost.
    //      rightmost 2 bits: 00 = rt, 01 = rft, 11 = rS, 10 = rO
    // 40 - Room temp
    // 41 - Secondary Temp (Floor)

    if (pair->key == 0x40) {
      auto temp = raw_to_celsius(pair->value);
      process_current_temperature(temp);

      if (log)
        ESP_LOGI(TAG, "Room Temp (%02X) = %04X = %f", pair->key, pair->value,
                 temp);
    } else if (pair->key == 0x41) {
      auto temp = raw_to_celsius(pair->value);
      process_floor_temperature(temp);

      if (log)
        ESP_LOGI(TAG, "Floor Temp (%02X) = %04X = %f", pair->key, pair->value,
                 temp);
    } else if (pair->key == 0x37) {
      auto temp = raw_to_celsius(pair->value);
      process_target_min_temperature(temp);

      if (log)
        ESP_LOGI(TAG, "Min Temp (%02X) = %04X = %f", pair->key, pair->value,
                 temp);
    } else if (pair->key == 0x38) {
      auto temp = raw_to_celsius(pair->value);
      process_target_max_temperature(temp);

      if (log)
        ESP_LOGI(TAG, "Max Temp (%02X) = %04X = %f", pair->key, pair->value,
                 temp);
    } else if (pair->key == 0x3B) {
      auto temp = round(raw_to_celsius(pair->value) * 2) / 2;
      if (temp < 100) {
        process_target_temperature(temp);
      }

      if (log)
        ESP_LOGI(TAG, "Target Temp (%02X) = %04X = %f", pair->key, pair->value,
                 temp);
    } else if (pair->key == 0x3C) {
      if (log)
        ESP_LOGI(TAG, "Eco (%02X) = %04X = %f", pair->key, pair->value,
                 convert_eco(pair->value));
    } else if (pair->key == 0x3F) {
      process_flags(pair->value);

      if (log)
        ESP_LOGI(TAG,
                 "Flags (%02X) = %04X. Frost flag = %d. " BYTE_TO_BINARY_PATTERN
                 " " BYTE_TO_BINARY_PATTERN,
                 pair->key, pair->value, (bool)(pair->value & (1 << 2)),
                 BYTE_TO_BINARY(pair->value >> 8), BYTE_TO_BINARY(pair->value));
    } else if (pair->key == 0x3D) {
      bool on = (bool)(pair->value & (1 << 14));
      process_heating_state(on);
      if (log)
        ESP_LOGI(TAG, "Heating (%02X) = %04X. On = %d", pair->key, pair->value,
                 on);
    } else {
      if (log)
        ESP_LOGI(TAG, "Key %02X = %04X", pair->key, pair->value);
    }
  }

  uint16_t flip_bytes(uint16_t i) {
    uint16_t hi = (i & 0xff00) >> 8;
    uint16_t lo = (i & 0xff);
    return (lo << 8) | hi;
  }

  float raw_to_celsius(const uint16_t raw) {
    return 5.0f * ((float)flip_bytes(raw) / 10.0f - 32.0f) / 9.0f;
  }

  float convert_eco(const uint16_t t) {
    uint16_t flipped = flip_bytes(t);
    return (float)flipped / 9.0f * 0.5;
  }
};
