#include "VictronDecoder.h"
#include <mbedtls/aes.h>
#include <string.h>

namespace {
class BitReader {
public:
  BitReader(const uint8_t *p, size_t n) : p_(p), bits_(n * 8), pos_(0) {}
  uint32_t u(uint8_t count) {
    if (count == 0 || count > 32 || pos_ + count > bits_) { ok_ = false; return 0; }
    uint32_t v = 0;
    for (uint8_t i = 0; i < count; ++i) v |= ((p_[(pos_ + i) >> 3] >> ((pos_ + i) & 7)) & 1U) << i;
    pos_ += count; return v;
  }
  int32_t s(uint8_t count) { uint32_t v = u(count); if (!ok_) return 0; uint32_t sign=1UL<<(count-1); return (int32_t)((v^sign)-sign); }
  bool ok() const { return ok_; }
private:
  const uint8_t *p_; size_t bits_, pos_; bool ok_ = true;
};
}

bool VictronDecoder::hexToBytes(const String &hex, uint8_t *bytes, size_t count) {
  if (hex.length() != count * 2) return false;
  for (size_t i=0;i<count;i++) { char q[3]={hex[i*2],hex[i*2+1],0}; char *e=nullptr; long v=strtol(q,&e,16); if(!e||*e||v<0||v>255)return false; bytes[i]=(uint8_t)v; }
  return true;
}
int32_t VictronDecoder::signExtend(uint32_t v,uint8_t bits){uint32_t s=1UL<<(bits-1);return(int32_t)((v^s)-s);}

bool VictronDecoder::decryptPayload(const uint8_t *data,size_t length,const String &hexKey,uint8_t *plain,size_t &plainLength){
  plainLength=0;
  if(!data||length<=10||length>42||data[0]!=0xE1||data[1]!=0x02||data[2]!=0x10)return false;
  uint8_t key[16]; if(!hexToBytes(hexKey,key,16)||data[9]!=key[0])return false;
  plainLength=length-10; if(plainLength>32)return false;
  uint8_t ctr[16]={data[7],data[8],0}, stream[16]={0}; size_t off=0;
  mbedtls_aes_context aes; mbedtls_aes_init(&aes);
  int rc=mbedtls_aes_setkey_enc(&aes,key,128);
  if(rc==0)rc=mbedtls_aes_crypt_ctr(&aes,plainLength,&off,ctr,stream,data+10,plain);
  mbedtls_aes_free(&aes); return rc==0;
}

bool VictronDecoder::decodeSmartShunt(const uint8_t *data,size_t length,const String &key,SmartShuntReading &r){
  r=SmartShuntReading(); if(!data||length<25||data[6]!=VICTRON_RECORD_BATTERY_MONITOR)return false;
  uint8_t p[32]={0}; size_t n=0; if(!decryptPayload(data,length,key,p,n)||n<15)return false;
  uint16_t ttg=p[0]|(uint16_t(p[1])<<8), vr=p[2]|(uint16_t(p[3])<<8);
  if(vr!=0x7FFF){r.voltage=signExtend(vr,16)/100.0f;r.voltageAvailable=true;}
  r.alarms=p[4]|(uint16_t(p[5])<<8); r.auxMode=p[8]&3;
  uint32_t cr=((p[8]&0xFC)>>2)|((p[9]&3)<<6)|(((p[9]&0xFC)>>2)<<8)|((p[10]&3)<<14)|(((p[10]&0x7C)>>2)<<16)|(((p[10]&0x80)>>7)<<21);
  if((cr&0x1FFFFF)!=0x1FFFFF){r.current=signExtend(cr,22)/1000.0f;r.currentAvailable=true;}
  uint32_t ah=p[11]|(uint32_t(p[12])<<8)|(uint32_t(p[13]&0x0F)<<16); if(ah!=0xFFFFF){r.consumedAh=ah/10.0f;r.consumedAhAvailable=true;}
  uint16_t soc=((p[13]&0xF0)>>4)|((p[14]&0x0F)<<4)|((p[14]&0x30)<<4); if(soc!=0x3FF&&soc<=1000){r.soc=soc/10.0f;r.socAvailable=true;}
  if(ttg!=0xFFFF){r.remainingMinutes=ttg;r.remainingTimeAvailable=true;}
  if(!r.voltageAvailable||!r.currentAvailable||r.voltage<-5||r.voltage>100||r.current<-2500||r.current>2500||(r.socAvailable&&(r.soc<0||r.soc>100)))return false;
  r.valid=true; return true;
}

