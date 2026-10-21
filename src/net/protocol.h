// XOPS ONLINE network protocol (v1). Pure C++17, no engine dependencies.
// See docs/NET-PROTOCOL.md for the wire contract.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace xnet {

constexpr uint8_t NET_PROTOCOL_VERSION = 1;

constexpr int MAX_HUMANS = 96;
constexpr int MAX_NAME_LEN = 15;
constexpr int MIN_CMDS = 1;
constexpr int MAX_CMDS = 4;

enum class MsgType : uint8_t {
    HELLO = 1,
    WELCOME = 2,
    CMD = 3,
    SNAP = 4,
    EVENT = 5,
    BYE = 6,
    SCORE = 7,
};

// NetCmd::buttons bitmask. All bits carry HELD key state for the tick; the
// host derives press edges by comparing against the previously applied
// command for that slot (there are no "edge" bits on the wire).
enum ButtonBit : uint16_t {
    BTN_FORWARD = 1u << 0,
    BTN_BACK = 1u << 1,
    BTN_LEFT = 1u << 2,
    BTN_RIGHT = 1u << 3,
    BTN_WALK = 1u << 4,
    BTN_JUMP = 1u << 5,
    BTN_FIRE = 1u << 6,
    BTN_RELOAD = 1u << 7,
    BTN_SWITCH_WEAPON = 1u << 8,
    BTN_WEAPON_SLOT1 = 1u << 9,
    BTN_WEAPON_SLOT2 = 1u << 10,
    BTN_DROP_WEAPON = 1u << 11,
    BTN_SCOPE_TOGGLE = 1u << 12,
    BTN_SHOTMODE = 1u << 13,
};

enum class EventType : uint8_t {
    SHOT = 1,
    KILL = 2,
    RESPAWN = 3,
    PICKUP = 4,
    MATCH = 5,
};

// SnapHuman::flags bitmask.
enum HumanFlag : uint8_t {
    FLAG_DEAD = 1u << 0,
    FLAG_DEAD_MOTION_FINISHED = 1u << 1,
    FLAG_SCOPE = 1u << 2,
    FLAG_INVINCIBLE = 1u << 3,
};

// ---- Little-endian, bounds-checked byte cursors -----------------------

class ByteWriter {
public:
    void u8(uint8_t v);
    void i8(int8_t v);
    void u16(uint16_t v);
    void i16(int16_t v);
    void u32(uint32_t v);
    void bytes(const uint8_t* data, size_t len);
    void str_u8len(const std::string& s); // 1-byte length prefix + bytes

    const std::vector<uint8_t>& data() const { return buf_; }
    std::vector<uint8_t> take() { return std::move(buf_); }

private:
    std::vector<uint8_t> buf_;
};

class ByteReader {
public:
    ByteReader(const uint8_t* data, size_t len) : data_(data), len_(len) {}

    uint8_t u8();
    int8_t i8();
    uint16_t u16();
    int16_t i16();
    uint32_t u32();
    void bytes(uint8_t* out, size_t len);
    std::string str_u8len(size_t max_len);

    bool ok() const { return !error_; }
    size_t pos() const { return pos_; }
    size_t remaining() const { return error_ ? 0 : (len_ - pos_); }
    size_t total_len() const { return len_; }

private:
    const uint8_t* data_;
    size_t len_;
    size_t pos_ = 0;
    bool error_ = false;
};

// ---- Angle / position quantisation -------------------------------------

// radians * (2^16 / 2pi)
constexpr float ANGLE_SCALE = 10430.378f;
constexpr float PI_F = 3.14159265358979323846f;
constexpr float PITCH_LIMIT_RAD = 70.0f * (PI_F / 180.0f);

// Position units are world-unit * 8, clamped to the representable i16 range.
constexpr float POS_SCALE = 8.0f;
constexpr float POS_LIMIT = 4096.0f;

