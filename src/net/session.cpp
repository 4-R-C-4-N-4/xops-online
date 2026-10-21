//[XOPS-ONLINE] Host/client session. See session.h and docs/NET-PROTOCOL.md.
#include "net/session.h"

#include <cmath>
#include <cstring>

#include "objectmanager.h"

namespace xnet {

static uint16_t ButtonsToHcmd(uint16_t b)
{
	//protocol bits and HCMD bits are identical by design (both 0..13)
	return b;
}

void Session::ArmHost(const std::string& name)
{
	Stop();
	role = ROLE_HOST;
	localName = name.empty() ? std::string("HOST") : name.substr(0, MAX_NAME_LEN);
}

void Session::ArmClient(const std::string& name)
{
	Stop();
	role = ROLE_CLIENT;
	localName = name.empty() ? std::string("PLAYER") : name.substr(0, MAX_NAME_LEN);
}

void Session::Attach(ObjectManager* in_om, PointDataInterface* in_points)
{
	if( role == ROLE_OFF ){ return; }
	om = in_om;
	points = in_points;
	tick = 0;
	if( role == ROLE_HOST ){
		om->SetNetPuppet(false);
		localSlot = om->GetPlayerID();
		scores.clear();
		AddScore(localSlot, localName);
		for(int i=0; i<96; i++){
			human* h = om->GetHumanObject(i);
			wasAlive[i] = h->GetEnableFlag() && h->GetHP() > 0;
			respawnAt[i] = 0;
			protectUntil[i] = 0;
		}
		//the host player and every DM player start somewhere fair
		float x, y, z, r;
		if( PickSpawn(&x, &y, &z, &r) ){
			om->RespawnHuman(localSlot, x, y, z, r, DefaultLoadout);
			om->GetHumanObject(localSlot)->SetRxRy(r, 0.0f);
		}
	}
	else{
		om->SetNetPuppet(true);
		localSlot = -1;
		if( helloSent == false ){
			Hello h; h.name = localName;
			Send(-1, true, encode_hello(h));
			helloSent = true;
		}
	}
}

void Session::Stop()
{
	if( (role != ROLE_OFF)&&(om != nullptr) ){ Send(-1, true, encode_bye(Bye())); }
	if( om != nullptr ){
		om->SetNetPuppet(false);
		for(int i=0; i<96; i++){ om->SetNetControlled(i, false); }
	}
	role = ROLE_OFF;
	om = nullptr;
	points = nullptr;
	localSlot = -1;
	for(int i=0; i<MAX_PEERS; i++){ peers[i] = Peer(); }
	scores.clear();
	killfeed.clear();
	tickEvents.clear();
	helloSent = false;
	cmdSeq = 0;
	sentCmds.clear();
	haveSnap = false;
	pendingEvents.clear();
	//outq is left for the transport to flush the BYE
}

void Session::Send(int peer, bool reliable, std::vector<uint8_t> bytes)
{
	Outgoing o;
	o.peer = peer;
	o.reliable = reliable;
	o.bytes = std::move(bytes);
	outq.push_back(std::move(o));
	if( outq.size() > 512 ){ outq.pop_front(); }	//never grow without bound if JS stops polling
}

bool Session::PollOutgoing(Outgoing* out)
{
	if( outq.empty() ){ return false; }
	*out = std::move(outq.front());
	outq.pop_front();
	return true;
}

int Session::GetPeerCount() const
{
	int n = 0;
	for(int i=0; i<MAX_PEERS; i++){ if( peers[i].used ){ n++; } }
	return n;
}

bool Session::GetSlotName(int slot, std::string* name) const
{
	for(const PlayerScore& s : scores){
		if( s.used && s.slot == slot ){ *name = s.name; return true; }
	}
	return false;
}

int Session::AddScore(int slot, const std::string& name)
{
	for(size_t i=0; i<scores.size(); i++){
		if( scores[i].used == false ){
			scores[i] = PlayerScore(); scores[i].used = true; scores[i].slot = slot; scores[i].name = name;
			scoresDirty = true;
			return (int)i;
		}
	}
	PlayerScore s; s.used = true; s.slot = slot; s.name = name;
	scores.push_back(s);
	scoresDirty = true;
	return (int)scores.size() - 1;
}

PlayerScore* Session::ScoreForSlot(int slot)
{
	for(PlayerScore& s : scores){ if( s.used && s.slot == slot ){ return &s; } }
	return nullptr;
}

void Session::PushKill(int killer, int victim, bool headshot)
{
	KillFeedEntry k; k.killer = killer; k.victim = victim; k.headshot = headshot; k.tick = tick;
	killfeed.push_back(k);
	while( killfeed.size() > 6 ){ killfeed.pop_front(); }
}

bool Session::PickSpawn(float* x, float* y, float* z, float* r)
{
	if( (om == nullptr)||(points == nullptr) ){ return false; }
	float best = -1.0f;
	bool found = false;
	int total = points->GetTotaldatas();
	for(int i=0; i<total; i++){
		pointdata p;
		if( points->Getdata(&p, i) != 0 ){ continue; }
		if( p.p1 != DM_SPAWN_P1 ){ continue; }
		//distance to the nearest living player (bigger is better), a little randomness to break ties
		float nearest = 1.0e9f;
		for(int h=0; h<96; h++){
			human* hu = om->GetHumanObject(h);
			if( (hu->GetEnableFlag() == false)||(hu->GetHP() <= 0) ){ continue; }
			float hx, hy, hz; hu->GetPosData(&hx, &hy, &hz, NULL);
			float d = sqrtf((hx-p.x)*(hx-p.x) + (hz-p.z)*(hz-p.z));
			if( d < nearest ){ nearest = d; }
		}
		float score = nearest + (float)((i * 7 + tick) % 13);
		if( score > best ){ best = score; *x = p.x; *y = p.y; *z = p.z; *r = p.r; found = true; }
	}
	if( found == false ){
		//no DM spawns: fall back to the map's player spawn
		pointdata p;
		if( points->SearchPointdata(&p, 0x01, 1, 0, 0, 0, 0) > 0 ){ *x = p.x; *y = p.y; *z = p.z; *r = p.r; found = true; }
	}
	return found;
}

void Session::OnMessage(int peer, const uint8_t* buf, size_t len)
{
	if( role == ROLE_CLIENT ){ ClientOnMessage(buf, len); return; }
	if( (role != ROLE_HOST)||(om == nullptr) ){ return; }
	switch( (MsgType)peek_type(buf, len) ){
		case MsgType::HELLO: HostOnHello(peer, buf, len); break;
		case MsgType::CMD: HostOnCmd(peer, buf, len); break;
		case MsgType::BYE: OnPeerLeft(peer); break;
		default: break;
	}
}

int Session::FindPeer(int peerId) const
{
	for(int i=0; i<MAX_PEERS; i++){ if( peers[i].used && peers[i].peerId == peerId ){ return i; } }
	return -1;
}

void Session::HostOnHello(int peer, const uint8_t* buf, size_t len)
{
	Hello h;
	if( decode_hello(buf, len, &h) == false ){ return; }
	int index = FindPeer(peer);
	if( index < 0 ){
		for(int i=0; i<MAX_PEERS; i++){ if( peers[i].used == false ){ index = i; break; } }
		if( index < 0 ){ Send(peer, true, encode_bye(Bye())); return; }	//room full
		float x, y, z, r;
		if( PickSpawn(&x, &y, &z, &r) == false ){ x = 0; y = 0; z = 0; r = 0; }
		int param = 0, team = 0;
		om->GetPlayerHumanObject()->GetParamData(&param, NULL, NULL, &team);
		//free-for-all: every player is on a team of their own
		int slot = om->AddHumanIndex(x, y, z, r, param, 10 + index, DefaultLoadout, 0, false);
		if( slot < 0 ){ Send(peer, true, encode_bye(Bye())); return; }
		om->GetHumanObject(slot)->SetRxRy(r, 0.0f);
		om->SetNetControlled(slot, true);
		peers[index] = Peer();
		peers[index].used = true;
		peers[index].peerId = peer;
		peers[index].slot = slot;
		peers[index].last.rx = r;
		peers[index].scoreIndex = AddScore(slot, h.name.empty() ? std::string("PLAYER") : h.name);
		wasAlive[slot] = true;
		protectUntil[slot] = (int)tick + SPAWN_PROTECT_TICKS;
		om->GetHumanObject(slot)->SetInvincibleFlag(true);
	}
	//(re)send the welcome: a repeated HELLO means the first one was lost
	Welcome w;
	w.slot = (uint8_t)peers[index].slot;
	w.map = 0;
	w.tick = tick;
	w.seed = 0;
	Send(peer, true, encode_welcome(w));
	scoresDirty = true;
}

void Session::HostOnCmd(int peer, const uint8_t* buf, size_t len)
{
	int index = FindPeer(peer);
	if( index < 0 ){ return; }
	Peer& p = peers[index];
	std::vector<NetCmd> cmds;
	if( decode_cmd(buf, len, &cmds) == false ){ return; }
	for(const NetCmd& c : cmds){
		if( c.seq <= p.lastSeq ){ continue; }	//redundant copy of a command already queued
		p.lastSeq = c.seq;
		p.queue.push_back(c);
		while( (int)p.queue.size() > CMD_QUEUE_MAX ){ p.queue.pop_front(); }
	}
}

void Session::OnPeerLeft(int peer)
{
	if( role == ROLE_CLIENT ){ Stop(); return; }
	if( role != ROLE_HOST ){ return; }
	int index = FindPeer(peer);
	if( index < 0 ){ return; }
	Peer& p = peers[index];
	if( om != nullptr ){
		om->SetNetControlled(p.slot, false);
		om->RemoveHuman(p.slot);
	}
	if( (p.scoreIndex >= 0)&&(p.scoreIndex < (int)scores.size()) ){ scores[p.scoreIndex].used = false; }
	p = Peer();
	scoresDirty = true;
}

void Session::HostPreTick()
{
	if( (role != ROLE_HOST)||(om == nullptr) ){ return; }
	for(int i=0; i<MAX_PEERS; i++){
		Peer& p = peers[i];
		if( p.used == false ){ continue; }
		NetCmd c = p.last;
		if( p.queue.empty() == false ){ c = p.queue.front(); p.queue.pop_front(); }
		else{ c.buttons &= (BTN_FORWARD|BTN_BACK|BTN_LEFT|BTN_RIGHT|BTN_WALK|BTN_FIRE); }	//late packet: keep moving, no new presses
		HumanCommand hc;
		hc.held = ButtonsToHcmd(c.buttons);
		hc.pressed = (uint16_t)(hc.held & ~p.prevHeld);
		hc.rx = c.rx;
		hc.ry = c.ry;
		om->ApplyHumanCommand(p.slot, &hc);
		p.prevHeld = hc.held;
		p.last = c;
	}
}

void Session::HostPostTick()
{
	if( (role != ROLE_HOST)||(om == nullptr) ){ return; }
	tick++;

	//shots fired this tick
	int sid[MAX_NETSHOTLOG], sw[MAX_NETSHOTLOG];
	float srx[MAX_NETSHOTLOG], sry[MAX_NETSHOTLOG];
	int n = om->TakeNetShots(sid, sw, srx, sry, MAX_NETSHOTLOG);
	for(int i=0; i<n; i++){
		Event e; e.type = EventType::SHOT;
		e.shot.shooter = (uint8_t)sid[i]; e.shot.weapon = (uint8_t)sw[i]; e.shot.rx = srx[i]; e.shot.ry = sry[i];
		tickEvents.push_back(e);
	}

	//deaths, frags, respawns, spawn protection for every player
	for(PlayerScore& s : scores){
		if( s.used == false ){ continue; }
		int slot = s.slot;
		human* h = om->GetHumanObject(slot);
		bool alive = h->GetEnableFlag() && h->GetHP() > 0;
		if( wasAlive[slot] && (alive == false) ){
			bool head = false;
			int killer = om->GetLastAttacker(slot, &head);
			s.deaths++;
			PlayerScore* ks = (killer >= 0) ? ScoreForSlot(killer) : nullptr;
			if( ks != nullptr ){ if( killer == slot ){ ks->frags--; } else { ks->frags++; } }
			Event e; e.type = EventType::KILL;
			e.kill.killer = (uint8_t)((killer >= 0) ? killer : slot); e.kill.victim = (uint8_t)slot; e.kill.weapon = 255; e.kill.headshot = head ? 1 : 0;
			tickEvents.push_back(e);
			PushKill(e.kill.killer, slot, head);
			respawnAt[slot] = (int)tick + RESPAWN_TICKS;
			scoresDirty = true;
		}
		if( (alive == false)&&(respawnAt[slot] > 0)&&((int)tick >= respawnAt[slot]) ){
			float x, y, z, r;
			if( PickSpawn(&x, &y, &z, &r) ){
				bool net = om->GetNetControlled(slot);
				if( om->RespawnHuman(slot, x, y, z, r, DefaultLoadout) == slot ){
					om->SetNetControlled(slot, net);
					om->GetHumanObject(slot)->SetRxRy(r, 0.0f);
					om->GetHumanObject(slot)->SetInvincibleFlag(true);
					protectUntil[slot] = (int)tick + SPAWN_PROTECT_TICKS;
					for(int i=0; i<MAX_PEERS; i++){ if( peers[i].used && peers[i].slot == slot ){ peers[i].last.rx = r; } }
					Event e; e.type = EventType::RESPAWN; e.respawn.slot = (uint8_t)slot;
					tickEvents.push_back(e);
					alive = true;
				}
			}
			respawnAt[slot] = 0;
		}
		if( alive && protectUntil[slot] > 0 && (int)tick >= protectUntil[slot] ){
			h->SetInvincibleFlag(false);
			protectUntil[slot] = 0;
		}
		wasAlive[slot] = alive;
	}

	if( tickEvents.empty() == false ){
		//events are small; chunk to stay under the per-message count limit
		for(size_t i=0; i<tickEvents.size(); i+=32){
			std::vector<Event> part(tickEvents.begin()+i, tickEvents.begin()+std::min(tickEvents.size(), i+32));
			Send(-1, true, encode_events(part));
		}
		tickEvents.clear();
	}

	//one snapshot per client (ack differs per client)
	Snapshot snap;
	snap.tick = tick;
	for(int i=0; i<96; i++){
		NetHumanState s;
		if( om->GetNetHumanState(i, &s) == false ){ continue; }
		SnapHuman sh;
		sh.id = (uint8_t)i; sh.x = s.x; sh.y = s.y; sh.z = s.z; sh.rx = s.rx; sh.ry = s.ry;
		sh.hp = (int16_t)s.hp; sh.flags = (uint8_t)s.flags; sh.team = (int8_t)s.team;
		sh.param = (uint8_t)s.param; sh.weapon = (uint8_t)s.weapon; sh.move = (uint8_t)s.move;
		snap.humans.push_back(sh);
	}
	for(int i=0; i<MAX_PEERS; i++){
		if( peers[i].used == false ){ continue; }
		snap.ack_seq = peers[i].lastSeq;
		Send(peers[i].peerId, false, encode_snapshot(snap));
	}

	if( scoresDirty || (tick % SCORE_EVERY_TICKS) == 0 ){ SendScores(); scoresDirty = false; }
}

void Session::SendScores()
{
	std::vector<ScoreEntry> list;
	for(const PlayerScore& s : scores){
		if( s.used == false ){ continue; }
		ScoreEntry e; e.slot = (uint8_t)s.slot; e.frags = (int16_t)s.frags; e.deaths = (int16_t)s.deaths; e.name = s.name;
		list.push_back(e);
	}
	if( list.empty() ){ return; }
	Send(-1, true, encode_score(list));
}

//---------------------------------------------------------------- client

void Session::ClientSendCommand(const HumanCommand& cmd)
{
	if( (role != ROLE_CLIENT)||(localSlot < 0) ){ return; }
	NetCmd c;
	c.seq = ++cmdSeq;
	c.buttons = cmd.held;
	c.rx = cmd.rx;
	c.ry = cmd.ry;
	sentCmds.push_back(c);
	while( (int)sentCmds.size() > MAX_CMDS ){ sentCmds.pop_front(); }
	std::vector<NetCmd> v(sentCmds.begin(), sentCmds.end());
	Send(-1, false, encode_cmd(v));
}

void Session::ClientOnMessage(const uint8_t* buf, size_t len)
{
	switch( (MsgType)peek_type(buf, len) ){
		case MsgType::WELCOME: {
			Welcome w;
			if( decode_welcome(buf, len, &w) ){ localSlot = w.slot; }
			break;
		}
		case MsgType::SNAP: {
			Snapshot s;
			if( decode_snapshot(buf, len, &s) && ((haveSnap == false)||(s.tick > latest.tick)) ){
				latest = std::move(s);
				haveSnap = true;
			}
			break;
		}
		case MsgType::EVENT: {
			std::vector<Event> ev;
			if( decode_events(buf, len, &ev) ){ pendingEvents.insert(pendingEvents.end(), ev.begin(), ev.end()); }
			break;
		}
		case MsgType::SCORE: {
			std::vector<ScoreEntry> list;
			if( decode_score(buf, len, &list) ){
				scores.clear();
				for(const ScoreEntry& e : list){
					PlayerScore s; s.used = true; s.slot = e.slot; s.frags = e.frags; s.deaths = e.deaths; s.name = e.name;
					scores.push_back(s);
				}
			}
			break;
		}
		case MsgType::BYE: Stop(); break;
		default: break;
	}
}

void Session::ApplyClientEvent(const Event& e)
{
	switch( e.type ){
		case EventType::SHOT:
			om->SpawnShot(e.shot.shooter, e.shot.weapon, e.shot.rx, e.shot.ry);
			break;
		case EventType::KILL:
			PushKill(e.kill.killer, e.kill.victim, e.kill.headshot != 0);
			break;
		default: break;
	}
}

void Session::ClientApply()
{
	if( (role != ROLE_CLIENT)||(om == nullptr) ){ return; }
	tick++;
	//the transport may still be connecting when the mission starts: repeat HELLO until welcomed
	if( (localSlot < 0)&&((tick % 30) == 0) ){
		Hello h; h.name = localName;
		Send(-1, true, encode_hello(h));
	}
	if( localSlot >= 0 && om->GetPlayerID() != localSlot ){
		//until the first snapshot creates our human, keep the map's player slot
		if( om->GetHumanObject(localSlot)->GetEnableFlag() ){ om->SetPlayerID(localSlot); }
	}
	if( haveSnap ){
		bool present[96] = {};
		for(const SnapHuman& sh : latest.humans){
			NetHumanState s;
			s.id = sh.id; s.x = sh.x; s.y = sh.y; s.z = sh.z; s.rx = sh.rx; s.ry = sh.ry;
			s.hp = sh.hp; s.flags = sh.flags; s.team = sh.team; s.param = sh.param; s.weapon = sh.weapon; s.move = sh.move;
			om->ApplyNetHumanState(&s, sh.id == localSlot);
			if( sh.id < 96 ){ present[sh.id] = true; }
		}
		for(int i=0; i<96; i++){
			if( (present[i] == false)&&om->GetHumanObject(i)->GetEnableFlag() ){ om->RemoveHuman(i); }
		}
		if( localSlot >= 0 && om->GetPlayerID() != localSlot && om->GetHumanObject(localSlot)->GetEnableFlag() ){
			om->SetPlayerID(localSlot);
		}
		lastSnapTick = latest.tick;
		haveSnap = false;
	}
	om->ApplyNetPuppetMoves();
	for(const Event& e : pendingEvents){ ApplyClientEvent(e); }
	pendingEvents.clear();
}

}