bool VictronDecoder::decodeCharger(const uint8_t *data,size_t length,const String &key,VictronChargerReading &r){
  r=VictronChargerReading(); if(!data||length<11)return false; r.recordType=data[6];
  uint8_t p[32]={0}; size_t n=0; if(!decryptPayload(data,length,key,p,n))return false; BitReader b(p,n);
  if(r.recordType==VICTRON_RECORD_SOLAR_CHARGER){
    r.state=b.u(8);r.error=b.u(8);int32_t v=b.s(16),a=b.s(16);uint32_t y=b.u(16),w=b.u(16),load=b.u(9);
    if(!b.ok())return false; if(v!=0x7FFF){r.outputVoltage=v/100.0f;r.outputVoltageAvailable=true;} if(a!=0x7FFF){r.outputCurrent=a/10.0f;r.outputCurrentAvailable=true;}
    if(y!=0xFFFF){r.yieldTodayWh=y*10.0f;r.yieldTodayAvailable=true;} if(w!=0xFFFF){r.solarPower=w;r.solarPowerAvailable=true;} if(load!=0x1FF){r.loadCurrent=load/10.0f;r.loadCurrentAvailable=true;r.outputOn=true;r.outputStateAvailable=true;}
  } else if(r.recordType==VICTRON_RECORD_DCDC_CONVERTER){
    r.state=b.u(8);r.error=b.u(8);uint32_t vin=b.u(16);int32_t vout=b.s(16);r.offReason=b.u(32); if(!b.ok())return false;
    if(vin!=0xFFFF){r.inputVoltage=vin/100.0f;r.inputVoltageAvailable=true;} if(vout!=0x7FFF){r.outputVoltage=vout/100.0f;r.outputVoltageAvailable=true;}
  } else if(r.recordType==VICTRON_RECORD_ORION_XS){
    r.state=b.u(8);r.error=b.u(8);uint32_t vo=b.u(16),ao=b.u(16),vi=b.u(16),ai=b.u(16);r.offReason=b.u(32);if(!b.ok())return false;
    if(vo!=0xFFFF){r.outputVoltage=vo/100.0f;r.outputVoltageAvailable=true;}if(ao!=0xFFFF){r.outputCurrent=ao/10.0f;r.outputCurrentAvailable=true;}if(vi!=0xFFFF){r.inputVoltage=vi/100.0f;r.inputVoltageAvailable=true;}if(ai!=0xFFFF){r.inputCurrent=ai/10.0f;r.inputCurrentAvailable=true;}
  } else if(r.recordType==VICTRON_RECORD_AC_CHARGER){
    r.state=b.u(8);r.error=b.u(8);uint32_t v1=b.u(13),a1=b.u(11); b.u(13);b.u(11);b.u(13);b.u(11);b.u(7);uint32_t ac=b.u(9);if(!b.ok())return false;
    if(v1!=0x1FFF){r.outputVoltage=v1/100.0f;r.outputVoltageAvailable=true;}if(a1!=0x7FF){r.outputCurrent=a1/10.0f;r.outputCurrentAvailable=true;}if(ac!=0x1FF){r.inputCurrent=ac/10.0f;r.inputCurrentAvailable=true;}
  } else if(r.recordType==VICTRON_RECORD_SMART_BATTERY_PROTECT){
    r.state=b.u(8);uint32_t os=b.u(8);r.error=b.u(8);b.u(16);b.u(16);int32_t vin=b.s(16);uint32_t vout=b.u(16);r.offReason=b.u(32);if(!b.ok())return false;
    if(vin!=0x7FFF){r.inputVoltage=vin/100.0f;r.inputVoltageAvailable=true;}if(vout!=0xFFFF){r.outputVoltage=vout/100.0f;r.outputVoltageAvailable=true;}if(os!=0xFF){r.outputOn=(os==1);r.outputStateAvailable=true;}
  } else return false;
  if(r.outputVoltageAvailable&&(r.outputVoltage<0||r.outputVoltage>100))return false; if(r.inputVoltageAvailable&&(r.inputVoltage<0||r.inputVoltage>100))return false;
  if(r.outputCurrentAvailable&&r.outputCurrent>1000)return false; r.valid=true;return true;
}

const char *VictronDecoder::recordTypeName(uint8_t t){switch(t){case 1:return"SmartSolar MPPT";case 2:return"Battery monitor";case 4:return"Orion Smart";case 8:return"Blue Smart Charger";case 9:return"Smart BatteryProtect";case 15:return"Orion XS";default:return"Victron device";}}
