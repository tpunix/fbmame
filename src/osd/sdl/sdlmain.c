//============================================================
//
//  sdlmain.c - main file for SDLMAME.
//
//  Copyright (c) 1996-2014, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================


#ifdef SDLMAME_UNIX
#if (!defined(SDLMAME_MACOSX)) && (!defined(SDLMAME_EMSCRIPTEN))
#ifndef SDLMAME_HAIKU
#include <fontconfig/fontconfig.h>
#endif
#endif
#ifdef SDLMAME_MACOSX
#include <Carbon/Carbon.h>
#endif
#endif

// standard includes
#if !defined(SDLMAME_WIN32) && !defined(SDLMAME_OS2)
#include <unistd.h>
#endif

// only for strconv.h
#if defined(SDLMAME_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif


#ifdef SDLMAME_OS2
#define INCL_DOS
#include <os2.h>
#endif

#ifdef SDLMAME_EMSCRIPTEN
#include <emscripten.h>
#endif

#include "sdlinc.h"

// MAME headers
#include "osdepend.h"
#include "emu.h"
#include "clifront.h"
#include "emuopts.h"
#include "strconv.h"

// OSD headers
#include "video.h"
#include "input.h"
#include "osdsdl.h"
#include "modules/lib/osdlib.h"

// we override SDL's normal startup on Win32
// please see sdlprefix.h as well

#if defined(SDLMAME_X11) && (SDL_MAJOR_VERSION == 1) && (SDL_MINOR_VERSION == 2)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#include "watchdog.h"

//============================================================
//  OPTIONS
//============================================================

#ifndef INI_PATH
#if defined(SDLMAME_WIN32) || defined(SDLMAME_OS2)
	#define INI_PATH ".;ini"
#elif defined(SDLMAME_MACOSX)
	#define INI_PATH "$HOME/Library/Application Support/APP_NAME;$HOME/.APP_NAME;.;ini"
#else
	#define INI_PATH "$HOME/.APP_NAME;.;ini"
#endif // MACOSX
#endif // INI_PATH


//============================================================
//  Global variables
//============================================================

#if defined(SDLMAME_UNIX) || defined(SDLMAME_WIN32)
int sdl_entered_debugger;
#endif

//============================================================
//  Local variables
//============================================================

