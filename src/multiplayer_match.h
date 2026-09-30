#pragma once
// Isolated, deterministic versus rules. No game pointers, wall clock, or RNG.
// Positions are millimetres; velocities are millimetres per 60 Hz simulation tick.
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>

namespace usm::mp {
constexpr int tick_rate = 60;
constexpr int max_health = 1000;
constexpr int max_meter = 1000;
enum class Character : std::uint8_t { spiderman, venom, blacksuit, carnage, count };
enum class Arena : std::uint8_t { warehouse, subway, bridge, football, count };
enum class Phase : std::uint8_t { countdown, fight, round_end, match_end };
enum class Action : std::uint8_t { idle, run, jump, light, heavy, special, guard, dodge, hurt, knockout };
enum Button : std::uint16_t {
    left=1, right=2, forward=4, backward=8, jump=16, light=32,
    heavy=64, special=128, guard=256, dodge=512
};
constexpr std::uint16_t valid_buttons = 1023;
struct Input { std::uint16_t held=0; };
struct Settings {
    Character character[2]{Character::spiderman, Character::venom};
    Arena arena=Arena::warehouse;
    int wins_required=2;
    int time_limit_seconds=0; // Video has no visible match clock. 0 = no limit.
    bool hazards=false;       // Optional PC adaptation, not inferred DS rules.
    void sanitize() {
        for (auto &c:character) if (c>=Character::count) c=Character::spiderman;
        if(arena>=Arena::count) arena=Arena::warehouse;
        wins_required=std::clamp(wins_required,1,5);
        time_limit_seconds=std::clamp(time_limit_seconds,0,600);
    }
};
struct Platform { int x0,x1,z0,z1,top; };
struct Stage {
    int half_width=14000, half_depth=3500;
    std::array<Platform,4> platforms{};
    unsigned count=0;
};
inline Stage stage_for(Arena a) {
    Stage s;
    switch(a) {
    case Arena::warehouse:
        s.platforms[0]={-10800,-7200,-1600,2200,800};
        s.platforms[1]={7200,10800,-1600,2200,800}; s.count=2; break;
    case Arena::subway:
        s.half_width=16000; s.half_depth=3500;
        s.platforms[0]={-15400,-12400,-3000,600,500}; s.count=1; break;
    case Arena::bridge: s.half_width=18000; s.half_depth=3000; break;
    case Arena::football: s.half_width=18000; s.half_depth=5000; break;
    default: break;
    }
    return s;
}
inline const char* character_name(Character c) {
    constexpr const char* names[]{"SPIDER-MAN","VENOM","BLACK SUIT","CARNAGE"};
    return c<Character::count ? names[static_cast<unsigned>(c)] : "UNKNOWN";
}
inline const char* arena_name(Arena a) {
    constexpr const char* names[]{"WAREHOUSE","SUBWAY","QUEENSBORO BRIDGE","FOOTBALL FIELD"};
    return a<Arena::count ? names[static_cast<unsigned>(a)] : "UNKNOWN";
}
inline const char* action_name(Action a) {
    constexpr const char* names[]{"IDLE","MOVE","JUMP","LIGHT","HEAVY","SPECIAL","BLOCK","DODGE","HIT","K.O."};
    auto i=static_cast<unsigned>(a); return i<10 ? names[i] : "?";
}
struct Fighter {
    Character character=Character::spiderman;
    int x=0,y=0,z=0,vy=0;
    int facing=1;
    int health=max_health,meter=0,guard_meter=max_meter;
    Action action=Action::idle;
    int age=0,stun=0,invulnerability=0;
    bool hit_used=false,grounded=true;
    std::uint16_t previous=0;
    std::array<Action,3> cards{Action::idle,Action::idle,Action::idle};
    int card_count=0,combo_hits=0,combo_timeout=0;
};
struct HitEvent {
    int attacker=-1,defender=-1,damage=0; bool blocked=false;
    int stun=0,guard_damage=0,push=0; // Captured before either simultaneous hit mutates state.
};
struct Attack { int startup,active,recovery,reach,damage,stun; };
inline Attack attack_for(Action a, Character c) {
    Attack r{6,4,12,1900,80,17};
    if(a==Action::heavy) r={11,5,22,2500,145,27};
    if(a==Action::special) r={16,6,29,6200,205,33};
    if(c==Character::venom) { r.reach+=450; r.damage=r.damage*115/100; r.recovery+=3; }
    if(c==Character::carnage) { r.reach+=650; r.damage=r.damage*105/100; }
    if(c==Character::blacksuit) r.damage=r.damage*108/100;
    return r;
}
inline bool is_attack(Action a) {return a==Action::light||a==Action::heavy||a==Action::special;}
class Match {
public:
    Settings settings{};
    std::array<Fighter,2> fighters{};
    std::array<int,2> wins{};
    Phase phase=Phase::countdown;
    int phase_ticks=120, round=1, winner=-1, round_winner=-1;
    std::uint32_t tick=0;
    int fight_ticks=0,hitstop=0;
    std::array<HitEvent,2> hits{};
    int hit_count=0;
    explicit Match(Settings s={}) { reset(s); }
    void reset(Settings s) {
        s.sanitize(); settings=s; wins={0,0}; round=1; winner=-1;
        tick=0; reset_round();
    }
    void reset_round() {
        fighters={Fighter{},Fighter{}};
        for(int i=0;i<2;++i) {
            fighters[i].character=settings.character[i];
            fighters[i].x=i ? 3200 : -3200; fighters[i].facing=i ? -1 : 1;
        }
        phase=Phase::countdown; phase_ticks=120; round_winner=-1;
        fight_ticks=0; hitstop=0; hit_count=0;
    }
    void step(std::array<Input,2> input) {
        ++tick; hit_count=0;
        for(auto &i:input) i.held &= valid_buttons;
        if(phase==Phase::match_end) return;
        if(phase==Phase::countdown) {
            for(int i=0;i<2;++i) fighters[i].previous=input[i].held;
            if(--phase_ticks<=0) {phase=Phase::fight;phase_ticks=0;}
            return;
        }
        if(phase==Phase::round_end) {
            if(--phase_ticks<=0) {
                if(wins[0]>=settings.wins_required||wins[1]>=settings.wins_required) {
                    phase=Phase::match_end;winner=wins[0]>wins[1] ? 0:1;
                } else { ++round;reset_round(); }
            }
            return;
        }
        if(hitstop>0) {--hitstop;return;} // Held inputs are not consumed during hit-stop.
        ++fight_ticks;
        auto stage=stage_for(settings.arena);
        // Update both fighters before evaluating either strike: no P1-first advantage.
        for(int i=0;i<2;++i) update_fighter(i,input[i],stage);
        separate_bodies(stage);
        const auto snapshot=fighters;
        std::array<HitEvent,2> pending{}; int count=0;
        for(int i=0;i<2;++i) {
            const auto &a=snapshot[i];const auto &b=snapshot[1-i];
            if(!is_attack(a.action)||a.hit_used||b.invulnerability>0||b.health<=0) continue;
            const auto spec=attack_for(a.action,a.character);
            const int dx=(b.x-a.x)*a.facing;
            if(a.age<spec.startup||a.age>=spec.startup+spec.active||dx<0||dx>spec.reach
               ||std::abs(b.z-a.z)>1150||std::abs(b.y-a.y)>1850) continue;
            bool blocked=b.action==Action::guard&&b.grounded&&b.facing==-a.facing;
            int damage=spec.damage;
            if(a.combo_hits>=2) damage=damage*125/100;
            pending[count++]={i,1-i,blocked ? std::max(1,damage/10):damage,blocked,
                              spec.stun,spec.damage*3,a.facing*(a.action==Action::heavy ? 650:350)};
        }
        for(int n=0;n<count;++n) apply_hit(pending[n]);
        if(settings.hazards&&settings.arena==Arena::subway) {
            const int cycle=fight_ticks%900;
            if(cycle>=780&&cycle<840) for(auto &f:fighters) {
                if(f.z>2000&&f.y<1700&&f.invulnerability==0) {
                    f.health=std::max(0,f.health-150); f.stun=30;
                    f.invulnerability=75; f.action=Action::hurt; f.age=0;
                }
            }
        }
        if(fighters[0].health<=0||fighters[1].health<=0) {
            finish_round(fighters[0].health==fighters[1].health ? -1 : fighters[0].health>0 ? 0:1);
        } else if(settings.time_limit_seconds>0&&fight_ticks>=settings.time_limit_seconds*tick_rate) {
            finish_round(fighters[0].health==fighters[1].health ? -1 : fighters[0].health>fighters[1].health ? 0:1);
        }
    }
    int remaining_seconds() const {
        return settings.time_limit_seconds==0 ? -1 : std::max(0,settings.time_limit_seconds-(fight_ticks/tick_rate));
    }
private:
    void finish_round(int who) {
        phase=Phase::round_end;phase_ticks=150;round_winner=who;
        if(who>=0) ++wins[who]; // A simultaneous KO/timeout tie replays without a win.
        for(auto &f:fighters) if(f.health<=0) {f.action=Action::knockout;f.age=0;}
    }
    static void start_action(Fighter &f,Action a) {f.action=a;f.age=0;f.hit_used=false;}
    static void record_card(Fighter &f,Action a) {
        f.combo_timeout=90;
        if(f.card_count<3) f.cards[f.card_count++]=a;
        else {f.cards[0]=f.cards[1];f.cards[1]=f.cards[2];f.cards[2]=a;}
    }
    void update_fighter(int index,Input in,const Stage &s) {
        auto &f=fighters[index];const auto &other=fighters[1-index];
        const auto pressed=static_cast<std::uint16_t>(in.held & ~f.previous);
        f.previous=in.held; ++f.age;
        if(f.invulnerability>0)--f.invulnerability;
        if(f.combo_timeout>0) {if(--f.combo_timeout==0) {f.combo_hits=0;f.card_count=0;}}
        if(f.stun>0) {--f.stun; if(f.stun==0) start_action(f,Action::idle);}
        if(is_attack(f.action)) {
            auto spec=attack_for(f.action,f.character);
            if(f.age>=spec.startup+spec.active+spec.recovery) start_action(f,Action::idle);
        }
        if(f.action==Action::dodge&&f.age>=19) start_action(f,Action::idle);
        const bool busy=f.stun>0||is_attack(f.action)||f.action==Action::dodge;
        if(!busy) {
            if(other.x!=f.x)f.facing=other.x>f.x ? 1:-1;
            if((pressed&Button::jump)&&f.grounded) {f.vy=145;f.grounded=false;start_action(f,Action::jump);}
            Action attack=Action::idle;
            if((pressed&Button::special)&&f.meter>=350) {attack=Action::special;f.meter-=350;}
            else if(pressed&Button::heavy) attack=Action::heavy;
            else if(pressed&Button::light) attack=Action::light;
            if(attack!=Action::idle) {start_action(f,attack);record_card(f,attack);}
            else if((pressed&Button::dodge)&&f.meter>=100) {
                f.meter-=100;start_action(f,Action::dodge);f.invulnerability=12;
            } else if((in.held&Button::guard)&&f.grounded&&f.guard_meter>0) {
                if(f.action!=Action::guard)start_action(f,Action::guard);
            } else {
                int speed=f.character==Character::venom ? 78:105;
                if(f.character==Character::carnage)speed=112;
                int dx=((in.held&Button::right)!=0)-((in.held&Button::left)!=0);
                int dz=((in.held&Button::backward)!=0)-((in.held&Button::forward)!=0);
                if(dx&&dz)speed=speed*707/1000;
                move(f,dx*speed,dz*speed,s);
                Action next=f.grounded ? (dx||dz ? Action::run:Action::idle):Action::jump;
                if(f.action!=next)start_action(f,next);
            }
        }
        if(f.action==Action::dodge) {
            int sign=(in.held&Button::left) ? -1 : (in.held&Button::right) ? 1 : -f.facing;
            move(f,sign*180,0,s);
        }
        if(f.action!=Action::guard)f.guard_meter=std::min(max_meter,f.guard_meter+4);
        const int old_y=f.y;
        if(!f.grounded) {f.vy-=5;f.y+=f.vy;}
        int floor=0;
        for(unsigned i=0;i<s.count;++i) {
            const auto &p=s.platforms[i];
            if(f.x>=p.x0-260&&f.x<=p.x1+260&&f.z>=p.z0-260&&f.z<=p.z1+260
               &&old_y>=p.top&&f.y<=p.top&&f.vy<=0) floor=std::max(floor,p.top);
        }
        if(f.y<=floor&&f.vy<=0) {f.y=floor;f.vy=0;f.grounded=true;}
        else if(f.grounded) { // Walking off a raised platform starts falling.
            bool supported=f.y==0;
            for(unsigned i=0;i<s.count;++i) {const auto &p=s.platforms[i];
                supported|=f.y==p.top&&f.x>=p.x0-260&&f.x<=p.x1+260&&f.z>=p.z0-260&&f.z<=p.z1+260;
            }
            if(!supported)f.grounded=false;
        }
    }
    static void move(Fighter &f,int dx,int dz,const Stage &s) {
        int nx=std::clamp(f.x+dx,-s.half_width+400,s.half_width-400);
        int nz=std::clamp(f.z+dz,-s.half_depth+400,s.half_depth-400);
        for(unsigned i=0;i<s.count;++i) {
            const auto &p=s.platforms[i];
            if(f.y>=p.top)continue;
            if(nx>p.x0-350&&nx<p.x1+350&&nz>p.z0-350&&nz<p.z1+350) {
                if(f.x<=p.x0-350||f.x>=p.x1+350)nx=f.x;
                if(f.z<=p.z0-350||f.z>=p.z1+350)nz=f.z;
                // A landing overlap is allowed; an entry from the side is not.
            }
        }
        f.x=nx;f.z=nz;
    }
    void separate_bodies(const Stage &s) {
        auto &a=fighters[0];auto &b=fighters[1];
        if(std::abs(a.y-b.y)>1600||std::abs(a.z-b.z)>750)return;
        int diff=b.x-a.x;
        if(std::abs(diff)>=850)return;
        int sign=diff>=0 ? 1:-1;int half=(850-std::abs(diff)+1)/2;
        move(a,-sign*half,0,s);move(b,sign*half,0,s);
    }
    void apply_hit(const HitEvent &hit) {
        auto &a=fighters[hit.attacker];auto &b=fighters[hit.defender];
        a.hit_used=true;
        b.health=std::max(0,b.health-hit.damage);
        a.meter=std::min(max_meter,a.meter+(hit.blocked ? 40:110));
        b.meter=std::min(max_meter,b.meter+45);
        if(hit.blocked) {
            b.guard_meter=std::max(0,b.guard_meter-hit.guard_damage);
            if(b.guard_meter==0) {b.stun=45;start_action(b,Action::hurt);}
        } else {
            ++a.combo_hits;a.combo_timeout=90;
            b.stun=hit.stun;start_action(b,Action::hurt);
            move(b,hit.push,0,stage_for(settings.arena));
        }
        hitstop=4;
        if(hit_count<2)hits[hit_count++]=hit;
    }
};
// Explicit field checksum: never hash padding/pointers or host endianness.
inline std::uint32_t state_hash(const Match &m) {
    std::uint32_t h=2166136261u;
    auto add=[&](std::uint32_t n){for(int i=0;i<4;++i){h^=(n>>(i*8))&255u;h*=16777619u;}};
    add(static_cast<unsigned>(m.settings.character[0]));add(static_cast<unsigned>(m.settings.character[1]));
    add(static_cast<unsigned>(m.settings.arena));add(m.settings.wins_required);
    add(m.settings.time_limit_seconds);add(m.settings.hazards);
    add(static_cast<unsigned>(m.phase));add(m.tick);add(m.phase_ticks);add(m.round);
    add(m.winner);add(m.round_winner);add(m.fight_ticks);add(m.hitstop);
    add(m.wins[0]);add(m.wins[1]);
    for(const auto &f:m.fighters) {
        add(static_cast<unsigned>(f.character));add(f.x);add(f.y);add(f.z);add(f.vy);
        add(f.facing);add(f.health);add(f.meter);add(f.guard_meter);add(static_cast<unsigned>(f.action));
        add(f.age);add(f.stun);add(f.invulnerability);add(f.hit_used);add(f.grounded);add(f.previous);
        add(f.card_count);add(f.combo_hits);add(f.combo_timeout);
        for(auto a:f.cards)add(static_cast<unsigned>(a));
    }
    return h;
}
} // namespace usm::mp
