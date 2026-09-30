#include "multiplayer_online_ui.h"
#include <cstdlib>
#include <iostream>
using namespace usm::online;
namespace pad=usm::mp::pad;
unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<__LINE__<<": " #x<<'\n';std::abort();}}while(0)
int main(){
    std::uint16_t port=123;for(auto s:{"0","65536","-1","7777x","1.2","","000000","99999"})CHECK(!parse_port(s,port));
    CHECK(port==123);CHECK(parse_port("7777",port)&&port==7777);CHECK(parse_port("65535",port)&&port==65535);
    for(auto s:{"127.0.0.1","26.1.2.3","192.168.1.20","255.255.255.255"})CHECK(valid_ipv4_text(s));
    for(auto s:{"1.2.3","1.2.3.4.","1.2.3.4.5","1..2.3","256.1.2.3","01.2.3.4","a.2.3.4",""})CHECK(!valid_ipv4_text(s));
    Editor e;e.begin("Piero",24,false);e.move(-2);e.insert('_');CHECK(e.value=="Pie_ro"&&e.cursor==4);e.backspace();CHECK(e.value=="Piero"&&e.cursor==3);
    e.erase();CHECK(e.value=="Pieo");e.move(100);CHECK(e.cursor==4);e.cycle(1);CHECK(e.value=="PieoA");e.cycle(-1);CHECK(e.value=="Pieo ");
    e.begin("",5,true);e.paste("77abc77#!9");CHECK(e.value=="77779");e.insert('1');CHECK(e.value=="77779");
    e.begin("",24,false);for(int i=0;i<10000;++i){e.cycle(i%2?1:-1);e.move(i%3-1);e.insert(i%2?'_':'a');if(i%3==0)e.backspace();CHECK(e.cursor<=e.value.size()&&e.value.size()<=24);}
    auto p=pad::map_xinput(0x1000|0x0010,20000,-32768,32767,-20000,130,250);auto n=native_pad(p,9000);
    CHECK(n.jump==255&&(n.flags&0x10));CHECK(n.move_x>0&&n.move_x<32767&&n.move_y==-32767);CHECK(n.camera_x==32767&&n.camera_y<0);CHECK(n.left_trigger==130&&n.right_trigger==250);
    for(int x=-32767;x<=32767;x+=41){auto v=analog(x,9000);CHECK(v>=-32767&&v<=32767);CHECK((std::abs(x)<=9000)==(v==0));CHECK(analog(-x,9000)==-v);}
    pad::DirectSample raw;raw.axis[2]=30000;raw.axis[5]=-32767;raw.button[1]=0x80;raw.button[6]=0x80;
    auto ds=pad::map_direct(raw,pad::default_mapping(true));n=native_pad(ds,9000);CHECK(n.jump==255&&n.left_trigger==255&&n.camera_x>0&&n.camera_y==32767);
    ds.connected=false;n=native_pad(ds,9000);CHECK(!n.jump&&!n.camera_x&&!n.flags);
    std::cout<<checks<<" UI/editor/native-pad checks passed\n";
}
