#include <libsnes.hpp>

#include "snes9x.h"
#include "memmap.h"
#include "apu/apu.h"
#include "gfx.h"
#include "snapshot.h"
#include "controls.h"
#include "cheats.h"
#include "movie.h"
#include "logger.h"
#include "display.h"
#include "conffile.h"
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>


static snes_video_refresh_t s9x_video_cb = NULL;
static snes_audio_sample_t s9x_audio_cb = NULL;
static snes_input_poll_t s9x_poller_cb = NULL;
static snes_input_state_t s9x_input_state_cb = NULL;

void snes_set_video_refresh(snes_video_refresh_t cb)
{
   s9x_video_cb = cb;
}

void snes_set_audio_sample(snes_audio_sample_t cb)
{
   s9x_audio_cb = cb;
}

void snes_set_input_poll(snes_input_poll_t cb)
{
   s9x_poller_cb = cb;
}

void snes_set_input_state(snes_input_state_t cb)
{
   s9x_input_state_cb = cb;
}

// Just pick a big buffer. We won't use it all.
static int16_t audio_buf[0x10000];
static void S9xAudioCallback(void*)
{
   S9xFinalizeSamples();
   size_t avail = S9xGetSampleCount();
   S9xMixSamples((uint8*)audio_buf, avail);
   for (size_t i = 0; i < avail; i+=2)
      s9x_audio_cb((uint16_t)audio_buf[i], (uint16_t)audio_buf[i + 1]);
}

unsigned snes_library_revision_major()
{
   return 1;
}

unsigned snes_library_revision_minor()
{
   return 1;
}

void snes_power()
{
   S9xReset();
}

void snes_reset()
{
   S9xMovieUpdateOnReset();
   if (S9xMoviePlaying())
   {
      S9xMovieStop(true);
   }
   S9xSoftReset();
}

void snes_set_controller_port_device(bool, unsigned)
{}

void snes_cheat_reset()
{}

void snes_cheat_set(unsigned, bool, const char*)
{}

bool snes_load_cartridge_bsx_slotted(
      const char *, const uint8_t *, unsigned,
      const char *, const uint8_t *, unsigned
      )
{
   return false;
}

bool snes_load_cartridge_bsx(
      const char *, const uint8_t *, unsigned,
      const char *, const uint8_t *, unsigned
      )
{
   return false;
}

bool snes_load_cartridge_sufami_turbo(
      const char *, const uint8_t *, unsigned,
      const char *, const uint8_t *, unsigned,
      const char *, const uint8_t *, unsigned
      )
{
   return false;
}

bool snes_load_cartridge_super_game_boy(
      const char *, const uint8_t *, unsigned,
      const char *, const uint8_t *, unsigned 
      )
{
   return false;
}


void snes_init()
{
   memset(&Settings, 0, sizeof(Settings));
   Settings.MouseMaster = TRUE;
   Settings.SuperScopeMaster = TRUE;
   Settings.JustifierMaster = TRUE;
   Settings.MultiPlayer5Master = TRUE;
   Settings.FrameTimePAL = 20000;
   Settings.FrameTimeNTSC = 16667;
   Settings.SixteenBitSound = TRUE;
   Settings.Stereo = TRUE;
   Settings.SoundPlaybackRate = 32000;
   Settings.SoundInputRate = 32000;
   Settings.SupportHiRes = TRUE;
   Settings.Transparency = TRUE;
   Settings.AutoDisplayMessages = TRUE;
   Settings.InitialInfoStringTimeout = 120;
   Settings.HDMATimingHack = 100;
   Settings.BlockInvalidVRAMAccessMaster = TRUE;
   Settings.StopEmulation = TRUE;
   Settings.WrongMovieStateProtection = TRUE;
   Settings.DumpStreamsMaxFrames = -1;
   Settings.StretchScreenshots = 1;
   Settings.SnapshotScreenshots = TRUE;
   Settings.SkipFrames = AUTO_FRAMERATE;
   Settings.TurboSkipFrames = 15;
   Settings.CartAName[0] = 0;
   Settings.CartBName[0] = 0;
   Settings.AutoSaveDelay = 1;

   CPU.Flags = 0;

   if (!Memory.Init() || !S9xInitAPU())
   {
      Memory.Deinit();
      S9xDeinitAPU();
      fprintf(stderr, "[libsnes]: Failed to init Memory or APU.\n");
      exit(1);
   }

   S9xInitSound(64, 0);
   S9xSetSoundMute(FALSE);
   S9xSetSamplesAvailableCallback(S9xAudioCallback, NULL);

   S9xSetRenderPixelFormat(RGB555);
   GFX.Pitch = 2048;
   GFX.Screen = (uint16*) calloc(1, GFX.Pitch * 512 * sizeof(uint16));
   S9xGraphicsInit();
}

