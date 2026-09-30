#include "multiplayer_lan.h"
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <sys/wait.h>
#include <unistd.h>
using namespace usm::mp;
static void check(bool value,const char *message){if(!value){std::cerr<<"FAIL "<<message<<" | "<<lan_error()<<"\n";std::exit(1);}}
static Input scripted(std::uint32_t tick,int player){
    std::uint16_t buttons=0;
    if(tick%400<120)buttons|=player?Button::left:Button::right;
    if(tick%31==0)buttons|=Button::light;
    if(tick%89==0)buttons|=Button::heavy;
    if(tick%173==0)buttons|=Button::jump;
    if(tick%257==0)buttons|=Button::special;
    if((tick+player*17)%101<11)buttons|=Button::guard;
    return {buttons};
}
static int peer(bool host,std::uint16_t port,int scenario){
    if(!host)std::this_thread::sleep_for(std::chrono::milliseconds(40));
    Settings settings;settings.arena=Arena::football;settings.wins_required=5;
    settings.character[0]=Character::blacksuit;settings.character[1]=Character::carnage;
    // Deliberately different join settings: only the host selects the rules.
    Settings supplied=host?settings:Settings{};
    check(lan_begin(host,"127.0.0.1",port,supplied),"begin");
    Match match,reference;bool initialized=false;int exchanged=0;
    auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(12);
    while(std::chrono::steady_clock::now()<deadline){
        lan_pump();
        if(lan_state()==LanState::assets&&!initialized){
            const auto &actual=lan_settings();check(actual.arena==settings.arena&&actual.wins_required==5&&actual.character[1]==Character::carnage,"host settings synchronization");
            match.reset(actual);reference.reset(actual);initialized=true;
            // Unequal load times exercise READY before/after local asset loading.
            if(!host)std::this_thread::sleep_for(std::chrono::milliseconds(25));
            lan_ready();
        }
        if(scenario==2&&host&&exchanged==150){lan_close();return 0;}
        if(lan_state()==LanState::error){
            if(scenario==1){
                // Whichever peer detects the bad hash first closes TCP. The other
                // may observe EOF before consuming that input. Require at least
                // one explicit hash rejection across the pair, not two identical
                // error strings (which would depend on process scheduling).
                if(lan_error().find("desynchronized")!=std::string::npos)return 10;
                check(lan_error().find("closed")!=std::string::npos||lan_error().find("disconnected")!=std::string::npos,"peer terminated after hash mismatch");
                return 11;
            }
            if(scenario==2&&!host){check(lan_error().find("closed")!=std::string::npos||lan_error().find("disconnected")!=std::string::npos,"EOF rejection");return 0;}
            check(false,"unexpected network error");
        }
        if(lan_state()==LanState::playing){
            auto checksum=state_hash(match);
            if(scenario==1&&host&&exchanged==120)checksum^=0x1u;
            std::array<Input,2> inputs{};
            if(lan_exchange(match.tick,scripted(match.tick,host?0:1),checksum,inputs)){
                check(inputs[0].held==scripted(match.tick,0).held&&inputs[1].held==scripted(match.tick,1).held,"authoritative player input mapping");
                reference.step({scripted(reference.tick,0),scripted(reference.tick,1)});match.step(inputs);
                check(state_hash(match)==state_hash(reference),"network state agrees with independent offline input stream");
                if(++exchanged==2000){
                    check(scenario==0,"negative test unexpectedly completed");
                    // Leave TCP alive while the other process consumes the last frame.
                    std::this_thread::sleep_for(std::chrono::milliseconds(120));
                    std::cout<<(host?"HOST":"JOIN")<<" PASS 2000 synchronized ticks, checksum "<<state_hash(match)<<"\n";
                    lan_close();return 0;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::microseconds(150));
    }
    check(false,"test timeout / stalled handshake");return 1;
}
int main(){
    std::signal(SIGPIPE,SIG_IGN);
    check(!lan_begin(false,"localhost",7777,{}),"non-literal address rejected");
    check(lan_state()==LanState::error,"invalid address status");lan_close();
    check(!lan_begin(true,"",0,{}),"zero port rejected");lan_close();
    const auto base=static_cast<std::uint16_t>(22000+(getpid()%15000));
    for(int scenario=0;scenario<3;++scenario){
        const pid_t child=fork();check(child>=0,"fork");
        if(child==0){const int result=peer(false,base+scenario,scenario);std::cout.flush();std::cerr.flush();_exit(result);}
        const int result=peer(true,base+scenario,scenario);int status=0;waitpid(child,&status,0);
        check(WIFEXITED(status),"peer process exited normally");
        const int child_result=WEXITSTATUS(status);
        if(scenario==1){
            check((result==10||result==11)&&(child_result==10||child_result==11)
                &&(result==10||child_result==10),"a bad state hash is rejected and both peers terminate");
        }else check(result==0&&child_result==0,"both peer processes passed");
        std::cout<<"PASS LAN scenario "<<scenario<<"\n";std::cout.flush();
    }
    std::cout<<"PASS: real loopback TCP handshake, asymmetric ready, lockstep, checksum rejection, disconnect, invalid configuration.\n";
}
