#pragma once
#include <string_view>
#include <unordered_map>

#include "common.hpp"

enum REG : u32 {
  DISPCNT    = 0x4000000,  //   LCD Control
  GREEN_SWAP = 0x4000002,  //   Undocumented - Green Swap
  DISPSTAT   = 0x4000004,  //   General LCD Status (STAT,LYC)
  VCOUNT     = 0x4000006,  //   Vertical Counter (LY)
  BG0CNT     = 0x4000008,  //   BG0 Control
  BG1CNT     = 0x400000A,  //   BG1 Control
  BG2CNT     = 0x400000C,  //   BG2 Control
  BG3CNT     = 0x400000E,  //   BG3 Control
  BG0HOFS    = 0x4000010,  //   BG0 X-Offset
  BG0VOFS    = 0x4000012,  //   BG0 Y-Offset
  BG1HOFS    = 0x4000014,  //   BG1 X-Offset
  BG1VOFS    = 0x4000016,  //   BG1 Y-Offset
  BG2HOFS    = 0x4000018,  //   BG2 X-Offset
  BG2VOFS    = 0x400001A,  //   BG2 Y-Offset
  BG3HOFS    = 0x400001C,  //   BG3 X-Offset
  BG3VOFS    = 0x400001E,  //   BG3 Y-Offset
  BG2PA      = 0x4000020,  //   BG2 Rotation/Scaling Parameter A (dx)
  BG2PB      = 0x4000022,  //   BG2 Rotation/Scaling Parameter B (dmx)
  BG2PC      = 0x4000024,  //   BG2 Rotation/Scaling Parameter C (dy)
  BG2PD      = 0x4000026,  //   BG2 Rotation/Scaling Parameter D (dmy)
  BG2X       = 0x4000028,  //   BG2 Reference Point X-Coordinate
  BG2Y       = 0x400002C,  //   BG2 Reference Point Y-Coordinate
  BG3PA      = 0x4000030,  //   BG3 Rotation/Scaling Parameter A (dx)
  BG3PB      = 0x4000032,  //   BG3 Rotation/Scaling Parameter B (dmx)
  BG3PC      = 0x4000034,  //   BG3 Rotation/Scaling Parameter C (dy)
  BG3PD      = 0x4000036,  //   BG3 Rotation/Scaling Parameter D (dmy)
  BG3X       = 0x4000038,  //   BG3 Reference Point X-Coordinate
  BG3Y       = 0x400003C,  //   BG3 Reference Point Y-Coordinate
  WIN0H      = 0x4000040,  //   Window 0 Horizontal Dimensions
  WIN1H      = 0x4000042,  //   Window 1 Horizontal Dimensions
  WIN0V      = 0x4000044,  //   Window 0 Vertical Dimensions
  WIN1V      = 0x4000046,  //   Window 1 Vertical Dimensions
  WININ      = 0x4000048,  //   Inside of Window 0 and 1
  WINOUT     = 0x400004A,  //   Inside of OBJ Window & Outside of Windows
  MOSAIC     = 0x400004C,  //   Mosaic Size
  BLDCNT     = 0x4000050,  //   Color Special Effects Selection
  BLDALPHA   = 0x4000052,  //   Alpha Blending Coefficients
  BLDY       = 0x4000054,  //   Brightness (Fade-In/Out) Coefficient

