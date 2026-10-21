#include "net/protocol.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace xnet {

// ---- ByteWriter ---------------------------------------------------------

void ByteWriter::u8(uint8_t v) { buf_.push_back(v); }
void ByteWriter::i8(int8_t v) { buf_.push_back(static_cast<uint8_t>(v)); }

void ByteWriter::u16(uint16_t v) {
    buf_.push_back(static_cast<uint8_t>(v & 0xFF));
    buf_.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}

void ByteWriter::i16(int16_t v) { u16(static_cast<uint16_t>(v)); }

void ByteWriter::u32(uint32_t v) {
    buf_.push_back(static_cast<uint8_t>(v & 0xFF));
    buf_.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    buf_.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    buf_.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
}

void ByteWriter::bytes(const uint8_t* data, size_t len) {
    buf_.insert(buf_.end(), data, data + len);
}

void ByteWriter::str_u8len(const std::string& s) {
    size_t len = std::min(s.size(), static_cast<size_t>(255));
    u8(static_cast<uint8_t>(len));
    bytes(reinterpret_cast<const uint8_t*>(s.data()), len);
}

// ---- ByteReader ---------------------------------------------------------

uint8_t ByteReader::u8() {
    if (error_ || pos_ + 1 > len_) { error_ = true; return 0; }
    return data_[pos_++];
}

int8_t ByteReader::i8() { return static_cast<int8_t>(u8()); }

uint16_t ByteReader::u16() {
    if (error_ || pos_ + 2 > len_) { error_ = true; return 0; }
    uint16_t v = static_cast<uint16_t>(data_[pos_]) |
                 (static_cast<uint16_t>(data_[pos_ + 1]) << 8);
    pos_ += 2;
    return v;
}

int16_t ByteReader::i16() { return static_cast<int16_t>(u16()); }

uint32_t ByteReader::u32() {
    if (error_ || pos_ + 4 > len_) { error_ = true; return 0; }
    uint32_t v = static_cast<uint32_t>(data_[pos_]) |
                 (static_cast<uint32_t>(data_[pos_ + 1]) << 8) |
                 (static_cast<uint32_t>(data_[pos_ + 2]) << 16) |
                 (static_cast<uint32_t>(data_[pos_ + 3]) << 24);
    pos_ += 4;
    return v;
}

void ByteReader::bytes(uint8_t* out, size_t len) {
    if (error_ || pos_ + len > len_) { error_ = true; return; }
    std::memcpy(out, data_ + pos_, len);
    pos_ += len;
}

std::string ByteReader::str_u8len(size_t max_len) {
    uint8_t len = u8();
    if (error_) return std::string();
    if (static_cast<size_t>(len) > max_len) { error_ = true; return std::string(); }
    if (pos_ + len > len_) { error_ = true; return std::string(); }
    std::string s(reinterpret_cast<const char*>(data_ + pos_), len);
    pos_ += len;
    return s;
}

// ---- Quantisation ---------------------------------------------------------

int16_t encode_yaw(float rx) {
    // wrap to [-pi, pi)
    float v = std::fmod(rx + PI_F, 2.0f * PI_F);
    if (v < 0.0f) v += 2.0f * PI_F;
    v -= PI_F;
    float scaled = v * ANGLE_SCALE;
    scaled = std::round(scaled);
    scaled = std::max(-32768.0f, std::min(32767.0f, scaled));
    return static_cast<int16_t>(scaled);
}

float decode_yaw(int16_t raw) {
    return static_cast<float>(raw) / ANGLE_SCALE;
}

int16_t encode_pitch(float ry) {
    float v = std::max(-PITCH_LIMIT_RAD, std::min(PITCH_LIMIT_RAD, ry));
    float scaled = std::round(v * ANGLE_SCALE);
    scaled = std::max(-32768.0f, std::min(32767.0f, scaled));
    return static_cast<int16_t>(scaled);
}

float decode_pitch(int16_t raw) {
    return static_cast<float>(raw) / ANGLE_SCALE;
}

int16_t encode_pos(float v) {
    float clamped = std::max(-POS_LIMIT, std::min(POS_LIMIT, v));
    float scaled = std::round(clamped * POS_SCALE);
    scaled = std::max(-32768.0f, std::min(32767.0f, scaled));
    return static_cast<int16_t>(scaled);
}