#define MAP_BUTTON(id, name) S9xMapButton((id), S9xGetCommandT((name)), false)
#define MAKE_BUTTON(pad, btn) (((pad)<<4)|(btn))

#define PAD_1 1
#define PAD_2 2

#define BTN_B SNES_DEVICE_ID_JOYPAD_B
#define BTN_Y SNES_DEVICE_ID_JOYPAD_Y
#define BTN_SELECT SNES_DEVICE_ID_JOYPAD_SELECT
#define BTN_START SNES_DEVICE_ID_JOYPAD_START
#define BTN_UP SNES_DEVICE_ID_JOYPAD_UP
#define BTN_DOWN SNES_DEVICE_ID_JOYPAD_DOWN
#define BTN_LEFT SNES_DEVICE_ID_JOYPAD_LEFT
#define BTN_RIGHT SNES_DEVICE_ID_JOYPAD_RIGHT
#define BTN_A SNES_DEVICE_ID_JOYPAD_A
#define BTN_X SNES_DEVICE_ID_JOYPAD_X
#define BTN_L SNES_DEVICE_ID_JOYPAD_L
#define BTN_R SNES_DEVICE_ID_JOYPAD_R
#define BTN_FIRST BTN_B
#define BTN_LAST BTN_R

static void map_buttons()
{
   MAP_BUTTON(MAKE_BUTTON(PAD_1, BTN_A), "Joypad1 A");
   MAP_BUTTON(MAKE_BUTTON(PAD_1, BTN_B), "Joypad1 B");
   MAP_BUTTON(MAKE_BUTTON(PAD_1, BTN_X), "Joypad1 X");
   MAP_BUTTON(MAKE_BUTTON(PAD_1, BTN_Y), "Joypad1 Y");
   MAP_BUTTON(MAKE_BUTTON(PAD_1, BTN_SELECT), "Joypad1 Select");
   MAP_BUTTON(MAKE_BUTTON(PAD_1, BTN_START), "Joypad1 Start");
   MAP_BUTTON(MAKE_BUTTON(PAD_1, BTN_L), "Joypad1 L");
   MAP_BUTTON(MAKE_BUTTON(PAD_1, BTN_R), "Joypad1 R");
   MAP_BUTTON(MAKE_BUTTON(PAD_1, BTN_LEFT), "Joypad1 Left");
   MAP_BUTTON(MAKE_BUTTON(PAD_1, BTN_RIGHT), "Joypad1 Right");
   MAP_BUTTON(MAKE_BUTTON(PAD_1, BTN_UP), "Joypad1 Up");
   MAP_BUTTON(MAKE_BUTTON(PAD_1, BTN_DOWN), "Joypad1 Down");

   MAP_BUTTON(MAKE_BUTTON(PAD_2, BTN_A), "Joypad2 A");
   MAP_BUTTON(MAKE_BUTTON(PAD_2, BTN_B), "Joypad2 B");
   MAP_BUTTON(MAKE_BUTTON(PAD_2, BTN_X), "Joypad2 X");
   MAP_BUTTON(MAKE_BUTTON(PAD_2, BTN_Y), "Joypad2 Y");
   MAP_BUTTON(MAKE_BUTTON(PAD_2, BTN_SELECT), "Joypad2 Select");
   MAP_BUTTON(MAKE_BUTTON(PAD_2, BTN_START), "Joypad2 Start");
   MAP_BUTTON(MAKE_BUTTON(PAD_2, BTN_L), "Joypad2 L");
   MAP_BUTTON(MAKE_BUTTON(PAD_2, BTN_R), "Joypad2 R");
   MAP_BUTTON(MAKE_BUTTON(PAD_2, BTN_LEFT), "Joypad2 Left");
   MAP_BUTTON(MAKE_BUTTON(PAD_2, BTN_RIGHT), "Joypad2 Right");
   MAP_BUTTON(MAKE_BUTTON(PAD_2, BTN_UP), "Joypad2 Up");
   MAP_BUTTON(MAKE_BUTTON(PAD_2, BTN_DOWN), "Joypad2 Down");
}