  SOUND1CNT_L  = 0x4000060,  //  Channel 1 Sweep register       (NR10)
  SOUND1CNT_H  = 0x4000062,  //  Channel 1 Duty/Length/Envelope (NR11, NR12)
  SOUND1CNT_X  = 0x4000064,  //  Channel 1 Frequency/Control    (NR13, NR14)
  SOUND2CNT_L  = 0x4000068,  //  Channel 2 Duty/Length/Envelope (NR21, NR22)
  SOUND2CNT_H  = 0x400006C,  //  Channel 2 Frequency/Control    (NR23, NR24)
  SOUND3CNT_L  = 0x4000070,  //  Channel 3 Stop/Wave RAM select (NR30)
  SOUND3CNT_H  = 0x4000072,  //  Channel 3 Length/Volume        (NR31, NR32)
  SOUND3CNT_X  = 0x4000074,  //  Channel 3 Frequency/Control    (NR33, NR34)
  SOUND4CNT_L  = 0x4000078,  //  Channel 4 Length/Envelope      (NR41, NR42)
  SOUND4CNT_H  = 0x400007C,  //  Channel 4 Frequency/Control    (NR43, NR44)
  SOUNDCNT_L   = 0x4000080,  //  Control Stereo/Volume/Enable   (NR50, NR51)
  SOUNDCNT_H   = 0x4000082,  //  Control Mixing/DMA Control
  SOUNDCNT_X   = 0x4000084,  //  Control Sound on/off           (NR52)
  SOUNDBIAS    = 0x4000088,  //  Sound PWM Control
  WAVE_RAM     = 0x4000090,  //  Channel 3 Wave Pattern RAM (2 banks!!)
  FIFO_A       = 0x40000A0,  //  Channel A FIFO, Data 0-3
  FIFO_B       = 0x40000A4,  //  Channel B FIFO, Data 0-3
  DMA0SAD      = 0x40000B0,  //  DMA 0 Source Address
  DMA0DAD      = 0x40000B4,  //  DMA 0 Destination Address
  DMA0CNT_L    = 0x40000B8,  //  DMA 0 Word Count
  DMA0CNT_H    = 0x40000BA,  //  DMA 0 Control
  DMA1SAD      = 0x40000BC,  //  DMA 1 Source Address
  DMA1DAD      = 0x40000C0,  //  DMA 1 Destination Address
  DMA1CNT_L    = 0x40000C4,  //  DMA 1 Word Count
  DMA1CNT_H    = 0x40000C6,  //  DMA 1 Control
  DMA2SAD      = 0x40000C8,  //  DMA 2 Source Address
  DMA2DAD      = 0x40000CC,  //  DMA 2 Destination Address
  DMA2CNT_L    = 0x40000D0,  //  DMA 2 Word Count
  DMA2CNT_H    = 0x40000D2,  //  DMA 2 Control
  DMA3SAD      = 0x40000D4,  //  DMA 3 Source Address
  DMA3DAD      = 0x40000D8,  //  DMA 3 Destination Address
  DMA3CNT_L    = 0x40000DC,  //  DMA 3 Word Count
  DMA3CNT_H    = 0x40000DE,  //  DMA 3 Control
  TM0CNT_L     = 0x4000100,  //  Timer 0 Counter/Reload
  TM0CNT_H     = 0x4000102,  //  Timer 0 Control
  TM1CNT_L     = 0x4000104,  //  Timer 1 Counter/Reload
  TM1CNT_H     = 0x4000106,  //  Timer 1 Control
  TM2CNT_L     = 0x4000108,  //  Timer 2 Counter/Reload
  TM2CNT_H     = 0x400010A,  //  Timer 2 Control
  TM3CNT_L     = 0x400010C,  //  Timer 3 Counter/Reload
  TM3CNT_H     = 0x400010E,  //  Timer 3 Control
  SIODATA32    = 0x4000120,  //  SIO Data (Normal-32bit Mode; shared with below)
  SIOMULTI0    = 0x4000120,  //  SIO Data 0 (Parent)    (Multi-Player Mode)
  SIOMULTI1    = 0x4000122,  //  SIO Data 1 (1st Child) (Multi-Player Mode)
  SIOMULTI2    = 0x4000124,  //  SIO Data 2 (2nd Child) (Multi-Player Mode)
  SIOMULTI3    = 0x4000126,  //  SIO Data 3 (3rd Child) (Multi-Player Mode)
  SIOCNT       = 0x4000128,  //  SIO Control Register
  SIOMLT_SEND  = 0x400012A,  //  SIO Data (Local of MultiPlayer; shared below)
  SIODATA8     = 0x400012A,  //  SIO Data (Normal-8bit and UART Mode)
  KEYINPUT     = 0x4000130,  //  Key Status
  KEYCNT       = 0x4000132,  //  Key Interrupt Control
  RCNT         = 0x4000134,  //  SIO Mode Select/General Purpose Data
  UNKNOWN136   = 0x4000136,
  JOYCNT       = 0x4000140,  //  SIO JOY Bus Control
  UNKNOWN142   = 0x4000142,
  JOY_RECV     = 0x4000150,  //  SIO JOY Bus Receive Data
  JOY_TRANS    = 0x4000154,  //  SIO JOY Bus Transmit Data
  JOYSTAT      = 0x4000158,  //  SIO JOY Bus Receive Status
  UNKNOWN15A   = 0x400015A,
  IE           = 0x4000200,  //  Interrupt Enable Register
  IF           = 0x4000202,  //  Interrupt Request Flags / IRQ Acknowledge
  WAITCNT      = 0x4000204,  //  Game Pak Waitstate Control
  UNKNOWN206   = 0x4000206,
  IME          = 0x4000208,  //  Interrupt Master Enable Register
  POSTFLG      = 0x4000300,  //  Undocumented - Post Boot Flag
  HALTCNT      = 0x4000301,  //  Undocumented - Power Down Control
  UNKNOWN302   = 0x4000302,
  UNKNOWN1000C = 0x400100C
};

