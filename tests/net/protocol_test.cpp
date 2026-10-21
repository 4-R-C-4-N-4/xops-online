// Native test harness for the xnet protocol module.
// Plain main(), asserts, no test framework. Prints NET PROTOCOL OK at end.
#include "net/protocol.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <vector>

using namespace xnet;

static void test_netcmd_roundtrip() {
    std::vector<NetCmd> cmds;
    NetCmd c1; c1.seq = 1234; c1.buttons = BTN_FORWARD | BTN_FIRE | BTN_SHOTMODE;
    c1.rx = 1.5f; c1.ry = 0.3f;
    cmds.push_back(c1);
    NetCmd c2; c2.seq = 1235; c2.buttons = 0; c2.rx = -3.14f; c2.ry = -1.2f;
    cmds.push_back(c2);

    auto buf = encode_cmd(cmds);
    assert(buf.size() == 3 + 2 * NETCMD_WIRE_SIZE); // header(2) + count(1) + 2 records
    std::vector<NetCmd> out;
    bool ok = decode_cmd(buf.data(), buf.size(), &out);
    assert(ok);
    assert(out.size() == 2);
    assert(out[0].seq == 1234);
    assert(out[0].buttons == cmds[0].buttons);
    assert(std::fabs(out[0].rx - c1.rx) <= 1e-3f);
    assert(std::fabs(out[0].ry - c1.ry) <= 1e-3f);
    assert(out[1].seq == 1235);
}

static void test_netcmd_wire_size() {
    // header(2) + count(1) + N * NETCMD_WIRE_SIZE
    NetCmd c{};
    std::vector<NetCmd> cmds{c};
    auto buf = encode_cmd(cmds);
    assert(buf.size() == 2 + 1 + NETCMD_WIRE_SIZE);
    assert(NETCMD_WIRE_SIZE == 11);
}

static void test_cmd_count_bounds() {
    // 0 cmds given is clamped up to MIN_CMDS by the encoder, so build raw bytes
    // to exercise the decoder's rejection of out-of-range counts directly.
    {
        std::vector<uint8_t> raw = {static_cast<uint8_t>(MsgType::CMD), NET_PROTOCOL_VERSION, 0};
        std::vector<NetCmd> out;
        assert(!decode_cmd(raw.data(), raw.size(), &out));
    }
    {
        std::vector<uint8_t> raw = {static_cast<uint8_t>(MsgType::CMD), NET_PROTOCOL_VERSION, 5};
        // not enough bytes for 5 records anyway, but count itself must be rejected
        std::vector<NetCmd> out;
        assert(!decode_cmd(raw.data(), raw.size(), &out));
    }
    // valid boundary counts 1 and 4 round trip.
    {
        std::vector<NetCmd> cmds(1);
        auto buf = encode_cmd(cmds);
        std::vector<NetCmd> out;
        assert(decode_cmd(buf.data(), buf.size(), &out));
        assert(out.size() == 1);
    }
    {
        std::vector<NetCmd> cmds(4);
        auto buf = encode_cmd(cmds);
        std::vector<NetCmd> out;
        assert(decode_cmd(buf.data(), buf.size(), &out));
        assert(out.size() == 4);
    }
}

static void test_yaw_wrap() {
    // Wraps into [-pi, pi)
    int16_t raw = encode_yaw(3.0f * PI_F); // should wrap to ~ -pi region -> near 0? 3pi mod 2pi = pi -> wraps to -pi
    float decoded = decode_yaw(raw);
    assert(decoded >= -PI_F - 1e-3f && decoded < PI_F + 1e-3f);

    // round trip near zero
    int16_t raw0 = encode_yaw(0.0f);
    assert(std::fabs(decode_yaw(raw0) - 0.0f) <= 1e-4);
}

static void test_pitch_clamp() {
    int16_t raw = encode_pitch(100.0f * (PI_F / 180.0f)); // 100 deg, over +70 limit
    float decoded = decode_pitch(raw);
    assert(decoded <= PITCH_LIMIT_RAD + 1e-3f);
    assert(decoded >= PITCH_LIMIT_RAD - 1e-3f);

    int16_t raw2 = encode_pitch(-100.0f * (PI_F / 180.0f));
    float decoded2 = decode_pitch(raw2);
    assert(decoded2 >= -PITCH_LIMIT_RAD - 1e-3f);
    assert(decoded2 <= -PITCH_LIMIT_RAD + 1e-3f);
}