float decode_pos(int16_t raw) {
    return static_cast<float>(raw) / POS_SCALE;
}

// ---- helpers ---------------------------------------------------------

static bool check_header(ByteReader& r, MsgType expect) {
    uint8_t type = r.u8();
    uint8_t version = r.u8();
    if (!r.ok()) return false;
    if (type != static_cast<uint8_t>(expect)) return false;
    if (version != NET_PROTOCOL_VERSION) return false;
    return true;
}

static bool finish(const ByteReader& r) {
    // Reject trailing bytes.
    return r.ok() && r.pos() == r.total_len();
}

uint8_t peek_type(const uint8_t* buf, size_t len) {
    if (buf == nullptr || len < 1) return 0;
    return buf[0];
}

// ---- HELLO ---------------------------------------------------------

std::vector<uint8_t> encode_hello(const Hello& m) {
    ByteWriter w;
    w.u8(static_cast<uint8_t>(MsgType::HELLO));
    w.u8(NET_PROTOCOL_VERSION);
    w.str_u8len(m.name);
    return w.take();
}

bool decode_hello(const uint8_t* buf, size_t len, Hello* out) {
    ByteReader r(buf, len);
    if (!check_header(r, MsgType::HELLO)) return false;
    std::string name = r.str_u8len(MAX_NAME_LEN);
    if (!r.ok()) return false;
    if (!finish(r)) return false;
    out->name = std::move(name);
    return true;
}

// ---- WELCOME ---------------------------------------------------------

std::vector<uint8_t> encode_welcome(const Welcome& m) {
    ByteWriter w;
    w.u8(static_cast<uint8_t>(MsgType::WELCOME));
    w.u8(NET_PROTOCOL_VERSION);
    w.u8(m.slot);
    w.u8(m.map);
    w.u32(m.tick);
    w.u32(m.seed);
    return w.take();
}

bool decode_welcome(const uint8_t* buf, size_t len, Welcome* out) {
    ByteReader r(buf, len);
    if (!check_header(r, MsgType::WELCOME)) return false;
    Welcome w;
    w.slot = r.u8();
    w.map = r.u8();
    w.tick = r.u32();
    w.seed = r.u32();
    if (!r.ok()) return false;
    if (!finish(r)) return false;
    *out = w;
    return true;
}

// ---- CMD ---------------------------------------------------------

static void write_netcmd(ByteWriter& w, const NetCmd& c) {
    w.u32(c.seq);
    w.u16(c.buttons);
    w.i16(encode_yaw(c.rx));
    w.i16(encode_pitch(c.ry));
    w.u8(0); // weapon_hint, reserved
}

static bool read_netcmd(ByteReader& r, NetCmd* out) {
    out->seq = r.u32();
    out->buttons = r.u16();
    int16_t rx = r.i16();
    int16_t ry = r.i16();
    r.u8(); // weapon_hint, reserved
    if (!r.ok()) return false;
    out->rx = decode_yaw(rx);
    out->ry = decode_pitch(ry);
    return true;
}

std::vector<uint8_t> encode_cmd(const std::vector<NetCmd>& cmds) {
    ByteWriter w;
    w.u8(static_cast<uint8_t>(MsgType::CMD));
    w.u8(NET_PROTOCOL_VERSION);
    size_t count = cmds.size();
    if (count < MIN_CMDS) count = MIN_CMDS;
    if (count > MAX_CMDS) count = MAX_CMDS;
    w.u8(static_cast<uint8_t>(count));
    for (size_t i = 0; i < count; ++i) {
        write_netcmd(w, cmds[i]);
    }
    return w.take();
}

bool decode_cmd(const uint8_t* buf, size_t len, std::vector<NetCmd>* out) {
    ByteReader r(buf, len);
    if (!check_header(r, MsgType::CMD)) return false;
    uint8_t count = r.u8();
    if (!r.ok()) return false;
    if (count < MIN_CMDS || count > MAX_CMDS) return false;
    std::vector<NetCmd> cmds;
    cmds.reserve(count);
    for (uint8_t i = 0; i < count; ++i) {
        NetCmd c;
        if (!read_netcmd(r, &c)) return false;
        cmds.push_back(c);
    }
    if (!finish(r)) return false;
    *out = std::move(cmds);
    return true;
}