const options_entry sdl_options::s_option_entries[] =
{
	{ SDLOPTION_INIPATH,                     INI_PATH,    OPTION_STRING,     "path to ini files" },

	// performance options
	{ NULL,                                   NULL,       OPTION_HEADER,     "SDL PERFORMANCE OPTIONS" },
	{ SDLOPTION_SDLVIDEOFPS,                  "0",        OPTION_BOOLEAN,    "show sdl video performance" },
	// video options
	{ NULL,                                   NULL,       OPTION_HEADER,     "SDL VIDEO OPTIONS" },
// OS X can be trusted to have working hardware OpenGL, so default to it on for the best user experience
	{ SDLOPTION_CENTERH,                      "1",        OPTION_BOOLEAN,    "center horizontally within the view area" },
	{ SDLOPTION_CENTERV,                      "1",        OPTION_BOOLEAN,    "center vertically within the view area" },
#if (SDLMAME_SDL2)
	{ SDLOPTION_SCALEMODE ";sm",         SDLOPTVAL_NONE,  OPTION_STRING,     "Scale mode: none, hwblit, hwbest, yv12, yuy2, yv12x2, yuy2x2 (-video soft only)" },
#else
	{ SDLOPTION_SCALEMODE ";sm",         SDLOPTVAL_NONE,  OPTION_STRING,     "Scale mode: none, async, yv12, yuy2, yv12x2, yuy2x2 (-video soft only)" },
#endif
#if USE_OPENGL
	// OpenGL specific options
	{ NULL,                                   NULL,   OPTION_HEADER,  "OpenGL-SPECIFIC OPTIONS" },
	{ SDLOPTION_FILTER ";glfilter;flt",       "1",    OPTION_BOOLEAN, "enable bilinear filtering on screen output" },
	{ SDLOPTION_PRESCALE,                     "1",    OPTION_INTEGER,                 "scale screen rendering by this amount in software" },
	{ SDLOPTION_GL_FORCEPOW2TEXTURE,          "0",    OPTION_BOOLEAN, "force power of two textures  (default no)" },
	{ SDLOPTION_GL_NOTEXTURERECT,             "0",    OPTION_BOOLEAN, "don't use OpenGL GL_ARB_texture_rectangle (default on)" },
	{ SDLOPTION_GL_VBO,                       "1",    OPTION_BOOLEAN, "enable OpenGL VBO,  if available (default on)" },
	{ SDLOPTION_GL_PBO,                       "1",    OPTION_BOOLEAN, "enable OpenGL PBO,  if available (default on)" },
	{ SDLOPTION_GL_GLSL,                      "0",    OPTION_BOOLEAN, "enable OpenGL GLSL, if available (default off)" },
	{ SDLOPTION_GLSL_FILTER,                  "1",    OPTION_STRING,  "enable OpenGL GLSL filtering instead of FF filtering 0-plain, 1-bilinear (default)" },
	{ SDLOPTION_SHADER_MAME "0",     SDLOPTVAL_NONE,  OPTION_STRING,  "custom OpenGL GLSL shader set mame bitmap 0" },
	{ SDLOPTION_SHADER_MAME "1",     SDLOPTVAL_NONE,  OPTION_STRING,  "custom OpenGL GLSL shader set mame bitmap 1" },
	{ SDLOPTION_SHADER_MAME "2",     SDLOPTVAL_NONE,  OPTION_STRING,  "custom OpenGL GLSL shader set mame bitmap 2" },
	{ SDLOPTION_SHADER_MAME "3",     SDLOPTVAL_NONE,  OPTION_STRING,  "custom OpenGL GLSL shader set mame bitmap 3" },
	{ SDLOPTION_SHADER_MAME "4",     SDLOPTVAL_NONE,  OPTION_STRING,  "custom OpenGL GLSL shader set mame bitmap 4" },
	{ SDLOPTION_SHADER_MAME "5",     SDLOPTVAL_NONE,  OPTION_STRING,  "custom OpenGL GLSL shader set mame bitmap 5" },
	{ SDLOPTION_SHADER_MAME "6",     SDLOPTVAL_NONE,  OPTION_STRING,  "custom OpenGL GLSL shader set mame bitmap 6" },
	{ SDLOPTION_SHADER_MAME "7",     SDLOPTVAL_NONE,  OPTION_STRING,  "custom OpenGL GLSL shader set mame bitmap 7" },
	{ SDLOPTION_SHADER_MAME "8",     SDLOPTVAL_NONE,  OPTION_STRING,  "custom OpenGL GLSL shader set mame bitmap 8" },
	{ SDLOPTION_SHADER_MAME "9",     SDLOPTVAL_NONE,  OPTION_STRING,  "custom OpenGL GLSL shader set mame bitmap 9" },
	{ SDLOPTION_SHADER_SCREEN "0",   SDLOPTVAL_NONE,  OPTION_STRING,  "custom OpenGL GLSL shader screen bitmap 0" },
	{ SDLOPTION_SHADER_SCREEN "1",   SDLOPTVAL_NONE,  OPTION_STRING,  "custom OpenGL GLSL shader screen bitmap 1" },
	{ SDLOPTION_SHADER_SCREEN "2",   SDLOPTVAL_NONE,  OPTION_STRING,  "custom OpenGL GLSL shader screen bitmap 2" },
	{ SDLOPTION_SHADER_SCREEN "3",   SDLOPTVAL_NONE,  OPTION_STRING,  "custom OpenGL GLSL shader screen bitmap 3" },
	{ SDLOPTION_SHADER_SCREEN "4",   SDLOPTVAL_NONE,  OPTION_STRING,  "custom OpenGL GLSL shader screen bitmap 4" },
	{ SDLOPTION_SHADER_SCREEN "5",   SDLOPTVAL_NONE,  OPTION_STRING,  "custom OpenGL GLSL shader screen bitmap 5" },
	{ SDLOPTION_SHADER_SCREEN "6",   SDLOPTVAL_NONE,  OPTION_STRING,  "custom OpenGL GLSL shader screen bitmap 6" },
	{ SDLOPTION_SHADER_SCREEN "7",   SDLOPTVAL_NONE,  OPTION_STRING,  "custom OpenGL GLSL shader screen bitmap 7" },
	{ SDLOPTION_SHADER_SCREEN "8",   SDLOPTVAL_NONE,  OPTION_STRING,  "custom OpenGL GLSL shader screen bitmap 8" },
	{ SDLOPTION_SHADER_SCREEN "9",   SDLOPTVAL_NONE,  OPTION_STRING,  "custom OpenGL GLSL shader screen bitmap 9" },
#endif

	// full screen options
	#ifdef SDLMAME_X11
	{ NULL,                                   NULL,  OPTION_HEADER,     "SDL FULL SCREEN OPTIONS" },
	{ SDLOPTION_USEALLHEADS,                 "0",     OPTION_BOOLEAN,    "split full screen image across monitors" },
	#endif

	// keyboard mapping
	{ NULL,                                   NULL,  OPTION_HEADER,     "SDL KEYBOARD MAPPING" },
	{ SDLOPTION_KEYMAP,                      "0",    OPTION_BOOLEAN,    "enable keymap" },
	{ SDLOPTION_KEYMAP_FILE,                 "keymap.dat", OPTION_STRING, "keymap filename" },
#ifdef SDLMAME_MACOSX
	{ SDLOPTION_UIMODEKEY,                   "DEL", OPTION_STRING,   "Key to toggle keyboard mode" },
#else
	{ SDLOPTION_UIMODEKEY,                   "SCRLOCK", OPTION_STRING,  "Key to toggle keyboard mode" },
#endif  // SDLMAME_MACOSX

	// joystick mapping
	{ NULL,                                  NULL,   OPTION_HEADER,     "SDL JOYSTICK MAPPING" },
	{ SDLOPTION_JOYINDEX "1",                SDLOPTVAL_AUTO, OPTION_STRING,         "name of joystick mapped to joystick #1" },
	{ SDLOPTION_JOYINDEX "2",                SDLOPTVAL_AUTO, OPTION_STRING,         "name of joystick mapped to joystick #2" },
	{ SDLOPTION_JOYINDEX "3",                SDLOPTVAL_AUTO, OPTION_STRING,         "name of joystick mapped to joystick #3" },
	{ SDLOPTION_JOYINDEX "4",                SDLOPTVAL_AUTO, OPTION_STRING,         "name of joystick mapped to joystick #4" },
	{ SDLOPTION_JOYINDEX "5",                SDLOPTVAL_AUTO, OPTION_STRING,         "name of joystick mapped to joystick #5" },
	{ SDLOPTION_JOYINDEX "6",                SDLOPTVAL_AUTO, OPTION_STRING,         "name of joystick mapped to joystick #6" },
	{ SDLOPTION_JOYINDEX "7",                SDLOPTVAL_AUTO, OPTION_STRING,         "name of joystick mapped to joystick #7" },
	{ SDLOPTION_JOYINDEX "8",                SDLOPTVAL_AUTO, OPTION_STRING,         "name of joystick mapped to joystick #8" },
	{ SDLOPTION_SIXAXIS,                     "0",    OPTION_BOOLEAN,    "Use special handling for PS3 Sixaxis controllers" },

#if (USE_XINPUT)
	// lightgun mapping
	{ NULL,                                  NULL,   OPTION_HEADER,     "SDL LIGHTGUN MAPPING" },
	{ SDLOPTION_LIGHTGUNINDEX "1",           SDLOPTVAL_AUTO, OPTION_STRING,         "name of lightgun mapped to lightgun #1" },
	{ SDLOPTION_LIGHTGUNINDEX "2",           SDLOPTVAL_AUTO, OPTION_STRING,         "name of lightgun mapped to lightgun #2" },
	{ SDLOPTION_LIGHTGUNINDEX "3",           SDLOPTVAL_AUTO, OPTION_STRING,         "name of lightgun mapped to lightgun #3" },
	{ SDLOPTION_LIGHTGUNINDEX "4",           SDLOPTVAL_AUTO, OPTION_STRING,         "name of lightgun mapped to lightgun #4" },
	{ SDLOPTION_LIGHTGUNINDEX "5",           SDLOPTVAL_AUTO, OPTION_STRING,         "name of lightgun mapped to lightgun #5" },
	{ SDLOPTION_LIGHTGUNINDEX "6",           SDLOPTVAL_AUTO, OPTION_STRING,         "name of lightgun mapped to lightgun #6" },
	{ SDLOPTION_LIGHTGUNINDEX "7",           SDLOPTVAL_AUTO, OPTION_STRING,         "name of lightgun mapped to lightgun #7" },
	{ SDLOPTION_LIGHTGUNINDEX "8",           SDLOPTVAL_AUTO, OPTION_STRING,         "name of lightgun mapped to lightgun #8" },
#endif

#if (SDLMAME_SDL2)
	{ NULL,                                  NULL,   OPTION_HEADER,     "SDL MOUSE MAPPING" },
	{ SDLOPTION_MOUSEINDEX "1",              SDLOPTVAL_AUTO, OPTION_STRING,         "name of mouse mapped to mouse #1" },
	{ SDLOPTION_MOUSEINDEX "2",              SDLOPTVAL_AUTO, OPTION_STRING,         "name of mouse mapped to mouse #2" },
	{ SDLOPTION_MOUSEINDEX "3",              SDLOPTVAL_AUTO, OPTION_STRING,         "name of mouse mapped to mouse #3" },
	{ SDLOPTION_MOUSEINDEX "4",              SDLOPTVAL_AUTO, OPTION_STRING,         "name of mouse mapped to mouse #4" },
	{ SDLOPTION_MOUSEINDEX "5",              SDLOPTVAL_AUTO, OPTION_STRING,         "name of mouse mapped to mouse #5" },
	{ SDLOPTION_MOUSEINDEX "6",              SDLOPTVAL_AUTO, OPTION_STRING,         "name of mouse mapped to mouse #6" },
	{ SDLOPTION_MOUSEINDEX "7",              SDLOPTVAL_AUTO, OPTION_STRING,         "name of mouse mapped to mouse #7" },
	{ SDLOPTION_MOUSEINDEX "8",              SDLOPTVAL_AUTO, OPTION_STRING,         "name of mouse mapped to mouse #8" },

	{ NULL,                                  NULL,   OPTION_HEADER,     "SDL KEYBOARD MAPPING" },
	{ SDLOPTION_KEYBINDEX "1",               SDLOPTVAL_AUTO, OPTION_STRING,         "name of keyboard mapped to keyboard #1" },
	{ SDLOPTION_KEYBINDEX "2",               SDLOPTVAL_AUTO, OPTION_STRING,         "name of keyboard mapped to keyboard #2" },
	{ SDLOPTION_KEYBINDEX "3",               SDLOPTVAL_AUTO, OPTION_STRING,         "name of keyboard mapped to keyboard #3" },
	{ SDLOPTION_KEYBINDEX "4",               SDLOPTVAL_AUTO, OPTION_STRING,         "name of keyboard mapped to keyboard #4" },
	{ SDLOPTION_KEYBINDEX "5",               SDLOPTVAL_AUTO, OPTION_STRING,         "name of keyboard mapped to keyboard #5" },
	{ SDLOPTION_KEYBINDEX "6",               SDLOPTVAL_AUTO, OPTION_STRING,         "name of keyboard mapped to keyboard #6" },
	{ SDLOPTION_KEYBINDEX "7",               SDLOPTVAL_AUTO, OPTION_STRING,         "name of keyboard mapped to keyboard #7" },
	{ SDLOPTION_KEYBINDEX "8",               SDLOPTVAL_AUTO, OPTION_STRING,         "name of keyboard mapped to keyboard #8" },
#endif
	// SDL low level driver options
	{ NULL,                                  NULL,   OPTION_HEADER,     "SDL LOWLEVEL DRIVER OPTIONS" },
	{ SDLOPTION_VIDEODRIVER ";vd",           SDLOPTVAL_AUTO,  OPTION_STRING,        "sdl video driver to use ('x11', 'directfb', ... or 'auto' for SDL default" },
#if (SDLMAME_SDL2)
	{ SDLOPTION_RENDERDRIVER ";rd",          SDLOPTVAL_AUTO,  OPTION_STRING,        "sdl render driver to use ('software', 'opengl', 'directfb' ... or 'auto' for SDL default" },
#endif
	{ SDLOPTION_AUDIODRIVER ";ad",           SDLOPTVAL_AUTO,  OPTION_STRING,        "sdl audio driver to use ('alsa', 'arts', ... or 'auto' for SDL default" },
#if USE_OPENGL
	{ SDLOPTION_GL_LIB,                      SDLOPTVAL_GLLIB, OPTION_STRING,        "alternative libGL.so to use; 'auto' for system default" },
#endif

	// End of list
	{ NULL }
};