static void test_angle_precision() {
    // Max quantisation error should be about half an ULP: 2pi/65536 ~ 9.587e-5 rad.
    float max_err = (2.0f * PI_F / 65536.0f) / 2.0f + 1e-6f;
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-PI_F, PI_F);
    for (int i = 0; i < 1000; ++i) {
        float a = dist(rng);
        int16_t raw = encode_yaw(a);
        float decoded = decode_yaw(raw);
        float diff = decoded - a;
        // account for wrap at boundary
        if (diff > PI_F) diff -= 2.0f * PI_F;
        if (diff < -PI_F) diff += 2.0f * PI_F;
        assert(std::fabs(diff) <= max_err);
    }
}

static void test_pos_precision_and_clamp() {
    std::mt19937 rng(7);
    std::uniform_real_distribution<float> dist(-4096.0f, 4096.0f);
    for (int i = 0; i < 1000; ++i) {
        float v = dist(rng);
        int16_t raw = encode_pos(v);
        float decoded = decode_pos(raw);
        assert(std::fabs(decoded - v) <= 1.0f / 16.0f + 1e-5f);
    }
    // clamp beyond range
    int16_t rawhi = encode_pos(999999.0f);
    assert(decode_pos(rawhi) <= 4096.0f + 1.0f);
    int16_t rawlo = encode_pos(-999999.0f);
    assert(decode_pos(rawlo) >= -4096.0f - 1.0f);
}

static void test_hello_roundtrip() {
    Hello h; h.name = "Player1";
    auto buf = encode_hello(h);
    Hello out;
    assert(decode_hello(buf.data(), buf.size(), &out));
    assert(out.name == "Player1");

    // Boundary: exactly 15 bytes ok.
    Hello h15; h15.name = std::string(15, 'x');
    auto buf15 = encode_hello(h15);
    Hello out15;
    assert(decode_hello(buf15.data(), buf15.size(), &out15));
    assert(out15.name.size() == 15);

    // 16 bytes should be rejected by the decoder.
    std::vector<uint8_t> raw = {static_cast<uint8_t>(MsgType::HELLO), NET_PROTOCOL_VERSION, 16};
    for (int i = 0; i < 16; ++i) raw.push_back('a');
    Hello outbad;
    assert(!decode_hello(raw.data(), raw.size(), &outbad));
}

static void test_welcome_roundtrip() {
    Welcome w; w.slot = 3; w.map = 7; w.tick = 99999; w.seed = 123456789;
    auto buf = encode_welcome(w);
    Welcome out;
    assert(decode_welcome(buf.data(), buf.size(), &out));
    assert(out.slot == 3 && out.map == 7 && out.tick == 99999 && out.seed == 123456789);
}

static void test_bye_roundtrip() {
    Bye b; b.reason = 2;
    auto buf = encode_bye(b);
    Bye out;
    assert(decode_bye(buf.data(), buf.size(), &out));
    assert(out.reason == 2);
}

static SnapHuman make_human(uint8_t id) {
    SnapHuman h{};
    h.id = id;
    h.x = 100.5f; h.y = -200.25f; h.z = 5.0f;
    h.rx = 1.0f; h.ry = -0.5f;
    h.hp = 80;
    h.flags = FLAG_SCOPE | FLAG_INVINCIBLE;
    h.team = -1;
    h.param = 2;
    h.weapon = 5;
    h.move = 9;
    return h;
}

static void test_snapshot_roundtrip() {
    Snapshot s; s.tick = 42; s.ack_seq = 7;
    s.humans.push_back(make_human(0));
    s.humans.push_back(make_human(1));
    auto buf = encode_snapshot(s);
    Snapshot out;
    assert(decode_snapshot(buf.data(), buf.size(), &out));
    assert(out.tick == 42 && out.ack_seq == 7);
    assert(out.humans.size() == 2);
    assert(out.humans[0].id == 0);
    assert(out.humans[0].hp == 80);
    assert(out.humans[0].team == -1);
    assert(std::fabs(out.humans[0].x - 100.5f) <= 1.0f / 16.0f + 1e-5f);
}