int16_t encode_yaw(float rx_radians);   // wraps to [-pi, pi)
float decode_yaw(int16_t raw);
int16_t encode_pitch(float ry_radians); // clamps to +-70 deg
float decode_pitch(int16_t raw);

int16_t encode_pos(float v); // clamped to +-4096
float decode_pos(int16_t raw);

// ---- Messages -----------------------------------------------------------

struct NetCmd {
    uint32_t seq = 0;
    uint16_t buttons = 0;
    float rx = 0.0f; // yaw, radians
    float ry = 0.0f; // pitch, radians
};

struct Hello {
    std::string name;
};

struct Welcome {
    uint8_t slot = 0;
    uint8_t map = 0;
    uint32_t tick = 0;
    uint32_t seed = 0;
};

struct SnapHuman {
    uint8_t id = 0;
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float rx = 0.0f, ry = 0.0f;
    int16_t hp = 0;
    uint8_t flags = 0;
    int8_t team = 0;
    uint8_t param = 0;
    uint8_t weapon = 0;
    uint8_t move = 0;
};

struct Snapshot {
    uint32_t tick = 0;
    uint32_t ack_seq = 0;
    std::vector<SnapHuman> humans;
};

struct EventShot {
    uint8_t shooter = 0;
    uint8_t weapon = 0;
    float rx = 0.0f, ry = 0.0f;
};

struct EventKill {
    uint8_t killer = 0;
    uint8_t victim = 0;
    uint8_t weapon = 0;
    uint8_t headshot = 0;
};

struct EventRespawn {
    uint8_t slot = 0;
};

struct EventPickup {
    uint8_t slot = 0;
    uint8_t weapon = 0;
};

struct EventMatch {
    uint8_t state = 0; // 0 warmup, 1 playing, 2 ended
    uint16_t seconds_left = 0;
};

struct Event {
    EventType type = EventType::SHOT;
    EventShot shot{};
    EventKill kill{};
    EventRespawn respawn{};
    EventPickup pickup{};
    EventMatch match{};
};

struct ScoreEntry {
    uint8_t slot = 0;
    int16_t frags = 0;
    int16_t deaths = 0;
    std::string name;
};

struct Bye {
    uint8_t reason = 0;
};

// peek_type: returns the message type byte, or 0 if buf is empty / invalid.
uint8_t peek_type(const uint8_t* buf, size_t len);

std::vector<uint8_t> encode_hello(const Hello& m);
bool decode_hello(const uint8_t* buf, size_t len, Hello* out);

std::vector<uint8_t> encode_welcome(const Welcome& m);
bool decode_welcome(const uint8_t* buf, size_t len, Welcome* out);

std::vector<uint8_t> encode_cmd(const std::vector<NetCmd>& cmds);
bool decode_cmd(const uint8_t* buf, size_t len, std::vector<NetCmd>* out);

std::vector<uint8_t> encode_snapshot(const Snapshot& snap);
bool decode_snapshot(const uint8_t* buf, size_t len, Snapshot* out);

std::vector<uint8_t> encode_events(const std::vector<Event>& events);
bool decode_events(const uint8_t* buf, size_t len, std::vector<Event>* out);

std::vector<uint8_t> encode_bye(const Bye& m);
bool decode_bye(const uint8_t* buf, size_t len, Bye* out);

std::vector<uint8_t> encode_score(const std::vector<ScoreEntry>& entries);
bool decode_score(const uint8_t* buf, size_t len, std::vector<ScoreEntry>* out);

// Size in bytes of one NetCmd record on the wire.
constexpr size_t NETCMD_WIRE_SIZE = 11; // seq(4) + buttons(2) + rx(2) + ry(2) + weapon_hint(1)
// Size in bytes of one SnapHuman record on the wire.
constexpr size_t SNAPHUMAN_WIRE_SIZE = 1 + 2 * 5 + 2 + 1 + 1 + 1 + 1 + 1; // 18

} // namespace xnet
