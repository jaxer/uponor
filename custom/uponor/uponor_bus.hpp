class UponorBus : public Component, public UARTDevice, public CustomAPIDevice {
public:
  UponorBus(UARTComponent *parent)
      : Component(), UARTDevice(parent), CustomAPIDevice() {}

  void register_device(UponorBaseDevice *device) { devices.push_back(device); }

  void setup() override { packet.reserve(maxPacketSize); }

  void loop() override {
    const auto now = millis();
    const auto since_last_char_ms = now - last_char_at;

    if ((packet.size() > 0) && (since_last_char_ms > 50)) {
      ESP_LOGW(
          TAG,
          "Packet time expired, resetting buffers. Crc: %04X. Packet size: %u",
          crc, packet.size());
      log_as_hex(true, packet.data(), packet.size());

      packet.clear();
    }

    // controller polls devices every 10 seconds, fit our packets in between

    if (!packets_to_send.empty() && (since_last_char_ms > 150) &&
        (since_last_char_ms < 9500)) {
      ESP_LOGW(TAG, "Sending packet now.");

      auto packet_to_send = packets_to_send.front();
      packets_to_send.pop_front();

      log_as_hex(true, packet_to_send.data(), packet_to_send.size());
      verify_crc(packet_to_send.data(), packet_to_send.size());

      write_array(packet_to_send.data(), packet_to_send.size());

      ESP_LOGW(TAG, "Packet sent.");

      last_char_at = now;
    }

    if (since_last_char_ms > 500 && session_in_progress) {
      session_in_progress = false;
      for (auto *device : devices) {
        device->do_publish();
      }
    }

    while (available()) {
      session_in_progress = true;
      char ch = read();

      parse_result_t parse_result = parse_packet(ch);

      if (parse_result == DONE) {
        if (verify_crc(packet.data(), packet.size())) {
          process_packet_by_device();
        } else {
          ESP_LOGW(TAG, "CRC Failed! Packet size: %u", packet.size());
          log_as_hex(false, packet.data(), packet.size());
        }
        packet.clear();
      } else if (parse_result == WRONG_DATA) {
        packet.clear();

        ESP_LOGW(TAG, "Wrong data! Packet size: %u", packet.size());
        log_as_hex(true, packet.data(), packet.size());
      }

      last_char_at = now;
    }
  }

  float get_loop_priority() const { return 50.0f; }

  void send_raw_packet(std::vector<uint8_t> &packet) {
    packets_to_send.push_back(packet);
  }

private:
  const char *TAG = "uponor";

  unsigned long last_char_at = 0;
  bool session_in_progress = false;

  const int maxPacketSize = 50;
  std::vector<uint8_t> packet;
  uint16_t crc = 0xFFFF;

  std::deque<std::vector<uint8_t>> packets_to_send;

  std::vector<UponorBaseDevice *> devices;

  enum parse_result_t { WRONG_DATA, INCOMPLETE, DONE };

  char const hexChars[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                             '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

  void log_as_hex(const bool warn, const uint8_t *data, const uint16_t size) {
    std::vector<uint8_t> buf;

    for (uint16_t i = 0; i < size; i++) {
      auto ch = data[i];
      buf.push_back(hexChars[(ch & 0xF0) >> 4]);
      buf.push_back(hexChars[(ch & 0x0F) >> 0]);
      buf.push_back(' ');
    }
    buf.push_back(0);

    if (warn) {
      ESP_LOGW(TAG, ">>> %s", (char *)buf.data());
    } else {
      ESP_LOGD(TAG, ">>> %s", (char *)buf.data());
    }
  }

  uint16_t crc16(const uint8_t *data, uint8_t len) {
    uint16_t crc = 0xFFFF;
    while (len--) {
      crc ^= *data++;
      for (uint8_t i = 0; i < 8; i++) {
        if ((crc & 0x01) != 0) {
          crc >>= 1;
          crc ^= 0xA001;
        } else {
          crc >>= 1;
        }
      }
    }
    return crc;
  }

  bool verify_crc(const uint8_t *raw, size_t size) {
    uint8_t crc_lo = raw[size - 2];
    uint8_t crc_hi = raw[size - 1];

    uint16_t computed = crc16(raw, size - 2);
    uint16_t remote = uint16_t(crc_lo) | (uint16_t(crc_hi) << 8);
    if (computed != remote) {
      ESP_LOGW(TAG, "CRC Check failed! %02X!=%02X", computed, remote);
      return false;
    }
    return true;
  }

  void calc_byte_crc(uint8_t byte) {
    crc ^= byte;
    for (uint8_t i = 0; i < 8; i++) {
      if ((crc & 0x01) != 0) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }

  parse_result_t parse_packet(uint8_t byte) {
    // 11 05 BA 2B FF A7 9A
    // 11 05 BA 2B 40 03 0F 3E 00 00 3F 0C 02 0A A6
    // 11 05 BA 2B 2D 7F FF 3D 00 00 0C 00 24 37 01 9A 38 03 B6 3B 03 02 3C 00
    // 48 35 80 00 D5 67 11 05 BA 2B 2D 7F FF 3D 00 00 0C 00 24 37 01 9A 38 03
    // B6 3B 03 03 3C 00 48 35 80 00 C5 A7

    size_t at = packet.size();
    packet.push_back(byte);
    const uint8_t *raw = &packet[0];

    if (at == 0) {
      crc = 0xFFFF;
    }

    if (at < 4) {
      calc_byte_crc(byte);
    }

    if (at == 0) {
      return ((byte == 0x11) || (byte == 0x10)) ? INCOMPLETE
                                                : WRONG_DATA; // sensor type?
    }

    if (at == 1) {
      return (byte == 0x05) ? INCOMPLETE : WRONG_DATA; // always 05
    }

    if (at <= 3) { // address hi and lo?
      return INCOMPLETE;
    }

    if (at == 4) { // first key
      return INCOMPLETE;
    }

    // station polling request
    if (raw[4] == 0xFF) {
      if (at < 6) {
        return INCOMPLETE;
      }

      if (at == 6) {
        return DONE;
      }

      return WRONG_DATA;
    }

    uint8_t pair_pos = (at - 4) % 3;
    uint16_t footer_crc = uint16_t(raw[at - 1]) | (uint16_t(raw[at]) << 8);

    if (pair_pos == 1 && footer_crc == crc) {
      return DONE;
    } else if (pair_pos == 2) {
      calc_byte_crc(raw[at - 2]);
      calc_byte_crc(raw[at - 1]);
      calc_byte_crc(raw[at]);
      return INCOMPLETE;
    } else {
      return INCOMPLETE;
    }
  }

  void process_packet_by_device() {
    ESP_LOGI(TAG, "[[[ ---");
    log_as_hex(false, packet.data(), packet.size());

    bool found = false;

    for (auto *device : devices) {
      if (device->process_packet(packet, !found)) {
        found = true;
      }
    }

    if (!found) {
      ESP_LOGW(TAG, "No devices processed the frame!");
    }

    ESP_LOGI(TAG, "]]] ---");
  }
};