static void test_snapshot_32_humans_size() {
    Snapshot s; s.tick = 1; s.ack_seq = 1;
    for (int i = 0; i < 32; ++i) s.humans.push_back(make_human(static_cast<uint8_t>(i)));
    auto buf = encode_snapshot(s);
    size_t expected = 2 /*header*/ + 4 + 4 + 1 /*snapshot header*/ + 32 * SNAPHUMAN_WIRE_SIZE;
    assert(buf.size() == expected);
    Snapshot out;
    assert(decode_snapshot(buf.data(), buf.size(), &out));
    assert(out.humans.size() == 32);
}

static void test_snapshot_human_count_over_max() {
    std::vector<uint8_t> raw = {static_cast<uint8_t>(MsgType::SNAP), NET_PROTOCOL_VERSION,
                                 0, 0, 0, 0, // tick
                                 0, 0, 0, 0, // ack_seq
                                 97}; // human_count > 96
    Snapshot out;
    assert(!decode_snapshot(raw.data(), raw.size(), &out));
}

static void test_events_roundtrip() {
    std::vector<Event> events;
    Event e1; e1.type = EventType::SHOT; e1.shot.shooter = 1; e1.shot.weapon = 2; e1.shot.rx = 0.5f; e1.shot.ry = -0.2f;
    events.push_back(e1);
    Event e2; e2.type = EventType::KILL; e2.kill.killer = 1; e2.kill.victim = 2; e2.kill.weapon = 3; e2.kill.headshot = 1;
    events.push_back(e2);
    Event e3; e3.type = EventType::RESPAWN; e3.respawn.slot = 4;
    events.push_back(e3);
    Event e4; e4.type = EventType::PICKUP; e4.pickup.slot = 5; e4.pickup.weapon = 6;
    events.push_back(e4);
    Event e5; e5.type = EventType::MATCH; e5.match.state = 1; e5.match.seconds_left = 120;
    events.push_back(e5);

    auto buf = encode_events(events);
    std::vector<Event> out;
    assert(decode_events(buf.data(), buf.size(), &out));
    assert(out.size() == 5);
    assert(out[0].type == EventType::SHOT && out[0].shot.shooter == 1);
    assert(out[1].type == EventType::KILL && out[1].kill.victim == 2 && out[1].kill.headshot == 1);
    assert(out[2].type == EventType::RESPAWN && out[2].respawn.slot == 4);
    assert(out[3].type == EventType::PICKUP && out[3].pickup.weapon == 6);
    assert(out[4].type == EventType::MATCH && out[4].match.seconds_left == 120);
}

static void test_score_roundtrip() {
    std::vector<ScoreEntry> entries;
    ScoreEntry a; a.slot = 0; a.frags = 10; a.deaths = 3; a.name = "Alice";
    entries.push_back(a);
    ScoreEntry b; b.slot = 1; b.frags = -1; b.deaths = 99; b.name = "Bob";
    entries.push_back(b);

    auto buf = encode_score(entries);
    std::vector<ScoreEntry> out;
    assert(decode_score(buf.data(), buf.size(), &out));
    assert(out.size() == 2);
    assert(out[0].name == "Alice" && out[0].frags == 10 && out[0].deaths == 3);
    assert(out[1].name == "Bob" && out[1].frags == -1);
}

static void test_peek_type() {
    Hello h; h.name = "x";
    auto buf = encode_hello(h);
    assert(peek_type(buf.data(), buf.size()) == static_cast<uint8_t>(MsgType::HELLO));
    assert(peek_type(nullptr, 0) == 0);
    assert(peek_type(buf.data(), 0) == 0);
}

// Truncation: for each message, truncating at every byte length must return
// false and never crash.
template <typename Decoder, typename Out>
static void test_truncation(const std::vector<uint8_t>& full, Decoder decode) {
    for (size_t len = 0; len < full.size(); ++len) {
        Out out{};
        bool ok = decode(full.data(), len, &out);
        assert(!ok);
    }
    // Full buffer must decode ok (sanity).
    Out out{};
    assert(decode(full.data(), full.size(), &out));
}

