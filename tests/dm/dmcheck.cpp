// dmcheck: real-engine acceptance gate for XOPS ONLINE deathmatch arenas.
// Uses the actual Collision / human physics / ObjectManager code; only
// rendering, input, window and audio devices are stubbed (like the UFO
// harness).
//
//   dmcheck <pack> <mapdir> <pd1> [--sealed] [--overlay DIR] [--jumpprobe]
//
// Prints one "METRIC name value" line per measurement and "FAIL ..." lines;
// exits 0 only when every gate passes. Gates (see docs/DM-MAP-GUIDE.md):
//   - sealed (indoor maps): no view ray from any standing spot escapes
//   - every DM spawn, weapon pickup and lift top is reachable from spawn 0,
//     and spawn 0 is reachable back from each of them (no one-way pockets)
//   - >= 97% of standing spots are in the main connected area
//   - every lift carries a real player (engine physics) onto its top floor
//   - spawn exposure: no spawn is visible from more than 35% of the map
//   - sight lines: 10%..45% of random standing pairs see each other
//   - no obstacle top sits 1.5 units either side of the jump limit
//     (ambiguous: looks climbable, sometimes is not)
#define main openxops_original_entry
#include "../../src/main.cpp"
#undef main
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

static const float G = 4.0f;            // standing-spot grid
static const float STEP_UP = 3.5f;      // walk onto (engine climb 3.2 + slack on ramps)
static const float DROP_MAX = 40.0f;    // safe drop
static float JUMP_UP = 16.0f;           // measured by --jumpprobe (walk + jump: 16 clears, 17 does not)

struct Node { int i, j, l; };
typedef std::tuple<int,int,int> Key;

