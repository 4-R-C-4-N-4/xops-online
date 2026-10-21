//[XOPS-ONLINE] HumanCommand: one tick of player-style control.
#ifndef HUMANCOMMAND_H
#define HUMANCOMMAND_H

//[XOPS-ONLINE] One tick of player-style control. The local keyboard and
//network peers both drive humans through ObjectManager::ApplyHumanCommand.
//held = keys down this tick; pressed = keys that went down this tick.
#define HCMD_FORWARD	0x0001
#define HCMD_BACK		0x0002
#define HCMD_LEFT		0x0004
#define HCMD_RIGHT		0x0008
#define HCMD_WALK		0x0010
#define HCMD_JUMP		0x0020
#define HCMD_FIRE		0x0040
#define HCMD_RELOAD		0x0080
#define HCMD_SWITCH		0x0100
#define HCMD_WEAPON1	0x0200
#define HCMD_WEAPON2	0x0400
#define HCMD_DROP		0x0800
#define HCMD_SCOPE		0x1000
#define HCMD_SHOTMODE	0x2000
//[XOPS-ONLINE] Replicated per-human state (host -> clients).
#define NETHUMAN_DEAD			0x01
#define NETHUMAN_SCOPE			0x04
#define NETHUMAN_INVINCIBLE		0x08
#define NETHUMAN_NOWEAPON		255
struct NetHumanState {
	int id;
	float x, y, z;
	float rx, ry;
	int hp;
	int flags;
	int team;
	int param;		//human type id
	int weapon;		//selected weapon param id, NETHUMAN_NOWEAPON if empty-handed
	int move;		//MOVEFLAG_* bits processed last tick (animation/extrapolation)
};

struct HumanCommand {
	unsigned short held;
	unsigned short pressed;
	float rx;
	float ry;
};

#endif