static void report_buttons()
{
   for (int pad = PAD_1; pad <= PAD_2; pad++)
   {
      for (int i = BTN_FIRST; i <= BTN_LAST; i++)
         S9xReportButton(MAKE_BUTTON(pad, i), s9x_input_state_cb(pad == PAD_2, SNES_DEVICE_JOYPAD, 0, i));
   }
}

#define S9X_TMP_ROM_PATH "/tmp/.s9x.rom.sfc"
#define S9X_TMP_SRM_PATH "/tmp/.s9x.srm.tmp"
#define S9X_TMP_SRM_PATH_2 "/tmp/.s9x.srm.tmp.2"
#define S9X_TMP_STATE_PATH "/tmp/.s9x.state.tmp"

bool snes_load_cartridge_normal(const char *, const uint8_t *rom_data, unsigned rom_size)
{
   // Hack. S9x cannot do stuff from RAM. <_<
   FILE *file = fopen(S9X_TMP_ROM_PATH, "wb");
   if (!file)
      return false;

   fwrite(rom_data, 1, rom_size, file);
   fclose(file);

   int loaded = Memory.LoadROM(S9X_TMP_ROM_PATH);
   if (!loaded)
   {
      fprintf(stderr, "[libsnes]: Rom loading failed...\n");
      return false;
   }
   unlink(S9X_TMP_ROM_PATH);

   S9xInitInputDevices();
   S9xSetController(0, CTL_JOYPAD, 0, 0, 0, 0);
   S9xSetController(1, CTL_JOYPAD, 1, 0, 0, 0);

   S9xUnmapAllControls();
   map_buttons();

   return true;
}

static bool need_load_sram = false;
static bool first_iteration = true;
void snes_run()
{
   // We can't really signal S9x that we need to reload SRAM. Luckily, you'd normally only deal with SRAM at start and end of emulation.
   if (first_iteration && need_load_sram)
   {
      Memory.LoadSRAM(S9X_TMP_SRM_PATH);
      first_iteration = false;
   }
   s9x_poller_cb();
   report_buttons();
   S9xMainLoop();
}

void snes_term()
{
   S9xDeinitAPU();
   Memory.Deinit();
   S9xGraphicsDeinit();
   S9xUnmapAllControls();

   unlink(S9X_TMP_SRM_PATH);
   unlink(S9X_TMP_STATE_PATH);
}


bool snes_get_region()
{ 
   return Settings.PAL ? SNES_REGION_PAL : SNES_REGION_NTSC; 
}

// We need to get really dirty here... Have to MMAP a save file :(
static void *sram_mapped = NULL;
static int mmap_fd = -1;
static ssize_t mmap_len = -1;

