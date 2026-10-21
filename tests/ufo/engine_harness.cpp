// Actual engine integration proof; only rendering, input, window and audio
// devices are stubbed. Keep upstream globals/utilities, but do not run GameEntry.
#define main openxops_original_entry
#include "../../src/main.cpp"
#undef main
#include "../../src/gamemain.h"
#include <cassert>
#include "net/session.h"
#include <initializer_list>
#include <map>
#include <array>
#include <set>
#include <tuple>
#include <vector>
#include <algorithm>
#include <cstdio>

static int kill_hostiles(ObjectManager& objects) {
    int team=0, count=0;
    objects.GetPlayerHumanObject()->GetParamData(nullptr,nullptr,nullptr,&team);
    for(int i=0;i<MAX_HUMAN;i++) {
        human* h=objects.GetHumanObject(i);
        int other=0; h->GetParamData(nullptr,nullptr,nullptr,&other);
        if(h->GetEnableFlag() && h->GetHP()>0 && team!=other) {
            h->SetHP(0); count++;
        }
    }
    return count;
}

int main(int argc, char** argv) {
    assert(argc==2);
    std::setvbuf(stdout,nullptr,_IONBF,0);
    InitRand();
    vfs_init_from_args(argv[1]);
    ParameterInfo params; params.InitInfo();
    WeaponParameter w;
    assert(params.GetWeapon(23,&w)==0 && w.attacks==64 && w.nbsmax==8 && w.scopemode==1);
    assert(params.GetWeapon(10,&w)==0 && w.attacks==65 && w.bullettype==0); // AK47
    assert(params.GetWeapon(11,&w)==0 && w.bullettype==0);
    for(int i=24;i<=26;i++) {
        assert(params.GetWeapon(i,&w)==0 && w.bullettype==i-22 && w.yakkyou_size==0);
    }
    assert(params.GetWeapon(53,&w)==0 && w.bullettype==0); // legacy BugWeapon fallback
    HumanParameter h;
    for(int i=43;i<=46;i++) {
        assert(params.GetHuman(i,&h)==0 && h.Weapon[0]==i-20 && h.Weapon[1]==ID_WEAPON_NONE);
    }
    for(int type=44;type<=46;type++) {
        for(int trial=0;trial<32;trial++) {
            human alien(&params);
            alien.SetParamData(type,0,1,1,true);
            alien.HitBulletUp(64);
            assert(alien.GetHP()>0); // Clean body hits still need follow-up shots.
            alien.SetParamData(type,0,1,1,true);
            alien.HitBulletHead(64);
            assert(alien.GetHP()<=0); // Every alien dies to one clean lever headshot.
        }
    }
    std::puts("ONE-SHOT LEVER HEADSHOTS OK");

    // UFO ESCAPE: a single thrown MOON ROCK kills the first guard only with a headshot.
    assert(params.GetWeapon(27,&w)==0 && strcmp(w.name,"MOON ROCK")==0);
    assert(w.throwaway && w.nbsmax==1 && w.bullettype==5 && w.attacks==45);
    assert(params.GetHuman(47,&h)==0 && h.Weapon[0]==27 && h.Weapon[1]==ID_WEAPON_NONE && h.texture==30 && h.hp==300);
    BulletParameter rockbolt;
    assert(params.GetBullet(5,&rockbolt)==0 && strcmp(rockbolt.model,"./data/model/weapon/rock.x")==0);
    for(int trial=0;trial<32;trial++) {
        human guard(&params);
        guard.SetParamData(44,0,1,1,true); guard.HitBulletHead(w.attacks); assert(guard.GetHP()<=0);
        guard.SetParamData(44,0,1,1,true); guard.HitBulletUp(w.attacks); assert(guard.GetHP()>0);
        guard.SetParamData(44,0,1,1,true); guard.HitBulletLeg(w.attacks); assert(guard.GetHP()>0);
    }
    for(int i=0;i<=26;i++){ assert(params.GetWeapon(i,&w)==0 && w.throwaway==false); }
    std::puts("MOON ROCK PARAMETERS OK");
    std::puts("PARAMETERS OK");

    // Free missions belong in the main list, including MIF-backed missions.
    char missionname[64], briefingpath[MAX_PATH];
    assert(params.GetOfficialMissionCount()==5);
    assert(params.GetOfficialMission(0,missionname,NULL,NULL,NULL,NULL,NULL)==0);
    assert(strcmp(missionname,"UFO FIELD")==0);
    assert(params.GetOfficialMissionBriefingPath(0,briefingpath)==0);
    assert(strcmp(briefingpath,"data\\ufofield\\ufo.mif")==0);
    assert(params.GetOfficialMission(1,missionname,NULL,NULL,NULL,NULL,NULL)==0);
    assert(strcmp(missionname,"UFO ESCAPE")==0);
    assert(params.GetOfficialMissionBriefingPath(1,briefingpath)==0);
    assert(strcmp(briefingpath,"data\\ufoescape\\escape.mif")==0);
    assert(params.GetOfficialMissionBriefingPath(2,briefingpath)==0);
    assert(strcmp(briefingpath,"data\\starter\\start.txt")==0);
    std::puts("UFO FIELD IN MAIN MISSION LIST OK");

    D3DGraphics graphics;
    ResourceManager resources; resources.SetParameterInfo(&params); resources.SetD3DGraphics(&graphics);
    SoundControl device;
    SoundManager sound(&device,&resources,&params);
    BlockDataInterface blocks; PointDataInterface points; Collision collision; MIFInterface mif;
    ObjectManager objects;
    objects.SetClass(&params,&graphics,&resources,&blocks,&points,&collision,&sound,&mif);
    assert(mif.LoadFiledata("data/ufofield/ufo.mif", true)==0);
    assert(points.LoadFiledata("data/ufofield/ufo.pd1")==0);
    objects.LoadPointData();
    assert(objects.GetStagedCount()==20 && objects.CountLivingHostiles()==0);
    assert(objects.CheckGameOverorComplete()==0);
    EventControl events(&points,&objects); events.Reset(-100);
    EventControl second(&points,&objects); second.Reset(-110);
    EventControl third(&points,&objects); third.Reset(-120);
    EventParameter state={};
    auto processEvents=[&]() {
        events.ProcessEventPoint(false,&state);
        second.ProcessEventPoint(false,&state);
        third.ProcessEventPoint(false,&state);
    };
    for(int wave=1;wave<=3;wave++) {
        const int expected=wave==2?8:6;
        for(int tick=0;tick<200 && objects.CountLivingHostiles()!=expected;tick++)
            processEvents();
        assert(objects.CountLivingHostiles()==expected);
        assert(objects.GetStagedCount()==(wave==1?14:wave==2?6:0));
        assert(objects.CheckGameOverorComplete()==0);
        assert(objects.DeployStagedGroup(wave)==0); // deploy is idempotent
        int flags=0;
        for(int i=0;i<MAX_HUMAN;i++) {
            if(objects.TakeAIInitFlag(i)) flags++;
            assert(!objects.TakeAIInitFlag(i));
        }
        assert(flags==expected);
        const short waiting=events.GetNextP4();
        for(int tick=0;tick<10;tick++) processEvents();
        assert(events.GetNextP4()==waiting); // event 40 blocks while hostiles live
        assert(kill_hostiles(objects)==expected);
        std::printf("WAVE %d OK: killed %d, pending %d\n",wave,expected,objects.GetStagedCount());
        if(wave<3) assert(objects.CheckGameOverorComplete()==0);
    }
    assert(objects.CheckGameOverorComplete()==1);
    std::puts("FINAL COMPLETION OK");
    objects.Cleanup();
    assert(objects.GetStagedCount()==0);
    for(int i=0;i<MAX_HUMAN;i++) assert(!objects.TakeAIInitFlag(i));
    assert(points.LoadFiledata("data/starter/start.pd1")==0);
    objects.LoadPointData();
    assert(objects.GetStagedCount()==0 && objects.CheckGameOverorComplete()==0);
    assert(kill_hostiles(objects)>0);
    assert(objects.CheckGameOverorComplete()==1);
    std::puts("CLASSIC NO-STAGING COMPLETION OK");
    objects.Cleanup();

    // Arrival cues must never make aliens investigate their own spawn.
    sound.InitWorldSound();
    sound.Teleport(0,32,-112,1);
    sound.PlayWorldSound(0,32,-112,0,1);
    soundlist heard[MAX_SOUNDMGR_LIST];
    assert(sound.GetWorldSound(0,32,-112,1,heard)==0);
    std::puts("ARRIVAL SOUND DOES NOT ALERT AI OK");

    // Exercise real AI navigation and collision, not just parsed waypoint ids.
    assert(blocks.LoadFiledata("data/ufofield/temp.bd1")==0);
    blocks.CalculationBlockdata(false);
    collision.InitCollision(&blocks);
    assert(points.LoadFiledata("data/ufofield/ufo.pd1")==0);
    objects.LoadPointData();
    assert(objects.DeployStagedGroup(1)==6);
    AIcontrol bots[6];
    for(int i=0;i<6;i++) {
        bots[i].SetClass(&objects,i+1,&blocks,&points,&params,&collision,&sound);
        bots[i].Init(); bots[i].SetNoFightFlag(true);
    }
    for(int tick=0;tick<400;tick++) {
        objects.Process(-1,false,false);
        for(auto& bot:bots) bot.Process();
        sound.PlayWorldSound(0,20,200,0,0);
    }
    int descended=0;
    for(int i=1;i<=6;i++) {
        float x,y,z;objects.GetHumanObject(i)->GetPosData(&x,&y,&z,nullptr);
        std::printf("ALIEN %d POS %.1f %.1f %.1f\n",i,x,y,z);
        if(y<5 && z>-40) descended++;
    }
    assert(descended>=4);
    std::puts("RAMP DESCENT OK");
    objects.Cleanup();

    // Combat-pressure probe: an exposed rancher standing still in the open
    // while one live wave hunts with real AI, weapons and collision.
    auto pressure=[&](int wave, unsigned seed) {
        srand(seed);
        objects.LoadPointData();
        int ids[8], n=0;
        for(int i=0;i<MAX_HUMAN;i++) objects.TakeAIInitFlag(i);
        n=objects.DeployStagedGroup(wave);
        for(int i=0,k=0;i<MAX_HUMAN && k<n;i++) if(objects.TakeAIInitFlag(i)) ids[k++]=i;
        human* player=objects.GetPlayerHumanObject();
        player->SetPosData(100,0,170,(float)M_PI/2);
        AIcontrol ai[8];
        for(int k=0;k<n;k++){ ai[k].SetClass(&objects,ids[k],&blocks,&points,&params,&collision,&sound); ai[k].Init(); }
        int tick=0;
        for(;tick<1000 && player->GetHP()>0;tick++){
            objects.Process(-1,false,false);
            for(int k=0;k<n;k++) ai[k].Process();
            sound.PlayWorldSound(100,20,170,0,0);
        }
        objects.Cleanup();
        return tick;
    };
    for(int wave=1;wave<=3;wave++){
        int total=0, fastest=100000;
        for(unsigned seed=1;seed<=5;seed++){ int t=pressure(wave,seed); total+=t; if(t<fastest) fastest=t; }
        std::printf("PRESSURE WAVE %d: exposed death after mean %.1fs, fastest %.1fs\n",wave,total/5*0.03,fastest*0.03);
        assert(total/5*0.03>=10.0 && fastest*0.03>=7.0); // Playtest: aliens previously killed in ~1-2s of contact.
    }
    std::puts("EXPOSED PLAYER SURVIVAL OK");

    // A chest-height laser must stop at a cow body, not pass through a tiny
    // ground-level legacy sphere. Removing the cow must reopen the shot.
    objects.LoadPointData();
    for(int i=0;i<MAX_SMALLOBJECT;i++) objects.GetSmallObject(i)->SetEnableFlag(false);
    smallobject* cover=objects.GetSmallObject(0);
    cover->SetPosData(0,0,0,0); cover->SetEnableFlag(true);
    int empty[2]={0,0};
    int targetid=objects.AddHumanIndex(15,0,0,0,43,1,empty,30,false);
    assert(targetid>0);
    auto shoot=[&](int index, float z=0, float height=16) {
        human* target=objects.GetHumanObject(targetid); target->SetHP(120);
        target->SetPosData(15,height-16,z,0);
        bullet* shot=objects.GetBulletObject(index);
        shot->SetPosData(-15,height,z,0,0); shot->SetParamData(50,0,40,0,0,0,true); shot->SetEnableFlag(true);
        objects.Process(-1,false,false);
        return target->GetHP();
    };
    assert(shoot(0)==120);
    assert(shoot(1,8)==120); // Along the cow's long body.
    cover->SetPosData(0,0,0,(float)M_PI/2);
    assert(shoot(2,8)<120);  // Outside the now-rotated body's narrow side.
    assert(shoot(3)==120);
    assert(shoot(4,0,26)<120); // A shot above the back is not blocked.
    cover->HitBullet(1000);
    assert(shoot(5)<120); // Destroyed cows leave no invisible cover.
    assert(mif.LoadFiledata("data/starter/start.txt",false)==0);
    assert(mif.GetAddSmallobjectCover(0)==nullptr); // No hull leaks into classic data.
    std::puts("COW BODY STOPS LASERS OK");
    objects.Cleanup();

    // [XOPS-ONLINE] HumanCommand seam: a command-driven human moves exactly
    // like one driven by direct ObjectManager calls; semi-auto fires once
    // per press, full-auto while held; puppet worlds never apply damage.
    {
        objects.LoadPointData();
        for(int i=0;i<MAX_HUMAN;i++) if(i!=objects.GetPlayerID()) objects.GetHumanObject(i)->SetEnableFlag(false);
        int lever[2]={23,ID_WEAPON_NONE}, ak[2]={10,ID_WEAPON_NONE};
        int a=objects.AddHumanIndex(-40,0,140,0,43,0,lever,40,false);
        int b=objects.AddHumanIndex(40,0,140,0,43,0,lever,41,false);
        assert(a>=0 && b>=0);
        // Same human, same start, same tick sequence: once via commands, once direct.
        auto run=[&](bool viaCommand, float* dx, float* dy, float* dz){
            human* h=objects.GetHumanObject(a);
            h->SetPosData(-40,0,140,0); h->SetRxRy(0.7f,0.0f);
            for(int t=0;t<30;t++) objects.Process(-1,false,false);
            float x0,y0,z0,x1,y1,z1; h->GetPosData(&x0,&y0,&z0,NULL);
            HumanCommand c; c.rx=0.7f; c.ry=0.0f;
            for(int t=0;t<40;t++){
                c.held = (t<25 ? HCMD_FORWARD : HCMD_BACK) | (t>10 && t<20 ? HCMD_LEFT : 0) | (t>30 ? HCMD_WALK : 0);
                c.pressed = (t==5 ? HCMD_JUMP : 0);
                if(viaCommand) objects.ApplyHumanCommand(a,&c);
                else {
                    h->SetRxRy(0.7f,0.0f);
                    if(t<25) objects.MoveForward(a); else objects.MoveBack(a);
                    if(t>10 && t<20) objects.MoveLeft(a);
                    if(t>30) objects.MoveWalk(a);
                    if(t==5) objects.MoveJump(a);
                }
                objects.Process(-1,false,false);
            }
            for(int t=0;t<30;t++) objects.Process(-1,false,false);
            h->GetPosData(&x1,&y1,&z1,NULL); *dx=x1-x0; *dy=y1-y0; *dz=z1-z0;
        };
        float ax,ay,az,bx,by,bz;
        run(true,&ax,&ay,&az); run(false,&bx,&by,&bz);
        std::printf("CMD HUMAN moved (%.3f,%.3f,%.3f) direct (%.3f,%.3f,%.3f)\n",ax,ay,az,bx,by,bz);
        assert(fabsf(ax-bx)<1e-3f && fabsf(ay-by)<1e-3f && fabsf(az-bz)<1e-3f);
        assert(fabsf(ax)+fabsf(az)>10.0f);
        std::puts("COMMAND MOVEMENT MATCHES DIRECT CONTROL OK");

        auto count_shots=[&](int id,int ticks,bool tapOnce){
            int shots=0; HumanCommand f; f.held=HCMD_FIRE; f.rx=0; f.ry=0;
            for(int t=0;t<ticks;t++){ f.pressed=(t==0||!tapOnce)?HCMD_FIRE:0; if(!tapOnce) f.pressed=(t%2==0)?HCMD_FIRE:0;
                if(objects.ApplyHumanCommand(id,&f)==1) shots++; objects.Process(-1,false,false); }
            return shots;
        };
        auto give=[&](int id,int w,int loaded){ assert(objects.CheatNewWeapon(id,w)); int s; weapon* hw[TOTAL_HAVEWEAPON];
            objects.GetHumanObject(id)->GetWeapon(&s,hw,NULL,NULL); hw[s]->SetParamData(w,loaded,loaded,false); };
        give(a,23,8);
        assert(objects.GetHumanObject(a)->GetWeaponBlazingmode()==false);
        int semi=count_shots(a,60,true);
        give(b,10,30);
        assert(objects.GetHumanObject(b)->GetWeaponBlazingmode()==true);
        int full=count_shots(b,60,true);
        std::printf("SEMI held 60 ticks: %d shot(s); FULL held 60 ticks: %d shots\n",semi,full);
        assert(semi==1 && full>5);
        std::puts("COMMAND FIRE MODES OK");

        objects.SetNetControlled(a,true);
        assert(objects.GetNetControlled(a) && !objects.GetNetControlled(b) && !objects.GetNetControlled(-1) && !objects.GetNetControlled(MAX_HUMAN));
        objects.SetNetControlled(a,false);

        // Puppet worlds: a headshot shows effects but never changes HP.
        auto headshot=[&](bool puppet){
            objects.SetNetPuppet(puppet);
            human* t=objects.GetHumanObject(b); t->SetHP(200); t->SetPosData(40,0,140,0);
            for(int i=0;i<MAX_BULLET;i++) objects.GetBulletObject(i)->SetEnableFlag(false);
            bullet* s=objects.GetBulletObject(0);
            s->SetPosData(10,18.5f,140,0,0); s->SetParamData(64,1,40,9,a,0,true); s->SetEnableFlag(true);
            for(int k=0;k<4;k++) objects.Process(-1,false,false);
            return t->GetHP();
        };
        int real=headshot(false), puppet=headshot(true);
        objects.SetNetPuppet(false);
        std::printf("HEADSHOT HP host %d puppet %d\n",real,puppet);
        assert(real<200 && puppet==200);
        std::puts("PUPPET WORLD APPLIES NO DAMAGE OK");
        objects.Cleanup();
    }

    // [XOPS-ONLINE] Two worlds in one process: a host world and a client
    // puppet world sharing the map. Replicating NetHumanState each tick keeps
    // positions, deaths, respawns (same slot) and held weapons in step.
    {
        ObjectManager client;
        client.SetClass(&params,&graphics,&resources,&blocks,&points,&collision,&sound,&mif);
        objects.LoadPointData(); client.LoadPointData();
        client.SetNetPuppet(true);
        int lever[2]={23,ID_WEAPON_NONE}, ak[2]={10,ID_WEAPON_NONE};
        int p1=objects.AddHumanIndex(-40,0,120,0,43,1,lever,50,false);
        int p2=objects.AddHumanIndex(40,0,120,0,43,2,ak,51,false);
        assert(p1>=0 && p2>=0);
        auto sync=[&](){
            for(int i=0;i<MAX_HUMAN;i++){
                NetHumanState s;
                if(objects.GetNetHumanState(i,&s)) assert(client.ApplyNetHumanState(&s,false)>=0);
                else if(client.GetHumanObject(i)->GetEnableFlag()) client.RemoveHuman(i);
            }
        };
        auto pos=[&](ObjectManager& w,int id,float* x,float* y,float* z){ w.GetHumanObject(id)->GetPosData(x,y,z,NULL); };
        HumanCommand run; run.held=HCMD_FORWARD|HCMD_LEFT; run.pressed=0; run.rx=1.1f; run.ry=0;
        int created=0;
        for(int t=0;t<60;t++){
            objects.ApplyHumanCommand(p1,&run);
            objects.Process(-1,false,false);
            if(t%2==0){ for(int i=0;i<MAX_HUMAN;i++){ NetHumanState s; if(objects.GetNetHumanState(i,&s)) created+=client.ApplyNetHumanState(&s,false)==1; } }
            client.ApplyNetPuppetMoves();
            client.Process(-1,false,false);
        }
        sync();
        float hx,hy,hz,cx,cy,cz;
        pos(objects,p1,&hx,&hy,&hz); pos(client,p1,&cx,&cy,&cz);
        std::printf("REPLICA host (%.2f,%.2f,%.2f) client (%.2f,%.2f,%.2f), created %d\n",hx,hy,hz,cx,cy,cz,created);
        assert(fabsf(hx-cx)<1e-4f && fabsf(hy-cy)<1e-4f && fabsf(hz-cz)<1e-4f);
        assert(created>=2);
        NetHumanState cs; assert(client.GetNetHumanState(p2,&cs) && cs.weapon==10 && cs.team==2);
        // Between snapshots the puppet keeps walking with the host's last move flags.
        objects.ApplyHumanCommand(p1,&run); objects.Process(-1,false,false); sync();
        pos(client,p1,&cx,&cy,&cz);
        client.ApplyNetPuppetMoves(); client.Process(-1,false,false);
        float ex,ey,ez; pos(client,p1,&ex,&ey,&ez);
        { NetHumanState hs; objects.GetNetHumanState(p1,&hs); std::printf("EXTRAP move flags %d, moved %.3f\n",hs.move,fabsf(ex-cx)+fabsf(ez-cz)); }
        assert(fabsf(ex-cx)+fabsf(ez-cz)>0.05f); // accelerates from rest after a snap; smoothing is phase 6
        std::puts("PUPPET REPLICATES POSITION, WEAPON, TEAM OK");

        // Kill on the host -> dead on the client; the client never kills by itself.
        objects.GetHumanObject(p2)->SetHP(1);
        objects.GetHumanObject(p2)->HitBulletHead(500);
        for(int t=0;t<5;t++){ objects.Process(-1,false,false); sync(); client.Process(-1,false,false); }
        assert(objects.GetHumanObject(p2)->GetHP()<=0);
        assert(client.GetHumanObject(p2)->GetHP()<=0);
        // Respawn in the same slot with a new weapon at a new place.
        int lev[2]={23,ID_WEAPON_NONE};
        assert(objects.RespawnHuman(p2,0,0,60,0,lev)==p2);
        assert(objects.GetHumanObject(p2)->GetHP()>0);
        sync(); client.Process(-1,false,false);
        assert(client.GetHumanObject(p2)->GetHP()>0);
        assert(client.GetNetHumanState(p2,&cs) && cs.weapon==23 && fabsf(cs.z-60)<2.0f);
        // Leaving players disappear.
        objects.RemoveHuman(p2); sync();
        assert(!client.GetHumanObject(p2)->GetEnableFlag());
        std::puts("PUPPET DEATH, RESPAWN, REMOVAL OK");
        client.Cleanup(); objects.Cleanup();
    }

    // [XOPS-ONLINE] Full session over an in-memory "transport" on DM HANGAR:
    // join, command-driven movement, replicated snapshot, frags, killfeed,
    // same-slot respawn after 3 s, scores on both sides, leaving.
    {
        assert(blocks.LoadFiledata("data/dmhangar/temp.bd1")==0);
        blocks.CalculationBlockdata(false); collision.InitCollision(&blocks);
        assert(points.LoadFiledata("data/dmhangar/hangar.pd1")==0);
        ObjectManager clientWorld;
        clientWorld.SetClass(&params,&graphics,&resources,&blocks,&points,&collision,&sound,&mif);
        objects.LoadPointData(); clientWorld.LoadPointData();
        xnet::Session host, cli;
        host.ArmHost("HOST"); host.Attach(&objects,&points);
        cli.ArmClient("ALICE"); cli.Attach(&clientWorld,&points);
        const int PEER=7;
        size_t bytesDown=0, snaps=0;
        auto pump=[&](){
            xnet::Outgoing o;
            while(cli.PollOutgoing(&o)) host.OnMessage(PEER,o.bytes.data(),o.bytes.size());
            while(host.PollOutgoing(&o)){ assert(o.peer==-1||o.peer==PEER); bytesDown+=o.bytes.size();
                if(xnet::peek_type(o.bytes.data(),o.bytes.size())==(uint8_t)xnet::MsgType::SNAP) snaps++;
                cli.OnMessage(0,o.bytes.data(),o.bytes.size()); }
        };
        auto step=[&](const HumanCommand* c){
            if(c) cli.ClientSendCommand(*c);
            pump();
            host.HostPreTick(); objects.Process(-1,false,false); host.HostPostTick();
            pump();
            cli.ClientApply(); clientWorld.Process(-1,false,false);
        };
        for(int t=0;t<5;t++) step(nullptr);
        int me=cli.GetLocalSlot();
        assert(me>0 && me!=objects.GetPlayerID() && objects.GetNetControlled(me));
        assert(clientWorld.GetPlayerID()==me && clientWorld.GetNetPuppet());
        // The client walks forward for 2 s via commands; the host world moves it.
        float x0,y0,z0; objects.GetHumanObject(me)->GetPosData(&x0,&y0,&z0,NULL);
        float face; objects.GetHumanObject(me)->GetRxRy(&face,NULL);
        HumanCommand go; go.held=HCMD_FORWARD; go.pressed=0; go.rx=face; go.ry=0;
        for(int t=0;t<60;t++) step(&go);
        float hx,hy,hz,cx,cy,cz;
        objects.GetHumanObject(me)->GetPosData(&hx,&hy,&hz,NULL);
        clientWorld.GetHumanObject(me)->GetPosData(&cx,&cy,&cz,NULL);
        float moved=sqrtf((hx-x0)*(hx-x0)+(hz-z0)*(hz-z0)), drift=sqrtf((hx-cx)*(hx-cx)+(hz-cz)*(hz-cz));
        std::printf("SESSION client moved %.1f on host; client view off by %.2f; %zu snapshots, %zu bytes down\n",moved,drift,snaps,bytesDown);
        assert(moved>5.0f && drift<2.0f && snaps>=60);
        // The host kills the client: frag, death, killfeed, mirrored scores.
        human* victim=objects.GetHumanObject(me);
        for(int t=0;t<50;t++) step(nullptr);  // spawn protection over
        assert(victim->GetInvincibleFlag()==false);
        float vx,vy,vz; victim->GetPosData(&vx,&vy,&vz,NULL);
        for(int i=0;i<MAX_BULLET;i++) objects.GetBulletObject(i)->SetEnableFlag(false);
        bullet* b=objects.GetBulletObject(0);
        b->SetPosData(vx-20,vy+18.5f,vz,0,0); b->SetParamData(500,1,40,0,objects.GetPlayerID(),1,true); b->SetEnableFlag(true);
        for(int t=0;t<5;t++) step(nullptr);
        assert(victim->GetHP()<=0);
        assert(clientWorld.GetHumanObject(me)->GetHP()<=0);
        auto frags=[&](const xnet::Session& s,int slot,int* f,int* d){ for(auto& e:s.GetScores()) if(e.used&&e.slot==slot){*f=e.frags;*d=e.deaths;return true;} return false; };
        int hf=0,hd=0,cf=0,cd=0;
        for(int t=0;t<31;t++) step(nullptr);
        assert(frags(host,objects.GetPlayerID(),&hf,&hd) && hf==1);
        assert(frags(cli,objects.GetPlayerID(),&cf,&cd) && cf==1);
        assert(frags(cli,me,&cf,&cd) && cd==1);
        assert(!cli.GetKillFeed().empty() && cli.GetKillFeed().back().victim==me && cli.GetKillFeed().back().headshot);
        // Same-slot respawn after 3 s, alive on both sides.
        for(int t=0;t<70;t++) step(nullptr);
        assert(objects.GetHumanObject(me)->GetHP()>0 && clientWorld.GetHumanObject(me)->GetHP()>0);
        assert(objects.GetNetControlled(me));
        std::puts("SESSION FRAG, KILLFEED, SCORES, RESPAWN OK");
        // Host shots replay on the client as projectiles (no damage there).
        for(int i=0;i<MAX_BULLET;i++) clientWorld.GetBulletObject(i)->SetEnableFlag(false);
        { int s; weapon* hw[TOTAL_HAVEWEAPON]; objects.GetPlayerHumanObject()->GetWeapon(&s,hw,NULL,NULL); assert(hw[s]); }
        HumanCommand fire; fire.held=HCMD_FIRE; fire.pressed=HCMD_FIRE; fire.rx=0; fire.ry=0;
        objects.ApplyHumanCommand(objects.GetPlayerID(),&fire);
        step(nullptr);
        int flying=0; for(int i=0;i<MAX_BULLET;i++) if(clientWorld.GetBulletObject(i)->GetEnableFlag()) flying++;
        assert(flying>0);
        std::puts("SESSION SHOT REPLAY OK");
        // Leaving removes the player everywhere.
        host.OnPeerLeft(PEER);
        step(nullptr);
        assert(!objects.GetHumanObject(me)->GetEnableFlag());
        std::puts("SESSION LEAVE OK");
        host.Stop(); cli.Stop();
        clientWorld.Cleanup(); objects.Cleanup();
    }

    // A thrown rock empties the hand: no reserve, slot freed for a laser pickup.
    objects.LoadPointData();
    int rockonly[2]={27,ID_WEAPON_NONE};
    int captive=objects.AddHumanIndex(0,0,180,0,47,0,rockonly,7,false);
    assert(captive>=0);
    weapon* held[TOTAL_HAVEWEAPON]; int loaded[TOTAL_HAVEWEAPON], total[TOTAL_HAVEWEAPON], selected=0;
    objects.GetHumanObject(captive)->GetWeapon(&selected,held,loaded,total);
    int rockid=-1,rockloaded=-1,rocktotal=-1;
    assert(held[0]!=nullptr); held[0]->GetParamData(&rockid,&rockloaded,&rocktotal);
    assert(rockid==27 && rockloaded==1 && rocktotal==1);
    for(int i=0;i<MAX_BULLET;i++) objects.GetBulletObject(i)->SetEnableFlag(false);
    objects.ShotWeapon(captive);
    objects.GetHumanObject(captive)->GetWeapon(&selected,held,loaded,total);
    assert(held[0]==nullptr && held[1]==nullptr);
    int flying=-1;
    for(int i=0;i<MAX_BULLET;i++) if(objects.GetBulletObject(i)->GetEnableFlag()) flying=i;
    assert(flying>=0 && objects.IsBulletHiddenInFirstPerson(flying)==false); // The rock stays visible.
    std::puts("THROWN ROCK EMPTIES HAND OK");

    // The player's own lasers are seen end-on in first person and cover the
    // crosshair, so they are hidden there; other shooters' lasers are not.
    int zap[2]={24,ID_WEAPON_NONE};
    int alien=objects.AddHumanIndex(30,0,180,0,44,1,zap,8,false);
    human* me=objects.GetPlayerHumanObject();
    me->SetWeapon(held,loaded,total); // drop whatever the map gave; give a zap
    assert(objects.CheatNewWeapon(objects.GetPlayerID(),24));
    { int s; weapon* hw[TOTAL_HAVEWEAPON]; me->GetWeapon(&s,hw,NULL,NULL); assert(hw[s]); hw[s]->SetParamData(24,12,36,false); }
    auto fired=[&](int shooter){
        for(int i=0;i<MAX_BULLET;i++) objects.GetBulletObject(i)->SetEnableFlag(false);
        for(int t=0;t<40;t++){ if(objects.ShotWeapon(shooter)) break; objects.Process(-1,false,false); }
        for(int i=0;i<MAX_BULLET;i++) if(objects.GetBulletObject(i)->GetEnableFlag()) return i;
        return -1;
    };
    int mine=fired(objects.GetPlayerID());
    assert(mine>=0 && objects.IsBulletHiddenInFirstPerson(mine));
    int theirs=fired(alien); assert(theirs>=0 && objects.IsBulletHiddenInFirstPerson(theirs)==false);
    std::puts("OWN LASER HIDDEN IN FIRST PERSON OK");
    objects.Cleanup();

    // UFO ESCAPE must be sealed: from any point a player could stand (floor
    // below, ceiling above), every view ray hits geometry. A ray that
    // escapes shows the black void in game.
    {
        BlockDataInterface ship; Collision shipcoll;
        assert(ship.LoadFiledata("data/ufoescape/temp.bd1")==0);
        ship.CalculationBlockdata(false); shipcoll.InitCollision(&ship);
        int inside=0, leaks=0;
        for(float y0 : {0.0f, 44.0f}) for(float x=-80;x<=580;x+=6) for(float z=-200;z<=410;z+=6){
            float ey=y0+16, d;
            if(!shipcoll.CheckALLBlockIntersectRay(x,ey,z,0,-1,0,NULL,NULL,&d,24.0f)) continue;
            if(d<10.0f) continue; // inside a solid block or on a raised prop
            if(shipcoll.CheckALLBlockInside(x,ey,z)) continue;
            if(!shipcoll.CheckALLBlockIntersectRay(x,ey,z,0,1,0,NULL,NULL,&d,80.0f)) continue;
            inside++;
            int here=0;
            for(int a=0;a<16;a++) for(int e=-1;e<=2;e++){
                float yaw=a*(float)M_PI/8, pitch=e*0.45f;
                float vx=cosf(yaw)*cosf(pitch), vy=sinf(pitch), vz=sinf(yaw)*cosf(pitch);
                if(!shipcoll.CheckALLBlockIntersectRay(x,ey,z,vx,vy,vz,NULL,NULL,&d,2000.0f)){
                    if(getenv("VOID_DETAIL")) std::printf("VOID RAY %.0f %.0f %.0f %.2f %.2f %.2f\n",x,ey,z,vx,vy,vz);
                    leaks++; here++;
                }
            }
            if(here && leaks-here<200) std::printf("VOID LEAK at (%.0f,%.0f,%.0f): %d rays\n",x,ey,z,here);
        }
        std::printf("ESCAPE SHIP: %d interior samples, %d leaking rays\n",inside,leaks);
        assert(inside>1000 && leaks==0);
        std::puts("ESCAPE SHIP SEALED OK");

        // Walkability on the real collision geometry. A node is a standing
        // spot: floor found by a downward ray, 20 units of headroom, and the
        // body column (floor+2..+19) not inside any block. Neighbours (4-unit
        // grid) connect when the floor step is small enough to walk and
        // body-height rays between them are clear.
        const float G=4.0f;
        auto floorAt=[&](float x,float top,float z,float* fy)->bool{
            float d; if(!shipcoll.CheckALLBlockIntersectRay(x,top,z,0,-1,0,NULL,NULL,&d,top+50)) return false;
            *fy=top-d; return true;
        };
        auto standable=[&](float x,float fy,float z)->bool{
            float d;
            for(float h : {2.0f,10.0f,19.0f}) if(shipcoll.CheckALLBlockInside(x,fy+h,z)) return false;
            if(shipcoll.CheckALLBlockIntersectRay(x,fy+2,z,0,1,0,NULL,NULL,&d,20.0f)) return false;
            for(int a=0;a<8;a++){ float yaw=a*(float)M_PI/4;
                if(shipcoll.CheckALLBlockIntersectRay(x,fy+10,z,cosf(yaw),0,sinf(yaw),NULL,NULL,&d,2.8f)) return false; }
            return true;
        };
        auto clearBetween=[&](float x0,float y0,float z0,float x1,float y1,float z1)->bool{
            float dx=x1-x0,dz=z1-z0,len=sqrtf(dx*dx+dz*dz),d; if(len<1e-3f) return true;
            for(float h : {6.0f,12.0f,18.0f}){
                float yh=std::max(y0,y1)+h;
                if(shipcoll.CheckALLBlockIntersectRay(x0,yh,z0,dx/len,0,dz/len,NULL,NULL,&d,len)) return false;
            }
            return true;
        };
        struct Node{int i,j;float y;};
        std::map<std::tuple<int,int,int>,float> nodes; // (i,j,level)->floor y
        auto key=[&](int i,int j,float y){ return std::make_tuple(i,j,(int)floorf(y/6.0f)); };
        for(int i=-20;i<=145;i++) for(int j=-50;j<=103;j++){
            float x=i*G,z=j*G,top=120.0f,fy;
            for(int layer=0;layer<6 && floorAt(x,top,z,&fy);layer++){
                if(standable(x,fy,z)) nodes[key(i,j,fy)]=fy;
                top=fy-0.5f; // continue below this surface (through its slab)
                float d; while(top>-10 && shipcoll.CheckALLBlockInside(x,top,z)) top-=1.0f;
                (void)d;
            }
        }
        auto nearest=[&](float x,float y,float z)->std::tuple<int,int,int>{
            std::tuple<int,int,int> best; float bd=1e9f;
            for(auto& kv : nodes){ int i,j,l; std::tie(i,j,l)=kv.first;
                float d=fabsf(i*G-x)+fabsf(j*G-z)+fabsf(kv.second-y)*2; if(d<bd){bd=d;best=kv.first;} }
            assert(bd<12.0f); return best;
        };
        std::set<std::tuple<int,int,int>> seen; std::vector<std::tuple<int,int,int>> stack;
        auto start=nearest(-42,0,390); seen.insert(start); stack.push_back(start);
        while(!stack.empty()){
            auto k=stack.back(); stack.pop_back(); int i,j,l; std::tie(i,j,l)=k; float y=nodes[k];
            for(int n=0;n<4;n++){ int ni=i+(n==0)-(n==1), nj=j+(n==2)-(n==3);
                for(int nl=l-1;nl<=l+1;nl++){ auto nk=std::make_tuple(ni,nj,nl); auto it=nodes.find(nk);
                    if(it==nodes.end()||seen.count(nk)) continue;
                    if(fabsf(it->second-y)>3.5f) continue;
                    if(!clearBetween(i*G,y,j*G,ni*G,it->second,nj*G)) continue;
                    seen.insert(nk); stack.push_back(nk); } }
        }
        auto reachable=[&](float x,float y,float z){ return seen.count(nearest(x,y,z))>0; };
        if(getenv("ROUTE_DETAIL")){
            std::map<int,int> maxx; for(auto& k : seen){ int i,j,l; std::tie(i,j,l)=k; int lvl=(int)nodes[k]; maxx[lvl/10]=std::max(maxx[lvl/10],i*(int)G); }
            for(auto& kv:maxx) std::printf("REACHED floor~%d0: max x %d\n",kv.first,kv.second);
            std::printf("nodes %zu seen %zu\n",nodes.size(),seen.size());
            int minz=99999; for(auto& k : seen){ int i,j,l; std::tie(i,j,l)=k; minz=std::min(minz,j*(int)G); }
            std::printf("seen min z %d\n",minz);
            for(int i=-5;i<=8;i++){ int jj=minz/(int)G-1; for(int l=-1;l<=1;l++){ auto k=std::make_tuple(i,jj,l); if(nodes.count(k)) std::printf("node beyond (%d,%d) y %.1f\n",i*(int)G,jj*(int)G,nodes[k]); } }
        }
        assert(reachable(502,46,-174));  // escape pod
        std::puts("ESCAPE ROUTE WALKABLE OK");
        int catwalk=0; for(auto& k : seen) if(nodes[k]>18 && nodes[k]<36){ int i,j,l; std::tie(i,j,l)=k; if(i*G<=180 && j*G<=80 && j*G>=-60) catwalk++; }
        assert(catwalk>20); // the lab catwalk is reachable by its ramp
        std::puts("LAB CATWALK REACHABLE OK");

        // Every human, staged human and waypoint stands on a reachable spot,
        // and each patrol leg is a straight walkable line (AI walks straight).
        PointDataInterface shippoints; assert(shippoints.LoadFiledata("data/ufoescape/escape.pd1")==0);
        std::map<int,pointdata> waypoints;
        for(int n=0;n<shippoints.GetTotaldatas();n++){ pointdata p; shippoints.Getdata(&p,n); if(p.p1==3) waypoints[p.p4]=p; }
        int bad=0;
        for(int n=0;n<shippoints.GetTotaldatas();n++){
            pointdata p; shippoints.Getdata(&p,n);
            if(!(p.p1==1||p.p1==6||p.p1==50||p.p1==3)) continue;
            float fy; floorAt(p.x,p.y+8,p.z,&fy);
            if(!standable(p.x,fy,p.z) || !reachable(p.x,fy,p.z)){
                std::printf("STUCK POINT p1=%d at (%.0f,%.0f,%.0f)\n",p.p1,p.x,p.y,p.z); bad++; }
            if(p.p1==3 && waypoints.count(p.p3) && p.p3!=p.p4){
                pointdata q=waypoints[p.p3]; float qy; floorAt(q.x,q.y+8,q.z,&qy);
                if(!clearBetween(p.x,fy,p.z,q.x,qy,q.z)){
                    std::printf("BLOCKED PATROL LEG %d->%d (%.0f,%.0f)->(%.0f,%.0f)\n",p.p4,p.p3,p.x,p.z,q.x,q.z); bad++; }
            }
        }
        assert(bad==0);
        std::puts("ESCAPE ACTORS NOT STUCK OK");

        // Real human physics (the player's own movement code): walk the
        // lab exit, climb the catwalk ramp and the switchback to the bridge.
        BlockDataInterface* oldblocks=&blocks; (void)oldblocks;
        assert(blocks.LoadFiledata("data/ufoescape/temp.bd1")==0);
        blocks.CalculationBlockdata(false); collision.InitCollision(&blocks);
        assert(points.LoadFiledata("data/ufoescape/escape.pd1")==0);
        objects.LoadPointData();
        auto walk=[&](float sx,float sy,float sz,std::vector<std::array<float,2>> legs,float wantY,const char* name){
            human* p=objects.GetPlayerHumanObject();
            p->SetPosData(sx,sy,sz,0);
            for(int t=0;t<20;t++) objects.Process(-1,false,false);
            size_t leg=0; float x,y,z;
            for(int tick=0;tick<1200 && leg<legs.size();tick++){
                p->GetPosData(&x,&y,&z,NULL);
                float dx=legs[leg][0]-x, dz=legs[leg][1]-z;
                if(dx*dx+dz*dz<9.0f){ leg++; continue; }
                if(getenv("WALK_DETAIL") && tick%10==0) std::printf("  t%d (%.1f,%.2f,%.1f)\n",tick,x,y,z);
                p->SetRxRy(atan2f(dx,dz),0); p->SetMoveForward();
                objects.Process(-1,false,false);
            }
            p->GetPosData(&x,&y,&z,NULL);
            std::printf("WALK %s: reached leg %zu/%zu at (%.1f,%.1f,%.1f)\n",name,leg,legs.size(),x,y,z);
            assert(leg==legs.size() && fabsf(y-wantY)<3.0f);
        };
        walk(150,1,-15,{{{175,-15}},{{196,-15}},{{204,-30}}},0,"LAB EAST DOOR");
        walk(26,1,70,{{{26,-6}},{{26,-30}},{{26,-50}},{{100,-50}}},28,"LAB CATWALK RAMP");
        walk(370,1,-22,{{{392,-22}},{{430,-22}},{{460,-22}},{{470,-50}}},44,"SWITCHBACK TO BRIDGE");
        std::puts("ESCAPE ROUTE CLIMBABLE OK");
        objects.Cleanup();
    }
    return 0;
}