//============================================================
//  OS2 specific
//============================================================

#ifdef SDLMAME_OS2
void MorphToPM()
{
	PPIB pib;
	PTIB tib;

	DosGetInfoBlocks(&tib, &pib);

	// Change flag from VIO to PM:
	if (pib->pib_ultype==2) pib->pib_ultype = 3;
}
#endif

//============================================================
//  sdl_options
//============================================================

sdl_options::sdl_options()
: osd_options()
{
	astring ini_path(INI_PATH);
	add_entries(sdl_options::s_option_entries);
	ini_path.replace(0, "APP_NAME", emulator_info::get_appname_lower());
	set_default_value(SDLOPTION_INIPATH, ini_path.cstr());
}

//============================================================
//  main
//============================================================

// we do some special sauce on Win32...

#if defined(SDLMAME_WIN32)
/* gee */
extern "C" DECLSPEC void SDLCALL SDL_SetModuleHandle(void *hInst);
#endif

// translated to utf8_main
int main(int argc, char *argv[])
{
	int res = 0;

#if defined(SDLMAME_WIN32)
#if !(SDLMAME_SDL2)
	/* Load SDL dynamic link library */
	if ( SDL_Init(SDL_INIT_NOPARACHUTE) < 0 ) {
		fprintf(stderr, "WinMain() error: %s", SDL_GetError());
		return(FALSE);
	}
	SDL_SetModuleHandle(GetModuleHandle(NULL));
#endif
#endif

	// disable I/O buffering
	setvbuf(stdout, (char *) NULL, _IONBF, 0);
	setvbuf(stderr, (char *) NULL, _IONBF, 0);

	// FIXME: this should be done differently

	#ifdef SDLMAME_UNIX
	sdl_entered_debugger = 0;
	#if (!defined(SDLMAME_MACOSX)) && (!defined(SDLMAME_HAIKU)) && (!defined(SDLMAME_EMSCRIPTEN))
	FcInit();
	#endif
	#endif

	#ifdef SDLMAME_OS2
	MorphToPM();
	#endif

	#ifdef SDLMAME_EMSCRIPTEN
	EM_ASM("SDL.defaults.copyOnLock = false; SDL.defaults.discardOnLock = true;");
	#endif

#if defined(SDLMAME_X11) && (SDL_MAJOR_VERSION == 1) && (SDL_MINOR_VERSION == 2)
	if (SDL_Linked_Version()->patch < 10)
	/* workaround for SDL choosing a 32-bit ARGB visual */
	{
		Display *display;
		if ((display = XOpenDisplay(NULL)) && (DefaultDepth(display, DefaultScreen(display)) >= 24))
		{
			XVisualInfo vi;
			char buf[130];
			if (XMatchVisualInfo(display, DefaultScreen(display), 24, TrueColor, &vi)) {
				snprintf(buf, sizeof(buf), "0x%lx", vi.visualid);
				osd_setenv(SDLENV_VISUALID, buf, 0);
			}
		}
		if (display)
			XCloseDisplay(display);
	}
#endif

	{
		sdl_options options;
		sdl_osd_interface osd(options);
		osd.register_options();
		cli_frontend frontend(options, osd);
		res = frontend.execute(argc, argv);
	}

#ifdef MALLOC_DEBUG
	{
		void check_unfreed_mem(void);
		check_unfreed_mem();
	}
#endif

	#ifdef SDLMAME_UNIX
	#if (!defined(SDLMAME_MACOSX)) && (!defined(SDLMAME_HAIKU)) && (!defined(SDLMAME_EMSCRIPTEN))
	if (!sdl_entered_debugger)
	{
		FcFini();
	}
	#endif
	#endif

	exit(res);
}



