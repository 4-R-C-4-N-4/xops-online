//[XOPS-ONLINE] C exports between the engine session and the JS transport.
//JS owns the network; the engine only sees opaque messages (docs/NET-PROTOCOL.md).
#include "net/session.h"

#include <cstdio>
#include <cstring>
#include <string>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define XNET_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define XNET_EXPORT
#endif

xnet::Session g_netSession;

static std::string g_json;
static unsigned char g_io[65536];	//JS <-> engine message staging (in and out)

static void AppendJsonString(std::string& out, const std::string& s)
{
	out += '"';
	for(unsigned char c : s){
		if( c == '"' || c == '\\' ){ out += '\\'; out += (char)c; }
		else if( c < 0x20 ){ char b[8]; snprintf(b, sizeof(b), "\\u%04x", c); out += b; }
		else{ out += (char)c; }
	}
	out += '"';
}

extern "C" {

//! role 1 = host, 2 = client; name is UTF-8 (truncated to 15 bytes). Call before the mission starts.
XNET_EXPORT int net_arm(int role, const char* name)
{
	std::string n = name ? std::string(name) : std::string();
	if( role == 1 ){ g_netSession.ArmHost(n); return 0; }
	if( role == 2 ){ g_netSession.ArmClient(n); return 0; }
	g_netSession.Stop();
	return 0;
}

XNET_EXPORT int net_on_message(const unsigned char* buf, int len, int peer)
{
	if( (buf == nullptr)||(len <= 0) ){ return -1; }
	g_netSession.OnMessage(peer, buf, (size_t)len);
	return 0;
}

//! Copies the next outgoing message into buf. Returns its length, 0 when the
//! queue is empty, or -1 if it did not fit (the message is dropped).
XNET_EXPORT int net_poll_outgoing(unsigned char* buf, int cap, int* peer, int* reliable)
{
	xnet::Outgoing o;
	if( g_netSession.PollOutgoing(&o) == false ){ return 0; }
	if( (int)o.bytes.size() > cap ){ return -1; }
	memcpy(buf, o.bytes.data(), o.bytes.size());
	if( peer ){ *peer = o.peer; }
	if( reliable ){ *reliable = o.reliable ? 1 : 0; }
	return (int)o.bytes.size();
}

//! Staging buffer for net_on_message / net_poll_outgoing / net_arm names (64 KiB).
XNET_EXPORT unsigned char* net_buffer(){ return g_io; }
XNET_EXPORT int net_buffer_size(){ return (int)sizeof(g_io); }

//! JS-friendly polling: next outgoing message into net_buffer(); length or 0.
static int g_lastPeer = -1, g_lastReliable = 0;
XNET_EXPORT int net_poll(){ return net_poll_outgoing(g_io, (int)sizeof(g_io), &g_lastPeer, &g_lastReliable); }
XNET_EXPORT int net_last_peer(){ return g_lastPeer; }
XNET_EXPORT int net_last_reliable(){ return g_lastReliable; }
//! JS-friendly delivery: the message is already in net_buffer().
XNET_EXPORT int net_deliver(int len, int peer){ return net_on_message(g_io, len, peer); }
//! JS-friendly arm: the UTF-8 name is already in net_buffer(), NUL-terminated.
XNET_EXPORT int net_arm_buffer(int role){ g_io[sizeof(g_io)-1] = 0; return net_arm(role, (const char*)g_io); }

XNET_EXPORT void net_peer_left(int peer){ g_netSession.OnPeerLeft(peer); }
XNET_EXPORT int net_role(){ return (int)g_netSession.GetRole(); }
XNET_EXPORT int net_active(){ return g_netSession.IsActive() ? 1 : 0; }
XNET_EXPORT int net_local_slot(){ return g_netSession.GetLocalSlot(); }

//! {"tick":N,"local":slot,"scores":[{"slot","name","frags","deaths"}],"feed":[{"killer","victim","head","tick","killerName","victimName"}]}
XNET_EXPORT const char* net_state_json()
{
	g_json.clear();
	char b[96];
	snprintf(b, sizeof(b), "{\"role\":%d,\"tick\":%u,\"local\":%d,\"scores\":[", (int)g_netSession.GetRole(), g_netSession.GetTick(), g_netSession.GetLocalSlot());
	g_json += b;
	bool first = true;
	for(const xnet::PlayerScore& s : g_netSession.GetScores()){
		if( s.used == false ){ continue; }
		if( first == false ){ g_json += ','; }
		first = false;
		snprintf(b, sizeof(b), "{\"slot\":%d,\"frags\":%d,\"deaths\":%d,\"name\":", s.slot, s.frags, s.deaths);
		g_json += b;
		AppendJsonString(g_json, s.name);
		g_json += '}';
	}
	g_json += "],\"feed\":[";
	first = true;
	for(const xnet::KillFeedEntry& k : g_netSession.GetKillFeed()){
		if( first == false ){ g_json += ','; }
		first = false;
		std::string kn, vn;
		g_netSession.GetSlotName(k.killer, &kn);
		g_netSession.GetSlotName(k.victim, &vn);
		snprintf(b, sizeof(b), "{\"killer\":%d,\"victim\":%d,\"head\":%d,\"tick\":%u,\"killerName\":", k.killer, k.victim, k.headshot ? 1 : 0, k.tick);
		g_json += b;
		AppendJsonString(g_json, kn);
		g_json += ",\"victimName\":";
		AppendJsonString(g_json, vn);
		g_json += '}';
	}
	g_json += "]}";
	return g_json.c_str();
}

}
