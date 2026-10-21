//[XOPS-ONLINE] Host/client session: runs the network side of a deathmatch on
//top of ObjectManager (engine seam) and the wire protocol (net/protocol.h).
//It never touches the transport: messages go in through OnMessage and come
//out through PollOutgoing, so the same code runs in the browser (C exports
//in net/exports.cpp) and in native tests (two sessions wired in memory).
#ifndef XNET_SESSION_H
#define XNET_SESSION_H

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

#include "net/protocol.h"
#include "humancommand.h"

class ObjectManager;
class PointDataInterface;

namespace xnet {

constexpr int MAX_PEERS = 15;				//clients per host (host is the 16th player)
constexpr int RESPAWN_TICKS = 90;			//3 s at 30 Hz
constexpr int SPAWN_PROTECT_TICKS = 45;		//1.5 s invincible after (re)spawn
constexpr int CMD_QUEUE_MAX = 8;
constexpr int SCORE_EVERY_TICKS = 30;
constexpr int DM_SPAWN_P1 = 60;				//DM spawn point type in .pd1 (p4 = index)

struct Outgoing {
	int peer;			//-1 = every connected peer
	bool reliable;
	std::vector<uint8_t> bytes;
};

struct KillFeedEntry {
	int killer, victim;
	bool headshot;
	uint32_t tick;
};

struct PlayerScore {
	bool used = false;
	int slot = -1;
	std::string name;
	int frags = 0;
	int deaths = 0;
};

class Session {
public:
	enum Role { ROLE_OFF = 0, ROLE_HOST = 1, ROLE_CLIENT = 2 };

	//Arm before the mission loads (lobby), attach once it has loaded.
	void ArmHost(const std::string& name);
	void ArmClient(const std::string& name);
	void Attach(ObjectManager* om, PointDataInterface* points);
	void Stop();

	Role GetRole() const { return role; }
	bool IsActive() const { return role != ROLE_OFF && om != nullptr; }
	int GetLocalSlot() const { return localSlot; }
	uint32_t GetTick() const { return tick; }

	//Transport side
	void OnMessage(int peer, const uint8_t* buf, size_t len);
	void OnPeerLeft(int peer);
	bool PollOutgoing(Outgoing* out);

	//Host: call HostPreTick before ObjectManager::Process, HostPostTick after.
	void HostPreTick();
	void HostPostTick();
	//Client: send this tick's command, then apply the newest snapshot/events
	//before ObjectManager::Process.
	void ClientSendCommand(const HumanCommand& cmd);
	void ClientApply();

	//Scores and kill feed (host authoritative, mirrored on clients)
	const std::vector<PlayerScore>& GetScores() const { return scores; }
	const std::deque<KillFeedEntry>& GetKillFeed() const { return killfeed; }
	bool GetSlotName(int slot, std::string* name) const;
	int GetPeerCount() const;
	uint32_t GetLastSnapshotTick() const { return lastSnapTick; }

	//Host: pick a spawn far from living players (DM spawns, else the map's player spawn)
	bool PickSpawn(float* x, float* y, float* z, float* r);
	int DefaultLoadout[2] = { 1, 3 };			//MP5 + M92F

private:
	struct Peer {
		bool used = false;
		int peerId = -1;
		int slot = -1;
		uint32_t lastSeq = 0;
		uint16_t prevHeld = 0;
		NetCmd last;
		std::deque<NetCmd> queue;
		int scoreIndex = -1;
	};

	Role role = ROLE_OFF;
	std::string localName;
	ObjectManager* om = nullptr;
	PointDataInterface* points = nullptr;
	uint32_t tick = 0;
	int localSlot = -1;
	std::deque<Outgoing> outq;
	std::vector<PlayerScore> scores;
	std::deque<KillFeedEntry> killfeed;
	bool scoresDirty = false;

	//host
	Peer peers[MAX_PEERS];
	bool wasAlive[96] = {};
	int respawnAt[96] = {};
	int protectUntil[96] = {};
	std::vector<Event> tickEvents;

	//client
	bool helloSent = false;
	uint32_t cmdSeq = 0;
	std::deque<NetCmd> sentCmds;
	bool haveSnap = false;
	Snapshot latest;
	uint32_t lastSnapTick = 0;
	std::vector<Event> pendingEvents;

	void Send(int peer, bool reliable, std::vector<uint8_t> bytes);
	int FindPeer(int peerId) const;
	void HostOnHello(int peer, const uint8_t* buf, size_t len);
	void HostOnCmd(int peer, const uint8_t* buf, size_t len);
	void ClientOnMessage(const uint8_t* buf, size_t len);
	int AddScore(int slot, const std::string& name);
	PlayerScore* ScoreForSlot(int slot);
	void PushKill(int killer, int victim, bool headshot);
	void SendScores();
	void ApplyClientEvent(const Event& e);
};

}

#endif