// ---- SNAP ---------------------------------------------------------

static void write_snaphuman(ByteWriter& w, const SnapHuman& h) {
    w.u8(h.id);
    w.i16(encode_pos(h.x));
    w.i16(encode_pos(h.y));
    w.i16(encode_pos(h.z));
    w.i16(encode_yaw(h.rx));
    w.i16(encode_pitch(h.ry));
    w.i16(h.hp);
    w.u8(h.flags);
    w.i8(h.team);
    w.u8(h.param);
    w.u8(h.weapon);
    w.u8(h.move);
}

static bool read_snaphuman(ByteReader& r, SnapHuman* out) {
    out->id = r.u8();
    int16_t x = r.i16();
    int16_t y = r.i16();
    int16_t z = r.i16();
    int16_t rx = r.i16();
    int16_t ry = r.i16();
    out->hp = r.i16();
    out->flags = r.u8();
    out->team = r.i8();
    out->param = r.u8();
    out->weapon = r.u8();
    out->move = r.u8();
    if (!r.ok()) return false;
    out->x = decode_pos(x);
    out->y = decode_pos(y);
    out->z = decode_pos(z);
    out->rx = decode_yaw(rx);
    out->ry = decode_pitch(ry);
    return true;
}

std::vector<uint8_t> encode_snapshot(const Snapshot& snap) {
    ByteWriter w;
    w.u8(static_cast<uint8_t>(MsgType::SNAP));
    w.u8(NET_PROTOCOL_VERSION);
    w.u32(snap.tick);
    w.u32(snap.ack_seq);
    size_t count = std::min(snap.humans.size(), static_cast<size_t>(MAX_HUMANS));
    w.u8(static_cast<uint8_t>(count));
    for (size_t i = 0; i < count; ++i) {
        write_snaphuman(w, snap.humans[i]);
    }
    return w.take();
}

bool decode_snapshot(const uint8_t* buf, size_t len, Snapshot* out) {
    ByteReader r(buf, len);
    if (!check_header(r, MsgType::SNAP)) return false;
    Snapshot s;
    s.tick = r.u32();
    s.ack_seq = r.u32();
    uint8_t human_count = r.u8();
    if (!r.ok()) return false;
    if (human_count > MAX_HUMANS) return false;
    s.humans.reserve(human_count);
    for (uint8_t i = 0; i < human_count; ++i) {
        SnapHuman h;
        if (!read_snaphuman(r, &h)) return false;
        s.humans.push_back(h);
    }
    if (!finish(r)) return false;
    *out = std::move(s);
    return true;
}

// ---- EVENT ---------------------------------------------------------

static void write_event(ByteWriter& w, const Event& e) {
    w.u8(static_cast<uint8_t>(e.type));
    switch (e.type) {
        case EventType::SHOT:
            w.u8(e.shot.shooter);
            w.u8(e.shot.weapon);
            w.i16(encode_yaw(e.shot.rx));
            w.i16(encode_pitch(e.shot.ry));
            break;
        case EventType::KILL:
            w.u8(e.kill.killer);
            w.u8(e.kill.victim);
            w.u8(e.kill.weapon);
            w.u8(e.kill.headshot);
            break;
        case EventType::RESPAWN:
            w.u8(e.respawn.slot);
            break;
        case EventType::PICKUP:
            w.u8(e.pickup.slot);
            w.u8(e.pickup.weapon);
            break;
        case EventType::MATCH:
            w.u8(e.match.state);
            w.u16(e.match.seconds_left);
            break;
    }
}