const std::unordered_map<u32, std::string> label_map = {
    {   DISPSTAT,    "DISPSTAT"},
    {     VCOUNT,      "VCOUNT"},
    {     BG0CNT,      "BG0CNT"},
    {     BG1CNT,      "BG1CNT"},
    {     BG2CNT,      "BG2CNT"},
    {     BG3CNT,      "BG3CNT"},
    {    BG0HOFS,     "BG0HOFS"},
    {    BG0VOFS,     "BG0VOFS"},
    {    BG1HOFS,     "BG1HOFS"},
    {    BG1VOFS,     "BG1VOFS"},
    {    BG2HOFS,     "BG2HOFS"},
    {    BG2VOFS,     "BG2VOFS"},
    {    BG3HOFS,     "BG3HOFS"},
    {    BG3VOFS,     "BG3VOFS"},
    // {     BG2X_L,      "BG2X_L"},
    // {     BG2X_H,      "BG2X_H"},
    // {     BG2Y_L,      "BG2Y_L"},
    // {     BG2Y_H,      "BG2Y_H"},
    {      BG2PA,       "BG2PA"},
    {      BG2PB,       "BG2PB"},
    {      BG2PC,       "BG2PC"},
    {      BG2PD,       "BG2PD"},
    {      BG3PA,       "BG3PA"},
    {      BG3PB,       "BG3PB"},
    {      BG3PC,       "BG3PC"},
    {      BG3PD,       "BG3PD"},
    {SOUND1CNT_L, "SOUND1CNT_L"},
    {SOUND1CNT_H, "SOUND1CNT_H"},
    {SOUND1CNT_X, "SOUND1CNT_X"},
    {SOUND2CNT_L, "SOUND2CNT_L"},
    {SOUND2CNT_H, "SOUND2CNT_H"},
    {SOUND3CNT_L, "SOUND3CNT_L"},
    {SOUND3CNT_H, "SOUND3CNT_H"},
    {SOUND3CNT_X, "SOUND3CNT_X"},
    {SOUND4CNT_L, "SOUND4CNT_L"},
    {SOUND4CNT_H, "SOUND4CNT_H"},
    { SOUNDCNT_L,  "SOUNDCNT_L"},
    { SOUNDCNT_H,  "SOUNDCNT_H"},
    { SOUNDCNT_X,  "SOUNDCNT_X"},
    {  SOUNDBIAS,   "SOUNDBIAS"},
    {   WAVE_RAM,    "WAVE_RAM"},
    {     FIFO_A,      "FIFO_A"},
    {     FIFO_B,      "FIFO_B"},
    {    DMA0SAD,     "DMA0SAD"},
    {    DMA0DAD,     "DMA0DAD"},
    {  DMA0CNT_L,   "DMA0CNT_L"},
    {  DMA0CNT_H,   "DMA0CNT_H"},
    {    DMA1SAD,     "DMA1SAD"},
    {    DMA1DAD,     "DMA1DAD"},
    {  DMA1CNT_L,   "DMA1CNT_L"},
    {  DMA1CNT_H,   "DMA1CNT_H"},
    {    DMA2SAD,     "DMA2SAD"},
    {    DMA2DAD,     "DMA2DAD"},
    {  DMA2CNT_L,   "DMA2CNT_L"},
    {  DMA2CNT_H,   "DMA2CNT_H"},
    {    DMA3SAD,     "DMA3SAD"},
    {    DMA3DAD,     "DMA3DAD"},
    {  DMA3CNT_L,   "DMA3CNT_L"},
    {  DMA3CNT_H,   "DMA3CNT_H"},
    {   TM0CNT_L,    "TM0CNT_L"},
    {   TM0CNT_H,    "TM0CNT_H"},
    {   TM1CNT_L,    "TM1CNT_L"},
    {   TM1CNT_H,    "TM1CNT_H"},
    {   TM2CNT_L,    "TM2CNT_L"},
    {   TM2CNT_H,    "TM2CNT_H"},
    {   TM3CNT_L,    "TM3CNT_L"},
    {   TM3CNT_H,    "TM3CNT_H"},
    {  SIODATA32,   "SIODATA32"},
    {  SIOMULTI0,   "SIOMULTI0"},
    {  SIOMULTI1,   "SIOMULTI1"},
    {  SIOMULTI2,   "SIOMULTI2"},
    {  SIOMULTI3,   "SIOMULTI3"},
    {     SIOCNT,      "SIOCNT"},
    {SIOMLT_SEND, "SIOMLT_SEND"},
    {   SIODATA8,    "SIODATA8"},
    {   KEYINPUT,    "KEYINPUT"},
    {     KEYCNT,      "KEYCNT"},
    {       RCNT,        "RCNT"},
    {     JOYCNT,      "JOYCNT"},
    {   JOY_RECV,    "JOY_RECV"},
    {  JOY_TRANS,   "JOY_TRANS"},
    {    JOYSTAT,     "JOYSTAT"},
    {         IE,          "IE"},
    {         IF,          "IF"},
    {    WAITCNT,     "WAITCNT"},
    {        IME,         "IME"},
    {    POSTFLG,     "POSTFLG"},
    {    HALTCNT,     "HALTCNT"},
};

inline std::string get_label(const u32 address) {
  auto label_entry = label_map.find(address);

  std::string result = fmt::format("{:#010x}", address);

  if (label_entry != label_map.end()) { result = fmt::format("{:#010x} [{}]", address, label_entry->second); }

  return result;
}