uint8_t* snes_get_memory_data(unsigned type)
{
   if (type != SNES_MEMORY_CARTRIDGE_RAM)
      return NULL;

   // Set up a memory map for SRAM.
   if (sram_mapped == NULL)
   {
      Memory.SaveSRAM(S9X_TMP_SRM_PATH);

      mmap_fd = open(S9X_TMP_SRM_PATH, O_RDWR);
      if (mmap_fd == -1)
         return NULL;

      struct stat info;
      fstat(mmap_fd, &info);

      sram_mapped = mmap(NULL, info.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, mmap_fd, 0);
      if (!sram_mapped)
      {
         fprintf(stderr, "[libsnes]: MMAP failed!\n");
         close(mmap_fd);
         mmap_fd = -1;
         return NULL;
      }
      mmap_len = info.st_size;
   }

   need_load_sram = true;
   return (uint8_t*)sram_mapped;
}

void snes_unload_cartridge()
{
   need_load_sram = false;
   first_iteration = true;

   if (sram_mapped)
   {
      munmap(sram_mapped, mmap_len);
      sram_mapped = NULL;
      mmap_len = -1;
   }
   if (mmap_fd >= 0)
   {
      close(mmap_fd);
      mmap_fd = -1;
   }
}

unsigned snes_get_memory_size(unsigned type)
{
   if (type != SNES_MEMORY_CARTRIDGE_RAM)
      return 0;

   // If we already know the size, return it.
   if (mmap_len >= 0)
      return mmap_len;

   // Hack :)
   Memory.SaveSRAM(S9X_TMP_SRM_PATH_2);
   FILE *tmp = fopen(S9X_TMP_SRM_PATH_2, "rb");
   if (!tmp)
      return 0;
   fseek(tmp, 0, SEEK_END);
   long len = ftell(tmp);
   fclose(tmp);

   unlink(S9X_TMP_SRM_PATH_2);
   return (unsigned)len;
}

void snes_set_cartridge_basename(const char*)
{}

#if S9X_SAVESTATES
unsigned snes_serialize_size()
{
   if (S9xFreezeGame(S9X_TMP_STATE_PATH) == FALSE)
   {
      return 0;
   }

   FILE *tmp = fopen(S9X_TMP_STATE_PATH, "rb");
   if (!tmp)
      return 0;

   fseek(tmp, 0, SEEK_END);
   long len = ftell(tmp);
   fclose(tmp);

   return (unsigned)len;
}

bool snes_serialize(uint8_t *data, unsigned size)
{ 
   if (S9xFreezeGame(S9X_TMP_STATE_PATH) == FALSE)
   {
      return false;
   }

   FILE *tmp = fopen(S9X_TMP_STATE_PATH, "rb");
   if (!tmp)
   {
      return false;
   }

   size_t len = fread(data, 1, size, tmp);
   if (len != size)
   {
      fclose(tmp);
      return false;
   }
   fclose(tmp);
   return true;
}

bool snes_unserialize(const uint8_t* data, unsigned size)
{ 
   FILE *tmp = fopen(S9X_TMP_STATE_PATH, "wb");
   if (!tmp)
   {
      return false;
   }

   fwrite(data, 1, size, tmp);
   fclose(tmp);

   if (S9xUnfreezeGame(S9X_TMP_STATE_PATH) == FALSE)
   {
      return false;
   }
   return true;
}
#else
unsigned snes_serialize_size() { return 0; }
bool snes_serialize(uint8_t *, unsigned) { return false; }
bool snes_unserialize(const uint8_t*, unsigned) { return false; }
#endif

// Pitch 2048 -> 1024, only done once per res-change.
static void pack_frame(uint16_t *frame, int width, int height)
{
   for (int y = 1; y < height; y++)
   {
      uint16_t *src = frame + y * 1024;
      uint16_t *dst = frame + y * 512;

      memcpy(dst, src, width * sizeof(uint16_t));
   }
}

// Pitch 1024 -> 2048, only done once per res-change.
static void stretch_frame(uint16_t *frame, int width, int height)
{
   for (int y = height - 1; y >= 0; y--)
   {
      uint16_t *src = frame + y * 512;
      uint16_t *dst = frame + y * 1024;

      memcpy(dst, src, width * sizeof(uint16_t));
   }
}