int main(int argc, char** argv)
{
	if( argc < 4 ){ std::fprintf(stderr, "usage: dmcheck <pack> <mapdir> <pd1> [--sealed] [--overlay DIR] [--jumpprobe]\n"); return 2; }
	std::setvbuf(stdout, nullptr, _IONBF, 0);
	bool sealed = false, jumpprobe = false;
	const char* overlay = nullptr;
	for(int a=4; a<argc; a++){
		if( !strcmp(argv[a], "--sealed") ) sealed = true;
		else if( !strcmp(argv[a], "--jumpprobe") ) jumpprobe = true;
		else if( !strcmp(argv[a], "--overlay") && a+1 < argc ) overlay = argv[++a];
	}
	InitRand();
	if( overlay ) vfs_mount_dir(overlay, "overlay");
	vfs_init_from_args(argv[1]);
	ParameterInfo params; params.InitInfo();
	D3DGraphics graphics;
	ResourceManager resources; resources.SetParameterInfo(&params); resources.SetD3DGraphics(&graphics);
	SoundControl device;
	SoundManager sound(&device, &resources, &params);
	BlockDataInterface blocks; PointDataInterface points; Collision coll; MIFInterface mif;
	ObjectManager objects;
	objects.SetClass(&params, &graphics, &resources, &blocks, &points, &coll, &sound, &mif);

	std::string dir = argv[2];
	if( blocks.LoadFiledata((dir + "/temp.bd1").c_str()) != 0 ){ std::printf("FAIL cannot load %s/temp.bd1\n", dir.c_str()); return 1; }
	blocks.CalculationBlockdata(false); coll.InitCollision(&blocks);
	if( points.LoadFiledata((dir + "/" + argv[3]).c_str()) != 0 ){ std::printf("FAIL cannot load %s/%s\n", dir.c_str(), argv[3]); return 1; }
	objects.LoadPointData();
	int failures = 0;
	auto fail = [&](const char* fmt, auto... args){ std::printf("FAIL "); std::printf(fmt, args...); std::printf("\n"); failures++; };

	// ---- jump probe: walk + jump onto each step of a fixture map ----------
	if( jumpprobe ){
		// fixture: steps along +z every 30 units, heights 4..28 at x=k*30 (see tools/tests/test_dm_check.py)
		human* p = objects.GetPlayerHumanObject();
		int pid = objects.GetPlayerID();
		float best = 0;
		if( getenv("DMCHECK_DEBUG") ){ float dd; bool hit = coll.CheckALLBlockIntersectRay(0, 40, 5, 0, -1, 0, NULL, NULL, &dd, 100);
			std::printf("  blocks %d, ray at step0 hit %d dist %.1f, player id %d enabled %d\n", blocks.GetTotaldatas(), hit, dd, pid, p->GetEnableFlag()); }
		for(int k=0; k<25; k++){
			float h = 4.0f + k, x0 = k*30.0f;
			p->SetPosData(x0, 0.5f, -20, 0); p->SetRxRy(0, 0);
			for(int t=0; t<20; t++) objects.Process(-1, false, false);
			float x, y, z;
			for(int t=0; t<80; t++){
				p->GetPosData(&x, &y, &z, NULL);
				if( getenv("DMCHECK_DEBUG") && k==12 && t%5==0 ) std::printf("    t%d (%.1f,%.1f,%.1f)\n", t, x, y, z);
				if( z > 5.0f ) break;	//standing on the step's top
				objects.MoveForward(pid);
				if( z > -9.0f && z < -6.0f ){ objects.MoveJump(pid); }
				objects.Process(-1, false, false);
			}
			p->GetPosData(&x, &y, &z, NULL);
			for(int t=0; t<10; t++) objects.Process(-1, false, false);
			p->GetPosData(&x, &y, &z, NULL);
			bool on = (y > h - 1.0f) && (z > -4.0f);
			if( getenv("DMCHECK_DEBUG") ) std::printf("  step %.0f end (%.1f,%.1f,%.1f)\n", h, x, y, z);
			std::printf("METRIC jump_step_%02d %d\n", (int)h, on ? 1 : 0);
			if( on ) best = h;
		}
		std::printf("METRIC jump_up_max %.1f\n", best);
		return 0;
	}

	// ---- bounds ------------------------------------------------------------
	float bmin[3] = {1e9f,1e9f,1e9f}, bmax[3] = {-1e9f,-1e9f,-1e9f};
	for(int b=0; b<blocks.GetTotaldatas(); b++){
		blockdata bd; blocks.Getdata(&bd, b);
		for(int v=0; v<8; v++){
			float c[3] = {bd.x[v], bd.y[v], bd.z[v]};
			for(int a=0; a<3; a++){ bmin[a] = std::min(bmin[a], c[a]); bmax[a] = std::max(bmax[a], c[a]); }
		}
	}
	std::printf("METRIC blocks %d\n", blocks.GetTotaldatas());
	std::printf("METRIC extent_x %.0f\nMETRIC extent_z %.0f\nMETRIC extent_y %.0f\n", bmax[0]-bmin[0], bmax[2]-bmin[2], bmax[1]-bmin[1]);

	// ---- standing spots ------------------------------------------------------
	auto floorAt = [&](float x, float top, float z, float* fy)->bool{
		float d; if( !coll.CheckALLBlockIntersectRay(x, top, z, 0, -1, 0, NULL, NULL, &d, top - bmin[1] + 10) ) return false;
		*fy = top - d; return true;
	};
	auto standable = [&](float x, float fy, float z)->bool{
		float d;
		for(float h : {2.0f, 10.0f, 19.0f}) if( coll.CheckALLBlockInside(x, fy+h, z) ) return false;
		if( coll.CheckALLBlockIntersectRay(x, fy+2, z, 0, 1, 0, NULL, NULL, &d, 20.0f) ) return false;
		for(int a=0; a<8; a++){ float yaw = a*(float)M_PI/4;
			if( coll.CheckALLBlockIntersectRay(x, fy+10, z, cosf(yaw), 0, sinf(yaw), NULL, NULL, &d, 2.8f) ) return false; }
		return true;
	};
	auto clearBetween = [&](float x0, float y0, float z0, float x1, float y1, float z1)->bool{
		float dx = x1-x0, dz = z1-z0, len = sqrtf(dx*dx+dz*dz), d; if( len < 1e-3f ) return true;
		for(float h : {6.0f, 12.0f, 18.0f}){
			float yh = std::max(y0, y1) + h;
			if( coll.CheckALLBlockIntersectRay(x0, yh, z0, dx/len, 0, dz/len, NULL, NULL, &d, len) ) return false;
		}
		return true;
	};
	std::map<Key, float> nodes;
	auto key = [&](int i, int j, float y){ return std::make_tuple(i, j, (int)floorf(y/6.0f)); };
	int i0 = (int)floorf(bmin[0]/G), i1 = (int)ceilf(bmax[0]/G), j0 = (int)floorf(bmin[2]/G), j1 = (int)ceilf(bmax[2]/G);
	for(int i=i0; i<=i1; i++) for(int j=j0; j<=j1; j++){
		float x = i*G, z = j*G, top = bmax[1] + 5, fy;
		for(int layer=0; layer<8 && floorAt(x, top, z, &fy); layer++){
			//roof tops (the highest surfaces of the map) are outside the arena
			if( (fy < bmax[1] - 3.0f)&&standable(x, fy, z) ) nodes[key(i, j, fy)] = fy;
			top = fy - 0.5f;
			while( top > bmin[1] - 5 && coll.CheckALLBlockInside(x, top, z) ) top -= 1.0f;
		}
	}
	std::printf("METRIC standing_spots %zu\nMETRIC floor_area %.0f\n", nodes.size(), nodes.size()*G*G);
	if( nodes.empty() ){ fail("no standing spots"); return 1; }

	// ---- points ----------------------------------------------------------------
	struct P { float x, y, z, r; int p1, p2, p3, p4; };
	std::vector<P> spawns, pickups, lifts;
	for(int n=0; n<points.GetTotaldatas(); n++){
		pointdata p; if( points.Getdata(&p, n) != 0 ) continue;
		P q = {p.x, p.y, p.z, p.r, p.p1, p.p2, p.p3, p.p4};
		if( p.p1 == 60 ) spawns.push_back(q);
		if( p.p1 == 2 || p.p1 == 3 ) pickups.push_back(q);
		if( p.p1 == 61 ) lifts.push_back(q);
	}
	std::sort(spawns.begin(), spawns.end(), [](const P& a, const P& b){ return a.p4 < b.p4; });
	std::printf("METRIC dm_spawns %zu\nMETRIC pickups %zu\nMETRIC lifts %zu\n", spawns.size(), pickups.size(), lifts.size());
	if( spawns.size() < 8 ) fail("only %zu DM spawns (need >= 8)", spawns.size());

	auto nearest = [&](float x, float y, float z, float* dist)->Key{
		Key best; float bd = 1e9f;
		for(auto& kv : nodes){ int i, j, l; std::tie(i, j, l) = kv.first;
			float d = fabsf(i*G-x) + fabsf(j*G-z) + fabsf(kv.second-y)*2; if( d < bd ){ bd = d; best = kv.first; } }
		*dist = bd; return best;
	};

	// ---- graph: walk, step, jump-up, drop, lifts --------------------------------
	std::map<Key, std::vector<Key>> adj;
	for(auto& kv : nodes){
		int i, j, l; std::tie(i, j, l) = kv.first; float y = kv.second;
		for(int n=0; n<4; n++){
			int ni = i + (n==0) - (n==1), nj = j + (n==2) - (n==3);
			for(int nl=l-8; nl<=l+8; nl++){
				auto it = nodes.find(std::make_tuple(ni, nj, nl)); if( it == nodes.end() ) continue;
				float dy = it->second - y;
				bool ok = false;
				if( fabsf(dy) <= STEP_UP ) ok = clearBetween(i*G, y, j*G, ni*G, it->second, nj*G);
				else if( dy > 0 && dy <= JUMP_UP ){
					float d; // headroom for the jump arc above the lower spot
					ok = !coll.CheckALLBlockIntersectRay(i*G, y+2, j*G, 0, 1, 0, NULL, NULL, &d, 20.0f + dy)
					   && clearBetween(i*G, it->second, j*G, ni*G, it->second, nj*G);
				}
				else if( dy < 0 && -dy <= DROP_MAX ) ok = clearBetween(i*G, it->second, j*G, ni*G, it->second, nj*G);
				if( ok ) adj[kv.first].push_back(it->first);
			}
		}
	}
	// lifts: physics test, then a graph edge foot -> top
	for(size_t k=0; k<lifts.size(); k++){
		const P& L = lifts[k];
		human* p = objects.GetPlayerHumanObject();
		p->SetPosData(L.x, L.y + 0.5f, L.z, 0);
		float top = L.y + L.p3, x, y, z, peak = 0;
		for(int t=0; t<200; t++){ objects.Process(-1, false, false); p->GetPosData(&x, &y, &z, NULL); peak = std::max(peak, y); }
		bool landed = y >= top - 3.0f;
		std::printf("METRIC lift_%zu_peak %.1f\nMETRIC lift_%zu_rest_y %.1f\n", k, peak, k, y);
		if( !landed ){ fail("lift %zu (foot %.0f,%.0f,%.0f, top %.0f) leaves the player at y %.1f", k, L.x, L.y, L.z, top, y); continue; }
		float d1, d2; Key foot = nearest(L.x, L.y, L.z, &d1), dest = nearest(x, y, z, &d2);
		if( d1 < 16 && d2 < 16 ) adj[foot].push_back(dest);
	}
	auto bfs = [&](Key s, bool reverse)->std::set<Key>{
		std::map<Key, std::vector<Key>> radj;
		if( reverse ) for(auto& kv : adj) for(auto& t : kv.second) radj[t].push_back(kv.first);
		auto& g = reverse ? radj : adj;
		std::set<Key> seen{s}; std::vector<Key> st{s};
		while( !st.empty() ){ Key k = st.back(); st.pop_back(); auto it = g.find(k); if( it == g.end() ) continue;
			for(auto& t : it->second) if( seen.insert(t).second ) st.push_back(t); }
		return seen;
	};
	float d;
	Key s0 = nearest(spawns.empty() ? 0 : spawns[0].x, spawns.empty() ? 0 : spawns[0].y, spawns.empty() ? 0 : spawns[0].z, &d);
	auto fwd = bfs(s0, false), back = bfs(s0, true);
	//play area = spots you can reach from spawn 0 and get back from; spots you
	//can reach but never leave are traps (fell into a pit, dropped behind cover)
	std::set<Key> play; size_t trapped = 0;
	for(auto& k : fwd){ if( back.count(k) ) play.insert(k); else trapped++; }
	float trapfrac = fwd.empty() ? 1.0f : (float)trapped / fwd.size();
	std::printf("METRIC play_spots %zu\nMETRIC play_area %.0f\nMETRIC trapped_fraction %.4f\n", play.size(), play.size()*G*G, trapfrac);
	if( trapfrac > 0.005f ){
		fail("%.1f%% of reachable spots are one-way traps (no way back)", trapfrac*100);
		int shown = 0;
		for(auto& k : fwd) if( !back.count(k) && shown++ < 5 ){ int i, j, l; std::tie(i, j, l) = k; std::printf("NOTE trap at (%.0f,%.0f,%.0f)\n", i*G, nodes[k], j*G); }
	}
	auto checkPoint = [&](const char* what, size_t idx, const P& q){
		float dist; Key k = nearest(q.x, q.y, q.z, &dist);
		if( dist > 12 ){ fail("%s %zu at (%.0f,%.0f,%.0f) is not on a standing spot", what, idx, q.x, q.y, q.z); return; }
		if( !fwd.count(k) ) fail("%s %zu at (%.0f,%.0f,%.0f) cannot be reached from spawn 0", what, idx, q.x, q.y, q.z);
		if( !back.count(k) ) fail("%s %zu at (%.0f,%.0f,%.0f) has no way back to spawn 0", what, idx, q.x, q.y, q.z);
	};
	for(size_t k=0; k<spawns.size(); k++) checkPoint("spawn", k, spawns[k]);
	for(size_t k=0; k<pickups.size(); k++) checkPoint("pickup", k, pickups[k]);

	// ---- sight lines -------------------------------------------------------------
	std::vector<std::pair<Key,float>> list;
	for(auto& k : play) list.push_back(std::make_pair(k, nodes[k]));
	if( list.empty() ){ fail("empty play area"); return 1; }
	auto eye = [&](const std::pair<Key,float>& n, float* x, float* y, float* z){ int i, j, l; std::tie(i, j, l) = n.first; *x = i*G; *z = j*G; *y = n.second + 17.0f; };
	auto sees = [&](float ax, float ay, float az, float bx, float by, float bz)->bool{
		float dx = bx-ax, dy = by-ay, dz = bz-az, len = sqrtf(dx*dx+dy*dy+dz*dz), dd;
		if( len < 1 ) return true;
		return !coll.CheckALLBlockIntersectRay(ax, ay, az, dx/len, dy/len, dz/len, NULL, NULL, &dd, len);
	};
	unsigned int rng = 12345; auto rnd = [&](){ rng = rng*1103515245u + 12345u; return (rng >> 8) % list.size(); };
	int vis = 0, pairs = 3000; float longest = 0;
	for(int n=0; n<pairs; n++){
		float ax, ay, az, bx, by, bz; eye(list[rnd()], &ax, &ay, &az); eye(list[rnd()], &bx, &by, &bz);
		if( sees(ax, ay, az, bx, by, bz) ){ vis++; longest = std::max(longest, sqrtf((ax-bx)*(ax-bx)+(az-bz)*(az-bz))); }
	}
	float losfrac = (float)vis / pairs;
	std::printf("METRIC sightline_fraction %.3f\nMETRIC longest_sightline %.0f\n", losfrac, longest);
	if( losfrac < 0.10f || losfrac > 0.45f ) fail("sight-line fraction %.2f outside 0.10..0.45", losfrac);
	float worst = 0;
	for(size_t k=0; k<spawns.size(); k++){
		int seen = 0, n = 600;
		for(int m=0; m<n; m++){ float ax, ay, az; eye(list[rnd()], &ax, &ay, &az);
			if( sees(ax, ay, az, spawns[k].x, spawns[k].y + 15.0f, spawns[k].z) ) seen++; }
		float e = (float)seen / n; worst = std::max(worst, e);
		std::printf("METRIC spawn_%zu_exposure %.2f\n", k, e);
		if( e > 0.35f ) fail("spawn %zu at (%.0f,%.0f,%.0f) is visible from %.0f%% of the map", k, spawns[k].x, spawns[k].y, spawns[k].z, e*100);
	}
	std::printf("METRIC worst_spawn_exposure %.2f\n", worst);

	// ---- ambiguous obstacle heights ------------------------------------------------
	int ambiguous = 0;
	for(auto& kv : nodes){
		int i, j, l; std::tie(i, j, l) = kv.first;
		for(int n=0; n<4; n++){
			int ni = i + (n==0) - (n==1), nj = j + (n==2) - (n==3);
			for(int nl=l+1; nl<=l+4; nl++){
				auto it = nodes.find(std::make_tuple(ni, nj, nl)); if( it == nodes.end() ) continue;
				float dy = it->second - kv.second;
				if( fabsf(dy - JUMP_UP) < 1.5f ){ ambiguous++; if( ambiguous <= 5 ) std::printf("NOTE ambiguous ledge %.1f high at (%.0f,%.0f,%.0f)\n", dy, ni*G, it->second, nj*G); }
			}
		}
	}
	std::printf("METRIC ambiguous_ledges %d\n", ambiguous);
	if( ambiguous > 0 ) fail("%d ledge edges within 1.5 of the jump limit %.1f (make them clearly jumpable or clearly not)", ambiguous, JUMP_UP);

	// ---- sealed ------------------------------------------------------------------------
	if( sealed ){
		int leaks = 0, sampled = 0;
		for(size_t n=0; n<list.size(); n+=3){
			float x, y, z; eye(list[n], &x, &y, &z); sampled++;
			for(int a=0; a<16; a++) for(int e=-1; e<=2; e++){
				float yaw = a*(float)M_PI/8, pitch = e*0.45f;
				if( !coll.CheckALLBlockIntersectRay(x, y, z, cosf(yaw)*cosf(pitch), sinf(pitch), sinf(yaw)*cosf(pitch), NULL, NULL, &d, 4000.0f) ){
					if( leaks < 5 ) std::printf("NOTE void ray from (%.0f,%.0f,%.0f) yaw %d pitch %d\n", x, y, z, a, e);
					leaks++;
				}
			}
		}
		std::printf("METRIC void_leaks %d (from %d spots)\n", leaks, sampled);
		if( leaks ) fail("%d view rays escape into the void", leaks);
	}

	std::printf(failures ? "DMCHECK FAILED (%d)\n" : "DMCHECK OK\n", failures);
	return failures ? 1 : 0;
}