static void test_all_truncations() {
    {
        Hello h; h.name = "abc";
        auto buf = encode_hello(h);
        test_truncation<bool(*)(const uint8_t*, size_t, Hello*), Hello>(buf, decode_hello);
    }
    {
        Welcome w; w.slot = 1; w.map = 2; w.tick = 3; w.seed = 4;
        auto buf = encode_welcome(w);
        test_truncation<bool(*)(const uint8_t*, size_t, Welcome*), Welcome>(buf, decode_welcome);
    }
    {
        std::vector<NetCmd> cmds(2);
        auto buf = encode_cmd(cmds);
        test_truncation<bool(*)(const uint8_t*, size_t, std::vector<NetCmd>*), std::vector<NetCmd>>(buf, decode_cmd);
    }
    {
        Snapshot s; s.tick = 1; s.ack_seq = 2; s.humans.push_back(make_human(0)); s.humans.push_back(make_human(1));
        auto buf = encode_snapshot(s);
        test_truncation<bool(*)(const uint8_t*, size_t, Snapshot*), Snapshot>(buf, decode_snapshot);
    }
    {
        std::vector<Event> events;
        Event e; e.type = EventType::MATCH; e.match.state = 1; e.match.seconds_left = 10;
        events.push_back(e);
        auto buf = encode_events(events);
        test_truncation<bool(*)(const uint8_t*, size_t, std::vector<Event>*), std::vector<Event>>(buf, decode_events);
    }
    {
        Bye b; b.reason = 1;
        auto buf = encode_bye(b);
        test_truncation<bool(*)(const uint8_t*, size_t, Bye*), Bye>(buf, decode_bye);
    }
    {
        std::vector<ScoreEntry> entries;
        ScoreEntry e; e.slot = 0; e.frags = 1; e.deaths = 2; e.name = "A";
        entries.push_back(e);
        auto buf = encode_score(entries);
        test_truncation<bool(*)(const uint8_t*, size_t, std::vector<ScoreEntry>*), std::vector<ScoreEntry>>(buf, decode_score);
    }
}

static void test_wrong_version_and_trailing_garbage() {
    Hello h; h.name = "abc";
    auto buf = encode_hello(h);
    buf[1] = NET_PROTOCOL_VERSION + 1;
    Hello out;
    assert(!decode_hello(buf.data(), buf.size(), &out));

    buf[1] = NET_PROTOCOL_VERSION; // restore
    buf.push_back(0xFF); // trailing garbage
    assert(!decode_hello(buf.data(), buf.size(), &out));

    // wrong type
    buf.pop_back();
    buf[0] = static_cast<uint8_t>(MsgType::WELCOME);
    assert(!decode_hello(buf.data(), buf.size(), &out));
}

static void fuzz_decoders() {
    std::mt19937 rng(1234);
    std::uniform_int_distribution<int> len_dist(0, 64);
    std::uniform_int_distribution<int> byte_dist(0, 255);
    for (int i = 0; i < 20000; ++i) {
        int len = len_dist(rng);
        std::vector<uint8_t> buf(len);
        for (auto& b : buf) b = static_cast<uint8_t>(byte_dist(rng));

        Hello h; decode_hello(buf.data(), buf.size(), &h);
        Welcome w; decode_welcome(buf.data(), buf.size(), &w);
        std::vector<NetCmd> cmds; decode_cmd(buf.data(), buf.size(), &cmds);
        Snapshot s; decode_snapshot(buf.data(), buf.size(), &s);
        std::vector<Event> events; decode_events(buf.data(), buf.size(), &events);
        Bye b; decode_bye(buf.data(), buf.size(), &b);
        std::vector<ScoreEntry> entries; decode_score(buf.data(), buf.size(), &entries);
        peek_type(buf.data(), buf.size());
    }
}

int main() {
    test_netcmd_roundtrip();
    test_netcmd_wire_size();
    test_cmd_count_bounds();
    test_yaw_wrap();
    test_pitch_clamp();
    test_angle_precision();
    test_pos_precision_and_clamp();
    test_hello_roundtrip();
    test_welcome_roundtrip();
    test_bye_roundtrip();
    test_snapshot_roundtrip();
    test_snapshot_32_humans_size();
    test_snapshot_human_count_over_max();
    test_events_roundtrip();
    test_score_roundtrip();
    test_peek_type();
    test_all_truncations();
    test_wrong_version_and_trailing_garbage();
    fuzz_decoders();

    std::printf("NET PROTOCOL OK\n");
    return 0;
}