bool8 S9xDeinitUpdate(int width, int height)
{
   if (height == 448 || height == 478)
   {
      if (GFX.Pitch == 2048)
         pack_frame(GFX.Screen, width, height);
      GFX.Pitch = 1024;
   }
   else
   {
      if (GFX.Pitch == 1024)
         stretch_frame(GFX.Screen, width, height);
      GFX.Pitch = 2048;
   }

   s9x_video_cb(GFX.Screen, width, height);
   return TRUE;
}

bool8 S9xContinueUpdate(int width, int height)
{
   return TRUE;
}


// Dummy functions that should probably be implemented correctly later.
void S9xParsePortConfig(ConfigFile&, int) {}
void S9xSyncSpeed() {}
void S9xPollPointer(int, short*, short*) {}
const char* S9xStringInput(const char* in) { return in; }
const char* S9xGetFilename(const char* in, s9x_getdirtype) { return in; }
const char* S9xGetDirectory(s9x_getdirtype) { return NULL; }
void S9xInitInputDevices() {}
const char* S9xChooseFilename(unsigned char) { return NULL; }
void S9xHandlePortCommand(s9xcommand_t, short, short) {}
bool S9xPollButton(unsigned int, bool*) { return false; }
void S9xToggleSoundChannel(int) {}
const char* S9xGetFilenameInc(const char* in, s9x_getdirtype) { return NULL; }
const char* S9xBasename(const char* in) { return in; }
bool8 S9xInitUpdate() { return TRUE; }
void S9xExtraUsage() {}
bool8 S9xOpenSoundDevice() { return TRUE; }
void S9xMessage(int, int, const char*) {}
bool S9xPollAxis(unsigned int, short*) { return FALSE; }
void S9xSetPalette() {}
void S9xParseArg(char**, int&, int) {}
void S9xExit() {}
bool S9xPollPointer(unsigned int, short*, short*) { return false; }
const char *S9xChooseMovieFilename(unsigned char) { return NULL; }

#ifdef S9X_SAVESTATES
// Stupid callbacks...
bool8 S9xOpenSnapshotFile(const char* filepath, bool8 read_only, STREAM *file) 
{ 
   if(read_only)
   {
      if((*file = OPEN_STREAM(filepath, "rb")) != 0)
      {
         return (TRUE);
      }
   }
   else
   {
      if((*file = OPEN_STREAM(filepath, "wb")) != 0)
      {
         return (TRUE);
      }
   }
   return (FALSE);
}

void S9xCloseSnapshotFile(STREAM file) 
{
   CLOSE_STREAM(file);
}
#else
bool8 S9xOpenSnapshotFile(const char* filepath, bool8 read_only, STREAM *file) { return FALSE; }
void S9xCloseSnapshotFile(STREAM file) {}
#endif

void S9xAutoSaveSRAM() 
{
   Memory.SaveSRAM(S9X_TMP_SRM_PATH);
}

// S9x weirdness.
void _splitpath (const char *path, char *drive, char *dir, char *fname, char *ext)
{
   *drive = 0;

   const char	*slash = strrchr(path, SLASH_CHAR),
         *dot   = strrchr(path, '.');

   if (dot && slash && dot < slash)
      dot = NULL;

   if (!slash)
   {
      *dir = 0;

      strcpy(fname, path);

      if (dot)
      {
         fname[dot - path] = 0;
         strcpy(ext, dot + 1);
      }
      else
         *ext = 0;
   }
   else
   {
      strcpy(dir, path);
      dir[slash - path] = 0;

      strcpy(fname, slash + 1);

      if (dot)
      {
         fname[dot - slash - 1] = 0;
         strcpy(ext, dot + 1);
      }
      else
         *ext = 0;
   }
}

void _makepath (char *path, const char *, const char *dir, const char *fname, const char *ext)
{
   if (dir && *dir)
   {
      strcpy(path, dir);
      strcat(path, SLASH_STR);
   }
   else
      *path = 0;

   strcat(path, fname);

   if (ext && *ext)
   {
      strcat(path, ".");
      strcat(path, ext);
   }
}