static bool read_event(ByteReader& r, Event* out) {
    uint8_t type = r.u8();
    if (!r.ok()) return false;
    switch (type) {
        case static_cast<uint8_t>(EventType::SHOT): {
            out->type = EventType::SHOT;
            out->shot.shooter = r.u8();
            out->shot.weapon = r.u8();
            int16_t rx = r.i16();
            int16_t ry = r.i16();
            if (!r.ok()) return false;
            out->shot.rx = decode_yaw(rx);
            out->shot.ry = decode_pitch(ry);
            return true;
        }
        case static_cast<uint8_t>(EventType::KILL): {
            out->type = EventType::KILL;
            out->kill.killer = r.u8();
            out->kill.victim = r.u8();
            out->kill.weapon = r.u8();
            out->kill.headshot = r.u8();
            return r.ok();
        }
        case static_cast<uint8_t>(EventType::RESPAWN): {
            out->type = EventType::RESPAWN;
            out->respawn.slot = r.u8();
            return r.ok();
        }
        case static_cast<uint8_t>(EventType::PICKUP): {
            out->type = EventType::PICKUP;
            out->pickup.slot = r.u8();
            out->pickup.weapon = r.u8();
            return r.ok();
        }
        case static_cast<uint8_t>(EventType::MATCH): {
            out->type = EventType::MATCH;
            out->match.state = r.u8();
            out->match.seconds_left = r.u16();
            return r.ok();
        }
        default:
            return false;
    }
}

std::vector<uint8_t> encode_events(const std::vector<Event>& events) {
    ByteWriter w;
    w.u8(static_cast<uint8_t>(MsgType::EVENT));
    w.u8(NET_PROTOCOL_VERSION);
    size_t count = std::min(events.size(), static_cast<size_t>(255));
    w.u8(static_cast<uint8_t>(count));
    for (size_t i = 0; i < count; ++i) {
        write_event(w, events[i]);
    }
    return w.take();
}

bool decode_events(const uint8_t* buf, size_t len, std::vector<Event>* out) {
    ByteReader r(buf, len);
    if (!check_header(r, MsgType::EVENT)) return false;
    uint8_t count = r.u8();
    if (!r.ok()) return false;
    std::vector<Event> events;
    events.reserve(count);
    for (uint8_t i = 0; i < count; ++i) {
        Event e;
        if (!read_event(r, &e)) return false;
        events.push_back(e);
    }
    if (!finish(r)) return false;
    *out = std::move(events);
    return true;
}

// ---- BYE ---------------------------------------------------------

std::vector<uint8_t> encode_bye(const Bye& m) {
    ByteWriter w;
    w.u8(static_cast<uint8_t>(MsgType::BYE));
    w.u8(NET_PROTOCOL_VERSION);
    w.u8(m.reason);
    return w.take();
}

bool decode_bye(const uint8_t* buf, size_t len, Bye* out) {
    ByteReader r(buf, len);
    if (!check_header(r, MsgType::BYE)) return false;
    Bye b;
    b.reason = r.u8();
    if (!r.ok()) return false;
    if (!finish(r)) return false;
    *out = b;
    return true;
}

// ---- SCORE ---------------------------------------------------------

std::vector<uint8_t> encode_score(const std::vector<ScoreEntry>& entries) {
    ByteWriter w;
    w.u8(static_cast<uint8_t>(MsgType::SCORE));
    w.u8(NET_PROTOCOL_VERSION);
    size_t count = std::min(entries.size(), static_cast<size_t>(255));
    w.u8(static_cast<uint8_t>(count));
    for (size_t i = 0; i < count; ++i) {
        const ScoreEntry& e = entries[i];
        w.u8(e.slot);
        w.i16(e.frags);
        w.i16(e.deaths);
        w.str_u8len(e.name);
    }
    return w.take();
}

bool decode_score(const uint8_t* buf, size_t len, std::vector<ScoreEntry>* out) {
    ByteReader r(buf, len);
    if (!check_header(r, MsgType::SCORE)) return false;
    uint8_t count = r.u8();
    if (!r.ok()) return false;
    std::vector<ScoreEntry> entries;
    entries.reserve(count);
    for (uint8_t i = 0; i < count; ++i) {
        ScoreEntry e;
        e.slot = r.u8();
        e.frags = r.i16();
        e.deaths = r.i16();
        e.name = r.str_u8len(255);
        if (!r.ok()) return false;
        entries.push_back(std::move(e));
    }
    if (!finish(r)) return false;
    *out = std::move(entries);
    return true;
}

} // namespace xnet