//============================================================
//  output_oslog
//============================================================

static void output_oslog(running_machine &machine, const char *buffer)
{
	fputs(buffer, stderr);
}



//============================================================
//  constructor
//============================================================

sdl_osd_interface::sdl_osd_interface(sdl_options &options)
: osd_common_t(options), m_options(options)
{
	m_watchdog = NULL;
}


//============================================================
//  destructor
//============================================================

sdl_osd_interface::~sdl_osd_interface()
{
}


//============================================================
//  osd_exit
//============================================================

void sdl_osd_interface::osd_exit()
{
	osd_common_t::osd_exit();

	if (!SDLMAME_INIT_IN_WORKER_THREAD)
	{
		/* FixMe: Bug in SDL2.0, Quitting joystick will cause SIGSEGV */
#if SDLMAME_SDL2
		SDL_QuitSubSystem(SDL_INIT_TIMER| SDL_INIT_VIDEO /*| SDL_INIT_JOYSTICK */);
#else
		SDL_Quit();
#endif
	}
}

//============================================================
//  defines_verbose
//============================================================

#define MAC_EXPAND_STR(_m) #_m
#define MACRO_VERBOSE(_mac) \
	do { \
		if (strcmp(MAC_EXPAND_STR(_mac), #_mac) != 0) \
			osd_printf_verbose("%s=%s ", #_mac, MAC_EXPAND_STR(_mac)); \
	} while (0)

#define _SDL_VER #SDL_MAJOR_VERSION "." #SDL_MINOR_VERSION "." #SDL_PATCHLEVEL

static void defines_verbose(void)
{
	osd_printf_verbose("Build version:      %s\n", build_version);
	osd_printf_verbose("Build architecure:  ");
	MACRO_VERBOSE(SDLMAME_ARCH);
	osd_printf_verbose("\n");
	osd_printf_verbose("Build defines 1:    ");
	MACRO_VERBOSE(SDLMAME_UNIX);
	MACRO_VERBOSE(SDLMAME_X11);
	MACRO_VERBOSE(SDLMAME_WIN32);
	MACRO_VERBOSE(SDLMAME_OS2);
	MACRO_VERBOSE(SDLMAME_MACOSX);
	MACRO_VERBOSE(SDLMAME_DARWIN);
	MACRO_VERBOSE(SDLMAME_LINUX);
	MACRO_VERBOSE(SDLMAME_SOLARIS);
	MACRO_VERBOSE(SDLMAME_NOASM);
	MACRO_VERBOSE(SDLMAME_IRIX);
	MACRO_VERBOSE(SDLMAME_BSD);
	osd_printf_verbose("\n");
	osd_printf_verbose("Build defines 1:    ");
	MACRO_VERBOSE(LSB_FIRST);
	MACRO_VERBOSE(PTR64);
	MACRO_VERBOSE(MAME_DEBUG);
	MACRO_VERBOSE(NO_DEBUGBER);
	MACRO_VERBOSE(BIGENDIAN);
	MACRO_VERBOSE(CPP_COMPILE);
	MACRO_VERBOSE(DISTRO);
	MACRO_VERBOSE(SYNC_IMPLEMENTATION);
	osd_printf_verbose("\n");
	osd_printf_verbose("SDL/OpenGL defines: ");
	osd_printf_verbose("SDL_COMPILEDVERSION=%d ", SDL_COMPILEDVERSION);
	MACRO_VERBOSE(USE_OPENGL);
	MACRO_VERBOSE(USE_DISPATCH_GL);
	osd_printf_verbose("\n");
	osd_printf_verbose("Compiler defines A: ");
	MACRO_VERBOSE(__GNUC__);
	MACRO_VERBOSE(__GNUC_MINOR__);
	MACRO_VERBOSE(__GNUC_PATCHLEVEL__);
	MACRO_VERBOSE(__VERSION__);
	osd_printf_verbose("\n");
	osd_printf_verbose("Compiler defines B: ");
	MACRO_VERBOSE(__amd64__);
	MACRO_VERBOSE(__x86_64__);
	MACRO_VERBOSE(__unix__);
	MACRO_VERBOSE(__i386__);
	MACRO_VERBOSE(__ppc__);
	MACRO_VERBOSE(__ppc64__);
	osd_printf_verbose("\n");
	osd_printf_verbose("Compiler defines C: ");
	MACRO_VERBOSE(_FORTIFY_SOURCE);
	MACRO_VERBOSE(__USE_FORTIFY_LEVEL);
	osd_printf_verbose("\n");
}

//============================================================
//  osd_sdl_info
//============================================================

static void osd_sdl_info(void)
{
#if SDLMAME_SDL2
	int i, num = SDL_GetNumVideoDrivers();

	osd_printf_verbose("Available videodrivers: ");
	for (i=0;i<num;i++)
	{
		const char *name = SDL_GetVideoDriver(i);
		osd_printf_verbose("%s ", name);
	}
	osd_printf_verbose("\n");
	osd_printf_verbose("Current Videodriver: %s\n", SDL_GetCurrentVideoDriver());
	num = SDL_GetNumVideoDisplays();
	for (i=0;i<num;i++)
	{
		SDL_DisplayMode mode;
		int j;

		osd_printf_verbose("\tDisplay #%d\n", i);
		if (SDL_GetDesktopDisplayMode(i, &mode))
			osd_printf_verbose("\t\tDesktop Mode:         %dx%d-%d@%d\n", mode.w, mode.h, SDL_BITSPERPIXEL(mode.format), mode.refresh_rate);
		if (SDL_GetCurrentDisplayMode(i, &mode))
			osd_printf_verbose("\t\tCurrent Display Mode: %dx%d-%d@%d\n", mode.w, mode.h, SDL_BITSPERPIXEL(mode.format), mode.refresh_rate);
		osd_printf_verbose("\t\tRenderdrivers:\n");
		for (j=0; j<SDL_GetNumRenderDrivers(); j++)
		{
			SDL_RendererInfo info;
			SDL_GetRenderDriverInfo(j, &info);
			osd_printf_verbose("\t\t\t%10s (%dx%d)\n", info.name, info.max_texture_width, info.max_texture_height);
		}
	}

	osd_printf_verbose("Available audio drivers: \n");
	num = SDL_GetNumAudioDrivers();
	for (i=0;i<num;i++)
	{
		osd_printf_verbose("\t%-20s\n", SDL_GetAudioDriver(i));
	}

#endif
}


//============================================================
//  video_register
//============================================================

void sdl_osd_interface::video_register()
{
	video_options_add("soft", NULL);
	video_options_add("opengl", NULL);
	//video_options_add("auto", NULL); // making d3d video default one
}

//============================================================
//  init
//============================================================


void sdl_osd_interface::init(running_machine &machine)
{
	// call our parent
	osd_common_t::init(machine);

	const char *stemp;

	// determine if we are benchmarking, and adjust options appropriately
	int bench = options().bench();
	astring error_string;
	if (bench > 0)
	{
		options().set_value(OPTION_THROTTLE, false, OPTION_PRIORITY_MAXIMUM, error_string);
		options().set_value(OSDOPTION_SOUND, "none", OPTION_PRIORITY_MAXIMUM, error_string);
		options().set_value(OSDOPTION_VIDEO, "none", OPTION_PRIORITY_MAXIMUM, error_string);
		options().set_value(OPTION_SECONDS_TO_RUN, bench, OPTION_PRIORITY_MAXIMUM, error_string);
		assert(!error_string);
	}

	// Some driver options - must be before audio init!
	stemp = options().audio_driver();
	if (stemp != NULL && strcmp(stemp, SDLOPTVAL_AUTO) != 0)
	{
		osd_printf_verbose("Setting SDL audiodriver '%s' ...\n", stemp);
		osd_setenv(SDLENV_AUDIODRIVER, stemp, 1);
	}

	stemp = options().video_driver();
	if (stemp != NULL && strcmp(stemp, SDLOPTVAL_AUTO) != 0)
	{
		osd_printf_verbose("Setting SDL videodriver '%s' ...\n", stemp);
		osd_setenv(SDLENV_VIDEODRIVER, stemp, 1);
	}

#if (SDLMAME_SDL2)
		stemp = options().render_driver();
		if (stemp != NULL && strcmp(stemp, SDLOPTVAL_AUTO) != 0)
		{
			osd_printf_verbose("Setting SDL renderdriver '%s' ...\n", stemp);
			//osd_setenv(SDLENV_RENDERDRIVER, stemp, 1);
			SDL_SetHint(SDL_HINT_RENDER_DRIVER, stemp);
		}
#endif

	/* Set the SDL environment variable for drivers wanting to load the
	 * lib at startup.
	 */
#if USE_OPENGL
	/* FIXME: move lib loading code from drawogl.c here */

	stemp = options().gl_lib();
	if (stemp != NULL && strcmp(stemp, SDLOPTVAL_AUTO) != 0)
	{
		osd_setenv("SDL_VIDEO_GL_DRIVER", stemp, 1);
		osd_printf_verbose("Setting SDL_VIDEO_GL_DRIVER = '%s' ...\n", stemp);
	}
#endif

	/* get number of processors */
	stemp = options().numprocessors();

	osd_num_processors = 0;

	if (strcmp(stemp, "auto") != 0)
	{
		osd_num_processors = atoi(stemp);
		if (osd_num_processors < 1)
		{
			osd_printf_warning("numprocessors < 1 doesn't make much sense. Assuming auto ...\n");
			osd_num_processors = 0;
		}
	}

	/* Initialize SDL */

	if (!SDLMAME_INIT_IN_WORKER_THREAD)
	{
#if (SDLMAME_SDL2)
		if (SDL_InitSubSystem(SDL_INIT_TIMER| SDL_INIT_VIDEO| SDL_INIT_JOYSTICK|SDL_INIT_NOPARACHUTE)) {
#else
		if (SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO| SDL_INIT_JOYSTICK|SDL_INIT_NOPARACHUTE)) {
#endif
			osd_printf_error("Could not initialize SDL %s\n", SDL_GetError());
			exit(-1);
		}
		osd_sdl_info();
	}

	defines_verbose();

	if (!SDLMAME_HAS_DEBUGGER)
		if (machine.debug_flags & DEBUG_FLAG_OSD_ENABLED)
		{
			osd_printf_error("sdlmame: -debug not supported on X11-less builds\n\n");
			osd_exit();
			exit(-1);
		}

	osd_common_t::init_subsystems();

	if (options().oslog())
		machine.add_logerror_callback(output_oslog);

	/* now setup watchdog */

	int watchdog_timeout = options().watchdog();

	if (watchdog_timeout != 0)
	{
		m_watchdog = auto_alloc(machine, watchdog);
		m_watchdog->setTimeout(watchdog_timeout);
	}

#if (SDLMAME_SDL2)
	SDL_EventState(SDL_TEXTINPUT, SDL_TRUE);
#else
	SDL_EnableUNICODE(SDL_TRUE);
#endif
}

#ifdef SDLMAME_WIN32

//============================================================
//  wstring_from_utf8
//============================================================

WCHAR *wstring_from_utf8(const char *utf8string)
{
	int char_count;
	WCHAR *result;

	// convert MAME string (UTF-8) to UTF-16
	char_count = MultiByteToWideChar(CP_UTF8, 0, utf8string, -1, NULL, 0);
	result = (WCHAR *)osd_malloc_array(char_count * sizeof(*result));
	if (result != NULL)
		MultiByteToWideChar(CP_UTF8, 0, utf8string, -1, result, char_count);

	return result;
}


//============================================================
//  utf8_from_wstring
//============================================================

char *utf8_from_wstring(const WCHAR *wstring)
{
	int char_count;
	char *result;

	// convert UTF-16 to MAME string (UTF-8)
	char_count = WideCharToMultiByte(CP_UTF8, 0, wstring, -1, NULL, 0, NULL, NULL);
	result = (char *)osd_malloc_array(char_count * sizeof(*result));
	if (result != NULL)
		WideCharToMultiByte(CP_UTF8, 0, wstring, -1, result, char_count, NULL, NULL);

	return result;
}

#endif
