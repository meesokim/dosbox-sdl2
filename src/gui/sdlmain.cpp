/*
 *  Copyright (C) 2002-2013  The DOSBox Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#ifdef WIN32
#include <signal.h>
#include <process.h>
#endif
#ifdef ANDROID
#include <android/log.h>
#endif

#include "cross.h"
#include "SDL.h"

#include "dosbox.h"
#include "video.h"
#include "mouse.h"
#include "pic.h"
#include "timer.h"
#include "setup.h"
#include "support.h"
#include "debug.h"
#include "mapper.h"
#include "vga.h"
#include "keyboard.h"
#include "cpu.h"
#include "cross.h"
#include "control.h"

#define MAPPERFILE "mapper-" VERSION ".map"
//#define DISABLE_JOYSTICK

#if C_OPENGL
#ifdef ANDROID
#include "SDL_opengles.h"
#else
#include "SDL_opengl.h"
#endif

#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

#ifndef GL_ARB_pixel_buffer_object
#define GL_ARB_pixel_buffer_object 1
#define GL_PIXEL_PACK_BUFFER_ARB           0x88EB
#define GL_PIXEL_UNPACK_BUFFER_ARB         0x88EC
#define GL_PIXEL_PACK_BUFFER_BINDING_ARB   0x88ED
#define GL_PIXEL_UNPACK_BUFFER_BINDING_ARB 0x88EF
#endif

#ifndef GL_ARB_vertex_buffer_object
#define GL_ARB_vertex_buffer_object 1
typedef void (APIENTRYP PFNGLGENBUFFERSARBPROC) (GLsizei n, GLuint *buffers);
typedef void (APIENTRYP PFNGLBINDBUFFERARBPROC) (GLenum target, GLuint buffer);
typedef void (APIENTRYP PFNGLDELETEBUFFERSARBPROC) (GLsizei n, const GLuint *buffers);
typedef void (APIENTRYP PFNGLBUFFERDATAARBPROC) (GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
typedef GLvoid* (APIENTRYP PFNGLMAPBUFFERARBPROC) (GLenum target, GLenum access);
typedef GLboolean (APIENTRYP PFNGLUNMAPBUFFERARBPROC) (GLenum target);
#endif

#ifndef ANDROID
PFNGLGENBUFFERSARBPROC glGenBuffersARB = NULL;
PFNGLBINDBUFFERARBPROC glBindBufferARB = NULL;
PFNGLDELETEBUFFERSARBPROC glDeleteBuffersARB = NULL;
PFNGLBUFFERDATAARBPROC glBufferDataARB = NULL;
PFNGLMAPBUFFERARBPROC glMapBufferARB = NULL;
PFNGLUNMAPBUFFERARBPROC glUnmapBufferARB = NULL;
#endif

#endif //C_OPENGL

#if !(ENVIRON_INCLUDED)
extern char** environ;
#endif

#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#if (HAVE_DDRAW_H)
#include <ddraw.h>
struct private_hwdata {
	LPDIRECTDRAWSURFACE3 dd_surface;
	LPDIRECTDRAWSURFACE3 dd_writebuf;
};
#endif

#define STDOUT_FILE	TEXT("stdout.txt")
#define STDERR_FILE	TEXT("stderr.txt")
#define DEFAULT_CONFIG_FILE "/dosbox.conf"
#elif defined(MACOSX)
#define DEFAULT_CONFIG_FILE "/Library/Preferences/DOSBox Preferences"
#else /*linux freebsd*/
#define DEFAULT_CONFIG_FILE "/.dosboxrc"
#endif

#if C_SET_PRIORITY
#include <sys/resource.h>
#define PRIO_TOTAL (PRIO_MAX-PRIO_MIN)
#endif

#ifdef OS2
#define INCL_DOS
#define INCL_WIN
#include <os2.h>
#endif

enum SCREEN_TYPES	{
	SCREEN_SURFACE,
#if SDL_VERSION_ATLEAST(2,0,0)
	SCREEN_TEXTURE,
#else
	SCREEN_SURFACE_DDRAW,
	SCREEN_OVERLAY,
#endif
	SCREEN_OPENGL
};

enum PRIORITY_LEVELS {
	PRIORITY_LEVEL_PAUSE,
	PRIORITY_LEVEL_LOWEST,
	PRIORITY_LEVEL_LOWER,
	PRIORITY_LEVEL_NORMAL,
	PRIORITY_LEVEL_HIGHER,
	PRIORITY_LEVEL_HIGHEST
};


struct SDL_Block {
	bool inited;
	bool active;							//If this isn't set don't draw
	bool updating;
#if SDL_VERSION_ATLEAST(2,0,0)
	bool update_display_contents;
	bool update_window;
	int window_desired_width, window_desired_height;
#endif
	struct {
		Bit32u width;
		Bit32u height;
#if 0
		Bit32u bpp; // Is that ever used?
#endif
		Bitu flags;
		double scalex,scaley;
		GFX_CallBack_t callback;
	} draw;
	bool wait_on_error;
	struct {
		struct {
			Bit16u width, height;
			bool fixed;
#if SDL_VERSION_ATLEAST(2,0,0)
			bool display_res;
#endif
		} full;
		struct {
			Bit16u width, height;
		} window;
		Bit8u bpp;
#if SDL_VERSION_ATLEAST(2,0,0)
		Bit32u sdl2pixelFormat;
#endif
		bool fullscreen;
		bool lazy_fullscreen;
		bool lazy_fullscreen_req;
		bool vsync;
		SCREEN_TYPES type;
		SCREEN_TYPES want_type;
	} desktop;
#if C_OPENGL
	struct {
#if SDL_VERSION_ATLEAST(2,0,0)
		SDL_GLContext context;
#endif
		Bitu pitch;
		void * framebuf;
		GLuint buffer;
		GLuint texture;
#ifdef ANDROID // OpenGL ES
		GLfloat vertCoords[8];
		GLfloat texCoords[8];
#else // OpenGL (not ES)
		GLuint displaylist;
#endif
		GLint max_texsize;
		bool bilinear;
		bool packed_pixel;
		bool paletted_texture;
		bool pixel_buffer_object;
	} opengl;
#endif	// C_OPENGL
#if !SDL_VERSION_ATLEAST(2,0,0)
	struct {
		SDL_Surface * surface;
#if (HAVE_DDRAW_H) && defined(WIN32)
		RECT rect;
#endif // Windows
	} blit;
#endif // Not SDL v2.0
	struct {
		PRIORITY_LEVELS focus;
		PRIORITY_LEVELS nofocus;
	} priority;
	SDL_Rect clip;
	SDL_Surface * surface;
#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_Window * window;
	SDL_Renderer * renderer;
	const char * rendererDriver;
	int displayNumber;
	struct {
		SDL_Texture * texture;
		SDL_PixelFormat * pixelFormat;
	} texture;
#else
	SDL_Overlay * overlay;
#endif
	SDL_cond *cond;
	struct {
		bool autolock;
		bool autoenable;
		bool requestlock;
		bool locked;
		Bitu sensitivity;
#ifdef ANDROID
		SDL_FingerID leftMouseFingerID, rightMouseFingerID,
		             middleMouseFingerID, mouseMotionFingerID,
		             escKeyFingerID;
		bool isLeftMouseFingerUsed, isRightMouseFingerUsed,
		     isMiddleMouseFingerUsed, isEscKeyFingerUsed;
#endif
	} mouse;
	SDL_Rect updateRects[1024];
	Bitu num_joysticks;
#if defined (WIN32)
	bool using_windib;
#endif
	// state of alt-keys for certain special handlings
	Bit16u laltstate;
	Bit16u raltstate;
};

static SDL_Block sdl;

extern const char* RunningProgram;
extern bool CPU_CycleAutoAdjust;
//Globals for keyboard initialisation
bool startup_state_numlock=false;
bool startup_state_capslock=false;

void GFX_SetTitle(Bit32s cycles,Bits frameskip,bool paused){
	char title[200]={0};
	static Bit32s internal_cycles=0;
	static Bits internal_frameskip=0;
	if(cycles != -1) internal_cycles = cycles;
	if(frameskip != -1) internal_frameskip = frameskip;
	if(CPU_CycleAutoAdjust) {
		sprintf(title,"DOSBox %s, CPU speed: max %3d%% cycles, Frameskip %2d, Program: %8s",VERSION,internal_cycles,internal_frameskip,RunningProgram);
	} else {
		sprintf(title,"DOSBox %s, CPU speed: %8d cycles, Frameskip %2d, Program: %8s",VERSION,internal_cycles,internal_frameskip,RunningProgram);
	}

	if(paused) strcat(title," PAUSED");
#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_SetWindowTitle(sdl.window,title); // VERSION is gone...
#else
	SDL_WM_SetCaption(title,VERSION);
#endif
}

static unsigned char logo[32*32*4]= {
#include "dosbox_logo.h"
};
static void GFX_SetIcon() {
#if !defined(MACOSX)
	/* Set Icon (must be done before any sdl_setvideomode call) */
	/* But don't set it on OS X, as we use a nicer external icon there. */
	/* Made into a separate call, so it can be called again when we restart the graphics output on win32 */
#if WORDS_BIGENDIAN
	SDL_Surface* logos= SDL_CreateRGBSurfaceFrom((void*)logo,32,32,32,128,0xff000000,0x00ff0000,0x0000ff00,0);
#else
	SDL_Surface* logos= SDL_CreateRGBSurfaceFrom((void*)logo,32,32,32,128,0x000000ff,0x0000ff00,0x00ff0000,0);
#endif // WORDS_BIGENDIAN

#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_SetWindowIcon(sdl.window, logos);
#else
	SDL_WM_SetIcon(logos,NULL);
#endif // SDL_VERSION_ATLEAST(2,0,0)
#endif // !defined(MACOSX)
}


static void KillSwitch(bool pressed) {
	if (!pressed)
		return;
	throw 1;
}

static void PauseDOSBox(bool pressed) {
	if (!pressed)
		return;
	GFX_SetTitle(-1,-1,true);
	bool paused = true;
	KEYBOARD_ClrBuffer();
	SDL_Delay(500);
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		// flush event queue.
	}
	/* NOTE: This is one of the few places where we use SDL key codes
	with SDL 2.0, rather than scan codes. Is that the correct behavior? */
	while (paused) {
		SDL_WaitEvent(&event);    // since we're not polling, cpu usage drops to 0.
		switch (event.type) {

			case SDL_QUIT: KillSwitch(true); break;
#if SDL_VERSION_ATLEAST(2,0,0)
			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_RESTORED) {
					// We may need to re-create a texture and more
					GFX_ResetScreen();
				}
				break;
#endif
			case SDL_KEYDOWN:   // Must use Pause/Break Key to resume.
			case SDL_KEYUP:
			if(event.key.keysym.sym == SDLK_PAUSE) {

				paused = false;
				GFX_SetTitle(-1,-1,false);
				break;
			}
#if defined (MACOSX)
			if (event.key.keysym.sym == SDLK_q && (event.key.keysym.mod == KMOD_RMETA || event.key.keysym.mod == KMOD_LMETA) ) {
				/* On macs, all aps exit when pressing cmd-q */
				KillSwitch(true);
				break;
			} 
#endif
		}
	}
}

#if !SDL_VERSION_ATLEAST(2,0,0)
#if defined (WIN32)
bool GFX_SDLUsingWinDIB(void) {
	return sdl.using_windib;
}
#endif
#endif

/* Reset the screen with current values in the sdl structure */
Bitu GFX_GetBestMode(Bitu flags) {
#if !SDL_VERSION_ATLEAST(2,0,0)
 	Bitu testbpp,gotbpp;
#endif
	/* For simplicity, with SDL 2.0 we accept
	the desktop's color depth only for now */
	switch (sdl.desktop.want_type) {
	case SCREEN_SURFACE:
#if SDL_VERSION_ATLEAST(2,0,0)
	case SCREEN_TEXTURE:
#endif
check_surface:
		flags &= ~GFX_LOVE_8;		//Disable love for 8bpp modes
#if !SDL_VERSION_ATLEAST(2,0,0)
		/* Check if we can satisfy the depth it loves */
		if (flags & GFX_LOVE_8) testbpp=8;
		else if (flags & GFX_LOVE_15) testbpp=15;
		else if (flags & GFX_LOVE_16) testbpp=16;
		else if (flags & GFX_LOVE_32) testbpp=32;
		else testbpp=0;
#if (HAVE_DDRAW_H) && defined(WIN32)
check_gotbpp:
#endif
		if (sdl.desktop.fullscreen) gotbpp=SDL_VideoModeOK(640,480,testbpp,SDL_FULLSCREEN|SDL_HWSURFACE|SDL_HWPALETTE);
		else gotbpp=sdl.desktop.bpp;
#endif	// !SDL_VERSION_ATLEAST(2,0,0)
		/* If we can't get our favorite mode check for another working one */
#if SDL_VERSION_ATLEAST(2,0,0)
		switch (sdl.desktop.bpp)
#else
		switch (gotbpp)
#endif
		{
		case 8:
			if (flags & GFX_CAN_8) flags&=~(GFX_CAN_15|GFX_CAN_16|GFX_CAN_32);
			break;
		case 15:
			if (flags & GFX_CAN_15) flags&=~(GFX_CAN_8|GFX_CAN_16|GFX_CAN_32);
			break;
		case 16:
			if (flags & GFX_CAN_16) flags&=~(GFX_CAN_8|GFX_CAN_15|GFX_CAN_32);
			break;
		case 24:
		case 32:
			if (flags & GFX_CAN_32) flags&=~(GFX_CAN_8|GFX_CAN_15|GFX_CAN_16);
			break;
		}
#if SDL_VERSION_ATLEAST(2,0,0)
		if (sdl.desktop.want_type == SCREEN_TEXTURE)
			flags |= GFX_SCALING; // We want texture...
		else	// Or we want/FORCE surface (e.g. for most scalers)
#endif
			flags |= GFX_CAN_RANDOM;
		break;
#if !SDL_VERSION_ATLEAST(2,0,0)
#if (HAVE_DDRAW_H) && defined(WIN32)
	case SCREEN_SURFACE_DDRAW:
		if (!(flags&(GFX_CAN_15|GFX_CAN_16|GFX_CAN_32))) goto check_surface;
		if (flags & GFX_LOVE_15) testbpp=15;
		else if (flags & GFX_LOVE_16) testbpp=16;
		else if (flags & GFX_LOVE_32) testbpp=32;
		else testbpp=0;
		flags|=GFX_SCALING;
		goto check_gotbpp;
#endif
	case SCREEN_OVERLAY:
		if (flags & GFX_RGBONLY || !(flags&GFX_CAN_32)) goto check_surface;
		flags|=GFX_SCALING;
		flags&=~(GFX_CAN_8|GFX_CAN_15|GFX_CAN_16);
		break;
#endif	// !SDL_VERSION_ATLEAST(2,0,0)
#if C_OPENGL
	case SCREEN_OPENGL:
#ifdef ANDROID
		if (!(flags&GFX_CAN_32)) goto check_surface; // RGBA is used on Android (OpenGL ES v1.1)
#else
		if (flags & GFX_RGBONLY || !(flags&GFX_CAN_32)) goto check_surface; // BGRA otherwise
#endif
		flags|=GFX_SCALING;
		flags&=~(GFX_CAN_8|GFX_CAN_15|GFX_CAN_16);
		break;
#endif
	default:
		goto check_surface;
		break;
	}
	return flags;
}


void GFX_ResetScreen(void) {
	GFX_Stop();
	if (sdl.draw.callback)
		(sdl.draw.callback)( GFX_CallBackReset );
	GFX_Start();
	CPU_Reset_AutoAdjust();
}

void GFX_ForceFullscreenExit(void) {
	if (sdl.desktop.lazy_fullscreen) {
//		sdl.desktop.lazy_fullscreen_req=true;
		LOG_MSG("GFX LF: invalid screen change");
	} else {
		sdl.desktop.fullscreen=false;
		GFX_ResetScreen();
	}
}

static int int_log2 (int val) {
    int log = 0;
    while ((val >>= 1) != 0)
	log++;
    return log;
}

#if SDL_VERSION_ATLEAST(2,0,0)

static SDL_Window * GFX_SetSDLWindowMode(Bit16u width, Bit16u height, bool fullscreen, SCREEN_TYPES screenType) {
//static SDL_Window * GFX_SetSDLWindowMode(Bit16u width, Bit16u height, bool fullscreen, bool glwindow) {
	static SCREEN_TYPES lastType = SCREEN_SURFACE; 
	if (sdl.renderer) {
		SDL_DestroyRenderer(sdl.renderer);
		sdl.renderer=0;
	}
	if (sdl.texture.pixelFormat) {
		SDL_FreeFormat(sdl.texture.pixelFormat);
		sdl.texture.pixelFormat = 0;
	}
	if (sdl.texture.texture) {
		SDL_DestroyTexture(sdl.texture.texture);
		sdl.texture.texture=0;
	}
#if C_OPENGL
	/* Should fix this border-related fullscreen bug on some platforms(?)
	http://vogons.zetafleet.com/viewtopic.php?t=27487                  */
	if (sdl.opengl.context) {
		SDL_GL_DeleteContext(sdl.opengl.context);
		sdl.opengl.context=0;
	}
#endif
	sdl.window_desired_width = width;
	sdl.window_desired_height = height;
	int currWidth, currHeight;
	if (sdl.window) {
		//SDL_GetWindowSize(sdl.window, &currWidth, &currHeight);
		if (!sdl.update_window) {
			SDL_GetWindowSize(sdl.window, &currWidth, &currHeight);
			sdl.update_display_contents = ((width == currWidth) && (height == currHeight));
			return sdl.window;
		}
	}
	/* If we change screen type, recreate the window. Furthermore, if
	 * it is our very first time then we simply create a new window.
	 */
	if (!sdl.window
	    || (lastType != screenType)
//	    || (currWidth != width) || (currHeight != height)
//	    || (glwindow != (0 != (SDL_GetWindowFlags(sdl.window) & SDL_WINDOW_OPENGL)))
//	    || (fullscreen && (0 == (SDL_GetWindowFlags(sdl.window) & SDL_WINDOW_FULLSCREEN)))
//	    || (fullscreen != (SDL_WINDOW_FULLSCREEN == (SDL_GetWindowFlags(sdl.window) & SDL_WINDOW_FULLSCREEN)))
//	    || (fullscreen && ((width != currWidth) || (height != currHeight)))
	) {
		lastType = screenType;
		if (sdl.window) {
			SDL_DestroyWindow(sdl.window);
		}
		sdl.window = SDL_CreateWindow("",
		                 SDL_WINDOWPOS_UNDEFINED_DISPLAY(sdl.displayNumber),
		                 SDL_WINDOWPOS_UNDEFINED_DISPLAY(sdl.displayNumber),
		                 width, height,
		                 (fullscreen ? (sdl.desktop.full.display_res ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_FULLSCREEN) : 0)
//		                 | (glwindow ? SDL_WINDOW_OPENGL : 0) | SDL_WINDOW_SHOWN);
		                 | SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
		if (sdl.window)
			GFX_SetTitle(-1,-1,false); //refresh title.
		SDL_GetWindowSize(sdl.window, &currWidth, &currHeight);
		sdl.update_display_contents = ((width == currWidth) && (height == currHeight));
		return sdl.window;
	}
	/* Fullscreen mode switching has its limits, and is also problematic on
	 * some window managers. For now, the following may work up to some
	 * level. On X11, SDL_VIDEO_X11_LEGACY_FULLSCREEN=1 can also help,
	 * although it has its own issues.
	 * Suggestion: Use the desktop res if possible, with output=surface
	 * if one is not interested in scaling.
	 */
	if (fullscreen) {
		SDL_DisplayMode displayMode;
		SDL_GetWindowDisplayMode(sdl.window, &displayMode);
		displayMode.w = width;
		displayMode.h = height;
		SDL_SetWindowDisplayMode(sdl.window, &displayMode);

		SDL_SetWindowFullscreen(sdl.window, SDL_WINDOW_FULLSCREEN);
	} else {
		SDL_SetWindowFullscreen(sdl.window, 0);

		SDL_SetWindowSize(sdl.window, width, height);
	}
	// Maybe some requested fullscreen resolution is unsupported?
	SDL_GetWindowSize(sdl.window, &currWidth, &currHeight);
	sdl.update_display_contents = ((width == currWidth) && (height == currHeight));
	return sdl.window;
}

// Used for the mapper and more (fullscreen on Android, windowed otherwise)
SDL_Window * GFX_SetSDLSurfaceWindow(Bit16u width, Bit16u height) {
#ifdef ANDROID
	return GFX_SetSDLWindowMode(width, height, true, SCREEN_SURFACE);
#else
	return GFX_SetSDLWindowMode(width, height, false, SCREEN_SURFACE);
#endif
}

// Currently used for an initial test here
static SDL_Window * GFX_SetSDLOpenGLWindow(Bit16u width, Bit16u height) {
#ifdef ANDROID
	return GFX_SetSDLWindowMode(width, height, true, SCREEN_OPENGL);
#else
	return GFX_SetSDLWindowMode(width, height, false, SCREEN_OPENGL);
#endif
}

static SDL_Window * GFX_SetupWindowScaled(SCREEN_TYPES screenType) {
	Bit16u fixedWidth;
	Bit16u fixedHeight;

	if (sdl.desktop.fullscreen) {
		fixedWidth = sdl.desktop.full.fixed ? sdl.desktop.full.width : 0;
		fixedHeight = sdl.desktop.full.fixed ? sdl.desktop.full.height : 0;
	} else {
		fixedWidth = sdl.desktop.window.width;
		fixedHeight = sdl.desktop.window.height;
	}
	if (fixedWidth && fixedHeight) {
		double ratio_w=(double)fixedWidth/(sdl.draw.width*sdl.draw.scalex);
		double ratio_h=(double)fixedHeight/(sdl.draw.height*sdl.draw.scaley);
		if ( ratio_w < ratio_h) {
			sdl.clip.w=fixedWidth;
			sdl.clip.h=(Bit16u)(sdl.draw.height*sdl.draw.scaley*ratio_w);
		} else {
			sdl.clip.w=(Bit16u)(sdl.draw.width*sdl.draw.scalex*ratio_h);
			sdl.clip.h=(Bit16u)fixedHeight;
		}
		if (sdl.desktop.fullscreen) {
			sdl.window = GFX_SetSDLWindowMode(fixedWidth, fixedHeight, sdl.desktop.fullscreen, screenType);
			sdl.clip.x=(Sint16)((fixedWidth-sdl.clip.w)/2);
#ifdef ANDROID
			/* Portrait orientation and on-screen keyboards
			are commonly found on that platform          */
			sdl.clip.y=0;
#else
			sdl.clip.y=(Sint16)((fixedHeight-sdl.clip.h)/2);
#endif
		} else {
			sdl.window = GFX_SetSDLWindowMode(sdl.clip.w, sdl.clip.h, sdl.desktop.fullscreen, screenType);
			sdl.clip.x = 0;
			sdl.clip.y = 0;
		}
		return sdl.window;
	} else {
		sdl.clip.x=0;sdl.clip.y=0;
		sdl.clip.w=(Bit16u)(sdl.draw.width*sdl.draw.scalex);
		sdl.clip.h=(Bit16u)(sdl.draw.height*sdl.draw.scaley);
		sdl.window = GFX_SetSDLWindowMode(sdl.clip.w, sdl.clip.h, sdl.desktop.fullscreen, screenType);
		return sdl.window;
	}
}

#else
static SDL_Surface * GFX_SetupSurfaceScaled(Bit32u sdl_flags, Bit32u bpp) {
	Bit16u fixedWidth;
	Bit16u fixedHeight;

	if (sdl.desktop.fullscreen) {
		fixedWidth = sdl.desktop.full.fixed ? sdl.desktop.full.width : 0;
		fixedHeight = sdl.desktop.full.fixed ? sdl.desktop.full.height : 0;
		sdl_flags |= SDL_FULLSCREEN|SDL_HWSURFACE;
	} else {
		fixedWidth = sdl.desktop.window.width;
		fixedHeight = sdl.desktop.window.height;
		sdl_flags |= SDL_HWSURFACE;
	}
	if (fixedWidth && fixedHeight) {
		double ratio_w=(double)fixedWidth/(sdl.draw.width*sdl.draw.scalex);
		double ratio_h=(double)fixedHeight/(sdl.draw.height*sdl.draw.scaley);
		if ( ratio_w < ratio_h) {
			sdl.clip.w=fixedWidth;
			sdl.clip.h=(Bit16u)(sdl.draw.height*sdl.draw.scaley*ratio_w);
		} else {
			sdl.clip.w=(Bit16u)(sdl.draw.width*sdl.draw.scalex*ratio_h);
			sdl.clip.h=(Bit16u)fixedHeight;
		}
		if (sdl.desktop.fullscreen)
			sdl.surface = SDL_SetVideoMode(fixedWidth,fixedHeight,bpp,sdl_flags);
		else
			sdl.surface = SDL_SetVideoMode(sdl.clip.w,sdl.clip.h,bpp,sdl_flags);
		if (sdl.surface && sdl.surface->flags & SDL_FULLSCREEN) {
			sdl.clip.x=(Sint16)((sdl.surface->w-sdl.clip.w)/2);
			sdl.clip.y=(Sint16)((sdl.surface->h-sdl.clip.h)/2);
		} else {
			sdl.clip.x = 0;
			sdl.clip.y = 0;
		}
		return sdl.surface;
	} else {
		sdl.clip.x=0;sdl.clip.y=0;
		sdl.clip.w=(Bit16u)(sdl.draw.width*sdl.draw.scalex);
		sdl.clip.h=(Bit16u)(sdl.draw.height*sdl.draw.scaley);
		sdl.surface=SDL_SetVideoMode(sdl.clip.w,sdl.clip.h,bpp,sdl_flags);
		return sdl.surface;
	}
}
#endif // SDL_VERSION_ATLEAST(2,0,0)

#if 0 // NOTE: Do we need this? Never used and can't be used as-is with SDL 2.0
void GFX_TearDown(void) {
	if (sdl.updating)
		GFX_EndUpdate( 0 );

	if (sdl.blit.surface) {
		SDL_FreeSurface(sdl.blit.surface);
		sdl.blit.surface=0;
	}
}
#endif

Bitu GFX_SetSize(Bitu width,Bitu height,Bitu flags,double scalex,double scaley,GFX_CallBack_t callback) {
	if (sdl.updating)
		GFX_EndUpdate( 0 );

	sdl.draw.width=width;
	sdl.draw.height=height;
	sdl.draw.callback=callback;
	sdl.draw.scalex=scalex;
	sdl.draw.scaley=scaley;

	Bitu retFlags = 0;
#if !SDL_VERSION_ATLEAST(2,0,0)
	Bitu bpp=0;
	if (sdl.blit.surface) {
		SDL_FreeSurface(sdl.blit.surface);
		sdl.blit.surface=0;
	}
#endif
	switch (sdl.desktop.want_type) {
	case SCREEN_SURFACE:
dosurface:
#if !SDL_VERSION_ATLEAST(2,0,0)
		if (flags & GFX_CAN_8) bpp=8;
		if (flags & GFX_CAN_15) bpp=15;
		if (flags & GFX_CAN_16) bpp=16;
		if (flags & GFX_CAN_32) bpp=32;
#endif
		sdl.desktop.type=SCREEN_SURFACE;
		sdl.clip.w=width;
		sdl.clip.h=height;
		if (sdl.desktop.fullscreen) {
			if (sdl.desktop.full.fixed) {
				sdl.clip.x=(Sint16)((sdl.desktop.full.width-width)/2);
#ifdef ANDROID
				/* Portrait orientation and on-screen keyboards
				are commonly found on that platform          */
				sdl.clip.y=0;
#else
				sdl.clip.y=(Sint16)((sdl.desktop.full.height-height)/2);
#endif
#if SDL_VERSION_ATLEAST(2,0,0)
				sdl.window = GFX_SetSDLWindowMode(sdl.desktop.full.width,
				                                  sdl.desktop.full.height,
				                                  sdl.desktop.fullscreen, sdl.desktop.type);
				if (sdl.window == NULL)
					E_Exit("Could not set fullscreen video mode %ix%i-%i: %s",sdl.desktop.full.width,sdl.desktop.full.height,sdl.desktop.bpp,SDL_GetError());
#else
				sdl.surface=SDL_SetVideoMode(sdl.desktop.full.width,sdl.desktop.full.height,bpp,
					SDL_FULLSCREEN | ((flags & GFX_CAN_RANDOM) ? SDL_SWSURFACE : SDL_HWSURFACE) |
					(sdl.desktop.vsync ? SDL_DOUBLEBUF|SDL_ASYNCBLIT : 0) | SDL_HWPALETTE);
				if (sdl.surface == NULL) E_Exit("Could not set fullscreen video mode %ix%i-%i: %s",sdl.desktop.full.width,sdl.desktop.full.height,bpp,SDL_GetError());
#endif
			} else {
				sdl.clip.x=0;sdl.clip.y=0;
#if SDL_VERSION_ATLEAST(2,0,0)
				sdl.window = GFX_SetSDLWindowMode(width, height,
				                                  sdl.desktop.fullscreen, sdl.desktop.type);
				if (sdl.window == NULL)
					E_Exit("Could not set fullscreen video mode %ix%i-%i: %s",width,height,sdl.desktop.bpp,SDL_GetError());
#else
				sdl.surface=SDL_SetVideoMode(width,height,bpp,
					SDL_FULLSCREEN | ((flags & GFX_CAN_RANDOM) ? SDL_SWSURFACE : SDL_HWSURFACE) |
					(sdl.desktop.vsync ? SDL_DOUBLEBUF|SDL_ASYNCBLIT  : 0)|SDL_HWPALETTE);
				if (sdl.surface == NULL)
					E_Exit("Could not set fullscreen video mode %ix%i-%i: %s",width,height,bpp,SDL_GetError());
#endif
			}
		} else {
			sdl.clip.x=0;sdl.clip.y=0;
#if SDL_VERSION_ATLEAST(2,0,0)
			sdl.window = GFX_SetSDLWindowMode(width, height,
			                                  sdl.desktop.fullscreen, sdl.desktop.type);
			if (sdl.window == NULL)
				E_Exit("Could not set windowed video mode %ix%i-%i: %s",width,height,sdl.desktop.bpp,SDL_GetError());
#else
			sdl.surface=SDL_SetVideoMode(width,height,bpp,(flags & GFX_CAN_RANDOM) ? SDL_SWSURFACE : SDL_HWSURFACE);
#ifdef WIN32
			if (sdl.surface == NULL) {
				SDL_QuitSubSystem(SDL_INIT_VIDEO);
				if (!sdl.using_windib) {
					LOG_MSG("Failed to create hardware surface.\nRestarting video subsystem with windib enabled.");
					putenv("SDL_VIDEODRIVER=windib");
					sdl.using_windib=true;
				} else {
					LOG_MSG("Failed to create hardware surface.\nRestarting video subsystem with directx enabled.");
					putenv("SDL_VIDEODRIVER=directx");
					sdl.using_windib=false;
				}
				SDL_InitSubSystem(SDL_INIT_VIDEO);
				GFX_SetIcon(); //Set Icon again
				sdl.surface = SDL_SetVideoMode(width,height,bpp,SDL_HWSURFACE);
				if(sdl.surface) GFX_SetTitle(-1,-1,false); //refresh title.
			}
#endif
			if (sdl.surface == NULL)
				E_Exit("Could not set windowed video mode %ix%i-%i: %s",width,height,bpp,SDL_GetError());
#endif	// !SDL_VERSION_ATLEAST(2,0,0)
		}
#if SDL_VERSION_ATLEAST(2,0,0)
		sdl.surface = SDL_GetWindowSurface(sdl.window);
		if (sdl.surface == NULL)
				E_Exit("Could not retrieve window surface: %s",SDL_GetError());
#endif
		switch (sdl.surface->format->BitsPerPixel) {
			case 8:
				retFlags = GFX_CAN_8;
				break;
			case 15:
				retFlags = GFX_CAN_15;
				break;
			case 16:
				retFlags = GFX_CAN_16;
				break;
			case 32:
				retFlags = GFX_CAN_32;
				break;
		}
#if SDL_VERSION_ATLEAST(2,0,0)
		/* Fix a glitch with aspect=true occuring when changing between
		graphical (aspect=true) and text (aspect=false) modes        */
		SDL_FillRect(sdl.surface, NULL, SDL_MapRGB(sdl.surface->format, 0, 0, 0));
		SDL_UpdateWindowSurface(sdl.window);
#else
		if (retFlags && (sdl.surface->flags & SDL_HWSURFACE))
			retFlags |= GFX_HARDWARE;
		if (retFlags && (sdl.surface->flags & SDL_DOUBLEBUF)) {
			sdl.blit.surface=SDL_CreateRGBSurface(SDL_HWSURFACE,
				sdl.draw.width, sdl.draw.height,
				sdl.surface->format->BitsPerPixel,
				sdl.surface->format->Rmask,
				sdl.surface->format->Gmask,
				sdl.surface->format->Bmask,
			0);
			/* If this one fails be ready for some flickering... */
		}
#endif
		break;
#if SDL_VERSION_ATLEAST(2,0,0)
	case SCREEN_TEXTURE:
	{
#if 0
		if (!strcmp(sdl.rendererDriver, "opengles")) {
			if (!(flags&GFX_CAN_32) || (flags & GFX_RGBONLY)) goto dosurface;
		}
#endif
		if (!GFX_SetupWindowScaled(sdl.desktop.want_type)) {
			LOG_MSG("SDL:Can't set video mode, falling back to surface");
			goto dosurface;
		}
		if (strcmp(sdl.rendererDriver, "auto"))
			SDL_SetHint(SDL_HINT_RENDER_DRIVER, sdl.rendererDriver); 
		sdl.renderer = SDL_CreateRenderer(sdl.window, -1,
		                                  SDL_RENDERER_ACCELERATED |
		                                  (sdl.desktop.vsync ? SDL_RENDERER_PRESENTVSYNC : 0));
		if (!sdl.renderer) {
			LOG_MSG("%s\n", SDL_GetError());
			LOG_MSG("SDL:Can't create renderer, falling back to surface");
			goto dosurface;
		}
		/* SDL_PIXELFORMAT_ARGB8888 is possible with most
		rendering drivers, "opengles" being a notable exception */
		sdl.texture.texture = SDL_CreateTexture(sdl.renderer, SDL_PIXELFORMAT_ARGB8888,
		                                        SDL_TEXTUREACCESS_STREAMING, width, height);
		/* SDL_PIXELFORMAT_ABGR8888 (not RGB) is the only supported
		format for supported format for the "opengles" driver    */
		if (!sdl.texture.texture) {
			if (flags & GFX_RGBONLY) goto dosurface;
			sdl.texture.texture = SDL_CreateTexture(sdl.renderer, SDL_PIXELFORMAT_ABGR8888,
			                                        SDL_TEXTUREACCESS_STREAMING, width, height);
		}
		if (!sdl.texture.texture) {
			SDL_DestroyRenderer(sdl.renderer);
			sdl.renderer = NULL;
			LOG_MSG("SDL:Can't create texture, falling back to surface");
			goto dosurface;
		}
		SDL_SetRenderDrawColor(sdl.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
		sdl.desktop.type=SCREEN_TEXTURE;
		Uint32 pixelFormat;
		SDL_QueryTexture(sdl.texture.texture, &pixelFormat, NULL, NULL, NULL);
		sdl.texture.pixelFormat = SDL_AllocFormat(pixelFormat);
		switch (SDL_BITSPERPIXEL(pixelFormat)) {
			case 8:
				retFlags = GFX_CAN_8;
				break;
			case 15:
				retFlags = GFX_CAN_15;
				break;
			case 16:
				retFlags = GFX_CAN_16;
				break;
			case 24: /* SDL_BYTESPERPIXEL is probably 4, though. */
			case 32:
				retFlags = GFX_CAN_32;
				break;
		}
		retFlags |= GFX_SCALING;
		SDL_RendererInfo rendererInfo;
		SDL_GetRendererInfo(sdl.renderer, &rendererInfo);
		LOG_MSG("Using driver \"%s\" for renderer", rendererInfo.name);
		if (rendererInfo.flags & SDL_RENDERER_ACCELERATED)
			retFlags |= GFX_HARDWARE;
		break;
	}
#else	// !SDL_VERSION_ATLEAST(2,0,0)
#if (HAVE_DDRAW_H) && defined(WIN32)
	case SCREEN_SURFACE_DDRAW:
		if (flags & GFX_CAN_15) bpp=15;
		if (flags & GFX_CAN_16) bpp=16;
		if (flags & GFX_CAN_32) bpp=32;
		if (!GFX_SetupSurfaceScaled((sdl.desktop.vsync && sdl.desktop.fullscreen) ? SDL_DOUBLEBUF : 0,bpp)) goto dosurface;
		sdl.blit.rect.top=sdl.clip.y;
		sdl.blit.rect.left=sdl.clip.x;
		sdl.blit.rect.right=sdl.clip.x+sdl.clip.w;
		sdl.blit.rect.bottom=sdl.clip.y+sdl.clip.h;
		sdl.blit.surface=SDL_CreateRGBSurface(SDL_HWSURFACE,sdl.draw.width,sdl.draw.height,
				sdl.surface->format->BitsPerPixel,
				sdl.surface->format->Rmask,
				sdl.surface->format->Gmask,
				sdl.surface->format->Bmask,
				0);
		if (!sdl.blit.surface || (!sdl.blit.surface->flags&SDL_HWSURFACE)) {
			if (sdl.blit.surface) {
				SDL_FreeSurface(sdl.blit.surface);
				sdl.blit.surface=0;
			}
			LOG_MSG("Failed to create ddraw surface, back to normal surface.");
			goto dosurface;
		}
		switch (sdl.surface->format->BitsPerPixel) {
		case 15:
			retFlags = GFX_CAN_15 | GFX_SCALING | GFX_HARDWARE;
			break;
		case 16:
			retFlags = GFX_CAN_16 | GFX_SCALING | GFX_HARDWARE;
               break;
		case 32:
			retFlags = GFX_CAN_32 | GFX_SCALING | GFX_HARDWARE;
               break;
		}
		sdl.desktop.type=SCREEN_SURFACE_DDRAW;
		break;
#endif
	case SCREEN_OVERLAY:
		if (sdl.overlay) {
			SDL_FreeYUVOverlay(sdl.overlay);
			sdl.overlay=0;
		}
		if (!(flags&GFX_CAN_32) || (flags & GFX_RGBONLY)) goto dosurface;
		if (!GFX_SetupSurfaceScaled(0,0)) goto dosurface;
		sdl.overlay=SDL_CreateYUVOverlay(width*2,height,SDL_UYVY_OVERLAY,sdl.surface);
		if (!sdl.overlay) {
			LOG_MSG("SDL:Failed to create overlay, switching back to surface");
			goto dosurface;
		}
		sdl.desktop.type=SCREEN_OVERLAY;
		retFlags = GFX_CAN_32 | GFX_SCALING | GFX_HARDWARE;
		break;
#endif	// !SDL_VERSION_ATLEAST(2,0,0)
#if C_OPENGL
	case SCREEN_OPENGL:
	{
#ifndef ANDROID
		if (sdl.opengl.pixel_buffer_object) {
			glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_EXT, 0);
			if (sdl.opengl.buffer) glDeleteBuffersARB(1, &sdl.opengl.buffer);
		} else
#endif
		if (sdl.opengl.framebuf) {
			free(sdl.opengl.framebuf);
		}
		sdl.opengl.framebuf=0;
#ifdef ANDROID
		if (!(flags&GFX_CAN_32)) goto dosurface; // RGBA is used on Android (OpenGL ES v1.1)
#else
		if (!(flags&GFX_CAN_32) || (flags & GFX_RGBONLY)) goto dosurface; // BGRA otherwise
#endif
		int texsize=2 << int_log2(width > height ? width : height);
		if (texsize>sdl.opengl.max_texsize) {
			LOG_MSG("SDL:OPENGL:No support for texturesize of %d, falling back to surface",texsize);
			goto dosurface;
		}
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
#ifdef ANDROID
		/* WARNING: OpenGL ES v2.0 is NOT backwards compatible
		 * with v1.1! For simplicity we force v1.1 for now,
		 * although v2.0 is probably the better way.
		 */
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif
#if SDL_VERSION_ATLEAST(2,0,0)
		GFX_SetupWindowScaled(sdl.desktop.want_type);
		/* We may simply use SDL_BYTESPERPIXEL
		here rather than SDL_BITSPERPIXEL   */
		if (!sdl.window || SDL_BYTESPERPIXEL(SDL_GetWindowPixelFormat(sdl.window))<2) {
			LOG_MSG("SDL:OPENGL:Can't open drawing window, are you running in 16bpp(or higher) mode?");
			goto dosurface;
		}
		sdl.opengl.context = SDL_GL_CreateContext(sdl.window);
		if (sdl.opengl.context == NULL) {
			LOG_MSG("SDL:OPENGL:Can't create OpenGL context, falling back to surface");
			goto dosurface;
		}
		/* Sync to VBlank if desired */
		SDL_GL_SetSwapInterval(sdl.desktop.vsync ? 1 : 0);
#else	// !SDL_VERSION_ATLEAST(2,0,0)
#if SDL_VERSION_ATLEAST(1, 2, 11)
		SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, sdl.desktop.vsync ? 1 : 0 );
#endif
		GFX_SetupSurfaceScaled(SDL_OPENGL,0);
		if (!sdl.surface || sdl.surface->format->BitsPerPixel<15) {
			LOG_MSG("SDL:OPENGL:Can't open drawing surface, are you running in 16bpp(or higher) mode?");
			goto dosurface;
		}
#endif	// !SDL_VERSION_ATLEAST(2,0,0)
		/* Create the texture and display list */
#ifndef ANDROID
		if (sdl.opengl.pixel_buffer_object) {
			glGenBuffersARB(1, &sdl.opengl.buffer);
			glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_EXT, sdl.opengl.buffer);
			glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_EXT, width*height*4, NULL, GL_STREAM_DRAW_ARB);
			glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_EXT, 0);
		} else
#endif
		{
			sdl.opengl.framebuf=malloc(width*height*4);		//32 bit color
		}
		sdl.opengl.pitch=width*4;
#if SDL_VERSION_ATLEAST(2,0,0)
		int windowHeight;
		SDL_GetWindowSize(sdl.window, NULL, &windowHeight);
		glViewport(sdl.clip.x,windowHeight-(sdl.clip.y+sdl.clip.h),sdl.clip.w,sdl.clip.h);
#else
		glViewport(sdl.clip.x,sdl.surface->h-(sdl.clip.y+sdl.clip.h),sdl.clip.w,sdl.clip.h);
#endif
		glMatrixMode (GL_PROJECTION);
		glDeleteTextures(1,&sdl.opengl.texture);
 		glGenTextures(1,&sdl.opengl.texture);
		glBindTexture(GL_TEXTURE_2D,sdl.opengl.texture);
		// No borders
#ifdef ANDROID
		/* Plain OpenGL ES (v1.1) has no mention
		of GL_CLAMP, so use GL_CLAMP_TO_EDGE  */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
#endif
		if (sdl.opengl.bilinear) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}

#ifdef ANDROID	// OpenGL ES
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texsize, texsize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
#else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texsize, texsize, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, 0);
#endif

		glClearColor (0.0, 0.0, 0.0, 1.0);
		glShadeModel (GL_FLAT);
		glDisable (GL_DEPTH_TEST);
		glDisable (GL_LIGHTING);
		glDisable(GL_CULL_FACE);
		glEnable(GL_TEXTURE_2D);
		glMatrixMode (GL_MODELVIEW);
		glLoadIdentity ();

		GLfloat tex_width=((GLfloat)(width)/(GLfloat)texsize);
		GLfloat tex_height=((GLfloat)(height)/(GLfloat)texsize);

#ifdef ANDROID
		/* Display lists are not available with OpenGL ES (1.0, 2.0).
		Furthermore, we can't use glBegin, glTexCoord2f and glEnd. */
		sdl.opengl.texCoords[0] = 0; sdl.opengl.texCoords[1] = tex_height; // lower left
		sdl.opengl.texCoords[2] = tex_width; sdl.opengl.texCoords[3] = tex_height; // lower right
		sdl.opengl.texCoords[4] = tex_width; sdl.opengl.texCoords[5] = 0; // upper right
		sdl.opengl.texCoords[6] = 0; sdl.opengl.texCoords[7] = 0; // upper left
#else
		if (glIsList(sdl.opengl.displaylist)) glDeleteLists(sdl.opengl.displaylist, 1);
		sdl.opengl.displaylist = glGenLists(1);
		glNewList(sdl.opengl.displaylist, GL_COMPILE);
		glClear(GL_COLOR_BUFFER_BIT);
		glBindTexture(GL_TEXTURE_2D, sdl.opengl.texture);
		glBegin(GL_QUADS);
		// lower left
		glTexCoord2f(0,tex_height); glVertex2f(-1.0f,-1.0f);
		// lower right
		glTexCoord2f(tex_width,tex_height); glVertex2f(1.0f, -1.0f);
		// upper right
		glTexCoord2f(tex_width,0); glVertex2f(1.0f, 1.0f);
		// upper left
		glTexCoord2f(0,0); glVertex2f(-1.0f, 1.0f);
		glEnd();
		glEndList();
#endif
		sdl.desktop.type=SCREEN_OPENGL;
		retFlags = GFX_CAN_32 | GFX_SCALING;
#ifndef ANDROID
		if (sdl.opengl.pixel_buffer_object)
			retFlags |= GFX_HARDWARE;
#endif
	break;
		}//OPENGL
#endif	//C_OPENGL
	default:
		goto dosurface;
		break;
	}//CASE
	if (retFlags)
		GFX_Start();
	if (!sdl.mouse.autoenable) SDL_ShowCursor(sdl.mouse.autolock?SDL_DISABLE:SDL_ENABLE);
	return retFlags;
}


void GFX_CaptureMouse(void) {
	sdl.mouse.locked=!sdl.mouse.locked;
	if (sdl.mouse.locked) {
#if SDL_VERSION_ATLEAST(2,0,0)
		SDL_SetRelativeMouseMode(SDL_TRUE);
#else
		SDL_WM_GrabInput(SDL_GRAB_ON);
#endif
		SDL_ShowCursor(SDL_DISABLE);
	} else {
#if SDL_VERSION_ATLEAST(2,0,0)
		SDL_SetRelativeMouseMode(SDL_FALSE);
#else
		SDL_WM_GrabInput(SDL_GRAB_OFF);
#endif
		if (sdl.mouse.autoenable || !sdl.mouse.autolock) SDL_ShowCursor(SDL_ENABLE);
	}
        mouselocked=sdl.mouse.locked;
}

void GFX_UpdateSDLCaptureState(void) {
	if (sdl.mouse.locked) {
#if SDL_VERSION_ATLEAST(2,0,0)
		SDL_SetRelativeMouseMode(SDL_TRUE);
#else
		SDL_WM_GrabInput(SDL_GRAB_ON);
#endif
		SDL_ShowCursor(SDL_DISABLE);
	} else {
#if SDL_VERSION_ATLEAST(2,0,0)
		SDL_SetRelativeMouseMode(SDL_FALSE);
#else
		SDL_WM_GrabInput(SDL_GRAB_OFF);
#endif
		if (sdl.mouse.autoenable || !sdl.mouse.autolock) SDL_ShowCursor(SDL_ENABLE);
	}
	CPU_Reset_AutoAdjust();
	GFX_SetTitle(-1,-1,false);
}

bool mouselocked; //Global variable for mapper
static void CaptureMouse(bool pressed) {
	if (!pressed)
		return;
	GFX_CaptureMouse();
}

#if defined (WIN32)
STICKYKEYS stick_keys = {sizeof(STICKYKEYS), 0};
void sticky_keys(bool restore){
	static bool inited = false;
	if (!inited){
		inited = true;
		SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(STICKYKEYS), &stick_keys, 0);
	} 
	if (restore) {
		SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &stick_keys, 0);
		return;
	}
	//Get current sticky keys layout:
	STICKYKEYS s = {sizeof(STICKYKEYS), 0};
	SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(STICKYKEYS), &s, 0);
	if ( !(s.dwFlags & SKF_STICKYKEYSON)) { //Not on already
		s.dwFlags &= ~SKF_HOTKEYACTIVE;
		SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &s, 0);
	}
}
#endif

void GFX_SwitchFullScreen(void) {
	sdl.desktop.fullscreen=!sdl.desktop.fullscreen;
	if (sdl.desktop.fullscreen) {
		if (!sdl.mouse.locked) GFX_CaptureMouse();
#if defined (WIN32)
		sticky_keys(false); //disable sticky keys in fullscreen mode
#endif
	} else {
		if (sdl.mouse.locked) GFX_CaptureMouse();
#if defined (WIN32)		
		sticky_keys(true); //restore sticky keys to default state in windowed mode.
#endif
	}
	GFX_ResetScreen();
}

static void SwitchFullScreen(bool pressed) {
	if (!pressed)
		return;

	if (sdl.desktop.lazy_fullscreen) {
//		sdl.desktop.lazy_fullscreen_req=true;
		LOG_MSG("GFX LF: fullscreen switching not supported");
	} else {
		GFX_SwitchFullScreen();
	}
}

void GFX_SwitchLazyFullscreen(bool lazy) {
	sdl.desktop.lazy_fullscreen=lazy;
	sdl.desktop.lazy_fullscreen_req=false;
}

void GFX_SwitchFullscreenNoReset(void) {
	sdl.desktop.fullscreen=!sdl.desktop.fullscreen;
}

bool GFX_LazyFullscreenRequested(void) {
	if (sdl.desktop.lazy_fullscreen) return sdl.desktop.lazy_fullscreen_req;
	return false;
}

void GFX_RestoreMode(void) {
	GFX_SetSize(sdl.draw.width,sdl.draw.height,sdl.draw.flags,sdl.draw.scalex,sdl.draw.scaley,sdl.draw.callback);
	GFX_UpdateSDLCaptureState();
}


bool GFX_StartUpdate(Bit8u * & pixels,Bitu & pitch) {
#if SDL_VERSION_ATLEAST(2,0,0)
	if (!sdl.update_display_contents)
		return false;
#endif
	if (!sdl.active || sdl.updating)
		return false;
	switch (sdl.desktop.type) {
	case SCREEN_SURFACE:
#if !SDL_VERSION_ATLEAST(2,0,0)
		if (sdl.blit.surface) {
			if (SDL_MUSTLOCK(sdl.blit.surface) && SDL_LockSurface(sdl.blit.surface))
				return false;
			pixels=(Bit8u *)sdl.blit.surface->pixels;
			pitch=sdl.blit.surface->pitch;
		} else
#endif
		{
#if !SDL_VERSION_ATLEAST(2,0,0)
			if (SDL_MUSTLOCK(sdl.surface) && SDL_LockSurface(sdl.surface))
				return false;
#endif
			pixels=(Bit8u *)sdl.surface->pixels;
			pixels+=sdl.clip.y*sdl.surface->pitch;
			pixels+=sdl.clip.x*sdl.surface->format->BytesPerPixel;
			pitch=sdl.surface->pitch;
		}
		sdl.updating=true;
		return true;
#if SDL_VERSION_ATLEAST(2,0,0)
	case SCREEN_TEXTURE:
	{
		void * texPixels;
		int texPitch;
		if (SDL_LockTexture(sdl.texture.texture, NULL, &texPixels, &texPitch) < 0)
			return false;
		pixels = (Bit8u *)texPixels;
		pitch = texPitch;
		sdl.updating=true;
		return true;
	}
#else	// !SDL_VERSION_ATLEAST(2,0,0)
#if (HAVE_DDRAW_H) && defined(WIN32)
	case SCREEN_SURFACE_DDRAW:
		if (SDL_LockSurface(sdl.blit.surface)) {
//			LOG_MSG("SDL Lock failed");
			return false;
		}
		pixels=(Bit8u *)sdl.blit.surface->pixels;
		pitch=sdl.blit.surface->pitch;
		sdl.updating=true;
		return true;
#endif	// DirectDraw
	case SCREEN_OVERLAY:
		if (SDL_LockYUVOverlay(sdl.overlay)) return false;
		pixels=(Bit8u *)*(sdl.overlay->pixels);
		pitch=*(sdl.overlay->pitches);
		sdl.updating=true;
		return true;
#endif	// !SDL_VERSION_ATLEAST(2,0,0)
#if C_OPENGL
	case SCREEN_OPENGL:
#ifndef ANDROID
		if(sdl.opengl.pixel_buffer_object) {
		    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_EXT, sdl.opengl.buffer);
		    pixels=(Bit8u *)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_EXT, GL_WRITE_ONLY);
		} else
#endif
		{
		    pixels=(Bit8u *)sdl.opengl.framebuf;
		}
		pitch=sdl.opengl.pitch;
		sdl.updating=true;
		return true;
#endif
	default:
		break;
	}
	return false;
}


void GFX_EndUpdate( const Bit16u *changedLines ) {
#if (HAVE_DDRAW_H) && defined(WIN32)
	int ret;
#endif
#if SDL_VERSION_ATLEAST(2,0,0)
	if (!sdl.update_display_contents)
		return;
#endif
	if (!sdl.updating)
		return;
	sdl.updating=false;
	switch (sdl.desktop.type) {
	case SCREEN_SURFACE:
#if !SDL_VERSION_ATLEAST(2,0,0)
		if (SDL_MUSTLOCK(sdl.surface)) {
			if (sdl.blit.surface) {
				SDL_UnlockSurface(sdl.blit.surface);
				int Blit = SDL_BlitSurface( sdl.blit.surface, 0, sdl.surface, &sdl.clip );
				LOG(LOG_MISC,LOG_WARN)("BlitSurface returned %d",Blit);
			} else {
				SDL_UnlockSurface(sdl.surface);
			}
			SDL_Flip(sdl.surface);
		} else
#endif
		if (changedLines) {
			Bitu y = 0, index = 0, rectCount = 0;
			while (y < sdl.draw.height) {
				if (!(index & 1)) {
					y += changedLines[index];
				} else {
					SDL_Rect *rect = &sdl.updateRects[rectCount++];
					rect->x = sdl.clip.x;
					rect->y = sdl.clip.y + y;
					rect->w = (Bit16u)sdl.draw.width;
					rect->h = changedLines[index];
#if 0
					if (rect->h + rect->y > sdl.surface->h) {
						LOG_MSG("WTF %d +  %d  >%d",rect->h,rect->y,sdl.surface->h);
					}
#endif
					y += changedLines[index];
				}
				index++;
			}
			if (rectCount)
#if SDL_VERSION_ATLEAST(2,0,0)
				SDL_UpdateWindowSurfaceRects( sdl.window, sdl.updateRects, rectCount );
#if 0
				if (SDL_UpdateWindowSurfaceRects( sdl.window, sdl.updateRects, rectCount ) < 0) {
					/* Maybe window has been restored
					 * in a way and this is needed
					 * for some reason
					 */
					sdl.surface = SDL_GetWindowSurface(sdl.window);
					SDL_UpdateWindowSurface( sdl.window );
				}
#endif
#else
				SDL_UpdateRects( sdl.surface, rectCount, sdl.updateRects );
#endif
		}
		break;
#if SDL_VERSION_ATLEAST(2,0,0)
	case SCREEN_TEXTURE:
		SDL_UnlockTexture(sdl.texture.texture);
		SDL_RenderClear(sdl.renderer);
		SDL_RenderCopy(sdl.renderer, sdl.texture.texture, NULL, &sdl.clip);
		SDL_RenderPresent(sdl.renderer);
		break;
#else	// !SDL_VERSION_ATLEAST(2,0,0)
#if (HAVE_DDRAW_H) && defined(WIN32)
	case SCREEN_SURFACE_DDRAW:
		SDL_UnlockSurface(sdl.blit.surface);
		ret=IDirectDrawSurface3_Blt(
			sdl.surface->hwdata->dd_writebuf,&sdl.blit.rect,
			sdl.blit.surface->hwdata->dd_surface,0,
			DDBLT_WAIT, NULL);
		switch (ret) {
		case DD_OK:
			break;
		case DDERR_SURFACELOST:
			IDirectDrawSurface3_Restore(sdl.blit.surface->hwdata->dd_surface);
			IDirectDrawSurface3_Restore(sdl.surface->hwdata->dd_surface);
			break;
		default:
			LOG_MSG("DDRAW:Failed to blit, error %X",ret);
		}
		SDL_Flip(sdl.surface);
		break;
#endif	// DirectDraw
	case SCREEN_OVERLAY:
		SDL_UnlockYUVOverlay(sdl.overlay);
		SDL_DisplayYUVOverlay(sdl.overlay,&sdl.clip);
		break;
#endif	// !SDL_VERSION_ATLEAST(2,0,0)
#if C_OPENGL
	case SCREEN_OPENGL:
#ifndef ANDROID
		if (sdl.opengl.pixel_buffer_object) {
			glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_EXT);
			glBindTexture(GL_TEXTURE_2D, sdl.opengl.texture);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
					sdl.draw.width, sdl.draw.height, GL_BGRA_EXT,
					GL_UNSIGNED_INT_8_8_8_8_REV, 0);
			glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_EXT, 0);
			glCallList(sdl.opengl.displaylist);
#if SDL_VERSION_ATLEAST(2,0,0)
			SDL_GL_SwapWindow(sdl.window);
#else
 			SDL_GL_SwapBuffers();
#endif
		} else
#endif	// ifndef ANDROID
		if (changedLines) {
			Bitu y = 0, index = 0;
			glBindTexture(GL_TEXTURE_2D, sdl.opengl.texture);
			while (y < sdl.draw.height) {
				if (!(index & 1)) {
					y += changedLines[index];
				} else {
					Bit8u *pixels = (Bit8u *)sdl.opengl.framebuf + y * sdl.opengl.pitch;
					Bitu height = changedLines[index];
#ifdef ANDROID
					/* Try GL_UNSIGNED_BYTE... */
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y,
						sdl.draw.width, height, GL_RGBA,
						GL_UNSIGNED_BYTE, pixels );
#else
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y,
						sdl.draw.width, height, GL_BGRA_EXT,
						GL_UNSIGNED_INT_8_8_8_8_REV, pixels );
#endif
					y += height;
				}
				index++;
			}
#ifdef ANDROID
			/* We can't use display lists with OpenGL ES
			and we should use Vertex Buffer Arrays    */
			glClear(GL_COLOR_BUFFER_BIT);
			glBindTexture(GL_TEXTURE_2D, sdl.opengl.texture);

			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);

			glVertexPointer(2, GL_FLOAT, 0, sdl.opengl.vertCoords);
			glTexCoordPointer(2, GL_FLOAT, 0, sdl.opengl.texCoords);
			glDrawArrays(GL_TRIANGLE_FAN,0,4);

			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#else	// !ANDROID
			glCallList(sdl.opengl.displaylist);
#endif	// !ANDROID
#if SDL_VERSION_ATLEAST(2,0,0)
			SDL_GL_SwapWindow(sdl.window);
#else
			SDL_GL_SwapBuffers();
#endif
		}
		break;
#endif
	default:
		break;
	}
}


void GFX_SetPalette(Bitu start,Bitu count,GFX_PalEntry * entries) {
	// Paletted window surfaces are unsupported with SDL 2.0
#if !SDL_VERSION_ATLEAST(2,0,0)
	/* I should probably not change the GFX_PalEntry :) */
	if (sdl.surface->flags & SDL_HWPALETTE) {
		if (!SDL_SetPalette(sdl.surface,SDL_PHYSPAL,(SDL_Color *)entries,start,count)) {
			E_Exit("SDL:Can't set palette");
		}
	} else {
		if (!SDL_SetPalette(sdl.surface,SDL_LOGPAL,(SDL_Color *)entries,start,count)) {
			E_Exit("SDL:Can't set palette");
		}
	}
#endif
}

Bitu GFX_GetRGB(Bit8u red,Bit8u green,Bit8u blue) {
	switch (sdl.desktop.type) {
	case SCREEN_SURFACE:
#if !SDL_VERSION_ATLEAST(2,0,0)
	case SCREEN_SURFACE_DDRAW:
#endif
		return SDL_MapRGB(sdl.surface->format,red,green,blue);
#if SDL_VERSION_ATLEAST(2,0,0)
	case SCREEN_TEXTURE:
		return SDL_MapRGB(sdl.texture.pixelFormat,red,green,blue);
#else
	case SCREEN_OVERLAY:
		{
			Bit8u y =  ( 9797*(red) + 19237*(green) +  3734*(blue) ) >> 15;
			Bit8u u =  (18492*((blue)-(y)) >> 15) + 128;
			Bit8u v =  (23372*((red)-(y)) >> 15) + 128;
#ifdef WORDS_BIGENDIAN
			return (y << 0) | (v << 8) | (y << 16) | (u << 24);
#else
			return (u << 0) | (y << 8) | (v << 16) | (y << 24);
#endif
		}
#endif	// !SDL_VERSION_ATLEAST(2,0,0)
	case SCREEN_OPENGL:
#ifdef ANDROID
		//Use RGBA on Android with OpenGL ES v1.1
		return ((red << 0) | (green << 8) | (blue << 16)) | (255 << 24);
#else
		//USE BGRA otherwise
		return ((blue << 0) | (green << 8) | (red << 16)) | (255 << 24);
#endif
	}
	return 0;
}

void GFX_Stop() {
	if (sdl.updating)
		GFX_EndUpdate( 0 );
	sdl.active=false;
}

void GFX_Start() {
	sdl.active=true;
}

/* NOTE: The following appears to do its job on Android only *before*
 * a screen rotation occurs. After that, the old dimensions are retrieved.
 * For the updated dimensions we should listen to a window resize event.
 */
#if SDL_VERSION_ATLEAST(2,0,0)
void GFX_ObtainDisplayDimensions() {
	SDL_Rect displayDimensions;
	SDL_GetDisplayBounds(sdl.displayNumber, &displayDimensions);
	sdl.desktop.full.width = displayDimensions.w;
	sdl.desktop.full.height = displayDimensions.h;

}
#endif

static void GUI_ShutDown(Section * /*sec*/) {
	GFX_Stop();
	if (sdl.draw.callback) (sdl.draw.callback)( GFX_CallBackStop );
	if (sdl.mouse.locked) GFX_CaptureMouse();
	if (sdl.desktop.fullscreen) GFX_SwitchFullScreen();
}


static void SetPriority(PRIORITY_LEVELS level) {

#if C_SET_PRIORITY
// Do nothing if priorties are not the same and not root, else the highest
// priority can not be set as users can only lower priority (not restore it)

	if((sdl.priority.focus != sdl.priority.nofocus ) &&
		(getuid()!=0) ) return;

#endif
	switch (level) {
#ifdef WIN32
	case PRIORITY_LEVEL_PAUSE:	// if DOSBox is paused, assume idle priority
	case PRIORITY_LEVEL_LOWEST:
		SetPriorityClass(GetCurrentProcess(),IDLE_PRIORITY_CLASS);
		break;
	case PRIORITY_LEVEL_LOWER:
		SetPriorityClass(GetCurrentProcess(),BELOW_NORMAL_PRIORITY_CLASS);
		break;
	case PRIORITY_LEVEL_NORMAL:
		SetPriorityClass(GetCurrentProcess(),NORMAL_PRIORITY_CLASS);
		break;
	case PRIORITY_LEVEL_HIGHER:
		SetPriorityClass(GetCurrentProcess(),ABOVE_NORMAL_PRIORITY_CLASS);
		break;
	case PRIORITY_LEVEL_HIGHEST:
		SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS);
		break;
#elif C_SET_PRIORITY
/* Linux use group as dosbox has mulitple threads under linux */
	case PRIORITY_LEVEL_PAUSE:	// if DOSBox is paused, assume idle priority
	case PRIORITY_LEVEL_LOWEST:
		setpriority (PRIO_PGRP, 0,PRIO_MAX);
		break;
	case PRIORITY_LEVEL_LOWER:
		setpriority (PRIO_PGRP, 0,PRIO_MAX-(PRIO_TOTAL/3));
		break;
	case PRIORITY_LEVEL_NORMAL:
		setpriority (PRIO_PGRP, 0,PRIO_MAX-(PRIO_TOTAL/2));
		break;
	case PRIORITY_LEVEL_HIGHER:
		setpriority (PRIO_PGRP, 0,PRIO_MAX-((3*PRIO_TOTAL)/5) );
		break;
	case PRIORITY_LEVEL_HIGHEST:
		setpriority (PRIO_PGRP, 0,PRIO_MAX-((3*PRIO_TOTAL)/4) );
		break;
#endif
	default:
		break;
	}
}

extern Bit8u int10_font_14[256 * 14];
static void OutputString(Bitu x,Bitu y,const char * text,Bit32u color,Bit32u color2,SDL_Surface * output_surface) {
	Bit32u * draw=(Bit32u*)(((Bit8u *)output_surface->pixels)+((y)*output_surface->pitch))+x;
	while (*text) {
		Bit8u * font=&int10_font_14[(*text)*14];
		Bitu i,j;
		Bit32u * draw_line=draw;
		for (i=0;i<14;i++) {
			Bit8u map=*font++;
			for (j=0;j<8;j++) {
				if (map & 0x80) *((Bit32u*)(draw_line+j))=color; else *((Bit32u*)(draw_line+j))=color2;
				map<<=1;
			}
			draw_line+=output_surface->pitch/4;
		}
		text++;
		draw+=8;
	}
}

#include "dosbox_splash.h"

//extern void UI_Run(bool);
void Restart(bool pressed);

static void GUI_StartUp(Section * sec) {
	sec->AddDestroyFunction(&GUI_ShutDown);
	Section_prop * section=static_cast<Section_prop *>(sec);
	sdl.active=false;
	sdl.updating=false;
#if SDL_VERSION_ATLEAST(2,0,0)
	sdl.update_window=true;
	sdl.update_display_contents=true;
#endif

	GFX_SetIcon();

	sdl.desktop.lazy_fullscreen=false;
	sdl.desktop.lazy_fullscreen_req=false;

#ifdef ANDROID
	sdl.desktop.fullscreen=true;
#else
	sdl.desktop.fullscreen=section->Get_bool("fullscreen");
#endif
	sdl.wait_on_error=section->Get_bool("waitonerror");

	Prop_multival* p=section->Get_multival("priority");
	std::string focus = p->GetSection()->Get_string("active");
	std::string notfocus = p->GetSection()->Get_string("inactive");

	if      (focus == "lowest")  { sdl.priority.focus = PRIORITY_LEVEL_LOWEST;  }
	else if (focus == "lower")   { sdl.priority.focus = PRIORITY_LEVEL_LOWER;   }
	else if (focus == "normal")  { sdl.priority.focus = PRIORITY_LEVEL_NORMAL;  }
	else if (focus == "higher")  { sdl.priority.focus = PRIORITY_LEVEL_HIGHER;  }
	else if (focus == "highest") { sdl.priority.focus = PRIORITY_LEVEL_HIGHEST; }

	if      (notfocus == "lowest")  { sdl.priority.nofocus=PRIORITY_LEVEL_LOWEST;  }
	else if (notfocus == "lower")   { sdl.priority.nofocus=PRIORITY_LEVEL_LOWER;   }
	else if (notfocus == "normal")  { sdl.priority.nofocus=PRIORITY_LEVEL_NORMAL;  }
	else if (notfocus == "higher")  { sdl.priority.nofocus=PRIORITY_LEVEL_HIGHER;  }
	else if (notfocus == "highest") { sdl.priority.nofocus=PRIORITY_LEVEL_HIGHEST; }
	else if (notfocus == "pause")   {
		/* we only check for pause here, because it makes no sense
		 * for DOSBox to be paused while it has focus
		 */
		sdl.priority.nofocus=PRIORITY_LEVEL_PAUSE;
	}

	SetPriority(sdl.priority.focus); //Assume focus on startup
	sdl.mouse.locked=false;
	mouselocked=false; //Global for mapper
	sdl.mouse.requestlock=false;
#ifdef ANDROID
	sdl.mouse.isLeftMouseFingerUsed=sdl.mouse.isRightMouseFingerUsed=false;
	sdl.mouse.isMiddleMouseFingerUsed=sdl.mouse.isEscKeyFingerUsed=false;
#endif
	sdl.desktop.full.fixed=false;
	const char* fullresolution=section->Get_string("fullresolution");
	sdl.desktop.full.width  = 0;
	sdl.desktop.full.height = 0;
	if(fullresolution && *fullresolution) {
		char res[100];
		strncpy( res, fullresolution, sizeof( res ));
		fullresolution = lowcase (res);//so x and X are allowed
		if (strcmp(fullresolution,"original")) {
			sdl.desktop.full.fixed = true;
			if (strcmp(fullresolution,"desktop")) { //desktop = 0x0
				char* height = const_cast<char*>(strchr(fullresolution,'x'));
				if (height && * height) {
					*height = 0;
					sdl.desktop.full.height = (Bit16u)atoi(height+1);
					sdl.desktop.full.width  = (Bit16u)atoi(res);
				}
			}
		}
	}

	sdl.desktop.window.width  = 0;
	sdl.desktop.window.height = 0;
#ifndef ANDROID
	const char* windowresolution=section->Get_string("windowresolution");
	if(windowresolution && *windowresolution) {
		char res[100];
		strncpy( res,windowresolution, sizeof( res ));
		windowresolution = lowcase (res);//so x and X are allowed
		if(strcmp(windowresolution,"original")) {
			char* height = const_cast<char*>(strchr(windowresolution,'x'));
			if(height && *height) {
				*height = 0;
				sdl.desktop.window.height = (Bit16u)atoi(height+1);
				sdl.desktop.window.width  = (Bit16u)atoi(res);
			}
		}
	}
#endif

	sdl.desktop.vsync=section->Get_bool("vsync");

#if SDL_VERSION_ATLEAST(2,0,0)

	sdl.displayNumber=section->Get_int("display");
	if ((sdl.displayNumber < 0) || (sdl.displayNumber >= SDL_GetNumVideoDisplays())) {
		sdl.displayNumber = 0;
		LOG_MSG("SDL:Display number out of bounds, switching back to 0");
	}
	sdl.desktop.full.display_res = (!sdl.desktop.full.width || !sdl.desktop.full.height);
	if (sdl.desktop.full.display_res) {
		GFX_ObtainDisplayDimensions();
	}

#else	// !SDL_VERSION_ATLEAST(2,0,0)

#if SDL_VERSION_ATLEAST(1, 2, 10)
	if (!sdl.desktop.full.width || !sdl.desktop.full.height){
		//Can only be done on the very first call! Not restartable.
		const SDL_VideoInfo* vidinfo = SDL_GetVideoInfo();
		if (vidinfo) {
			sdl.desktop.full.width = vidinfo->current_w;
			sdl.desktop.full.height = vidinfo->current_h;
		}
	}
#endif

	if (!sdl.desktop.full.width) {
#ifdef WIN32
		sdl.desktop.full.width=(Bit16u)GetSystemMetrics(SM_CXSCREEN);
#else
		LOG_MSG("Your fullscreen resolution can NOT be determined, it's assumed to be 1024x768.\nPlease edit the configuration file if this value is wrong.");
		sdl.desktop.full.width=1024;
#endif
	}
	if (!sdl.desktop.full.height) {
#ifdef WIN32
		sdl.desktop.full.height=(Bit16u)GetSystemMetrics(SM_CYSCREEN);
#else
		sdl.desktop.full.height=768;
#endif
	}
#endif	// !SDL_VERSION_ATLEAST(2,0,0)

	sdl.mouse.autoenable=section->Get_bool("autolock");
	if (!sdl.mouse.autoenable) SDL_ShowCursor(SDL_DISABLE);
	sdl.mouse.autolock=false;
	sdl.mouse.sensitivity=section->Get_int("sensitivity");
	std::string output=section->Get_string("output");

	/* Setup Mouse correctly if fullscreen */
	if(sdl.desktop.fullscreen) GFX_CaptureMouse();

	if (output == "surface") {
		sdl.desktop.want_type=SCREEN_SURFACE;
#if SDL_VERSION_ATLEAST(2,0,0)
	} else if (output == "texture") {
		sdl.desktop.want_type=SCREEN_TEXTURE;
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	} else if (output == "texturenb") {
		sdl.desktop.want_type=SCREEN_TEXTURE;
		// Currently the default, but... oh well
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
#else	// !SDL_VERSION_ATLEAST(2,0,0)
#if (HAVE_DDRAW_H) && defined(WIN32)
	} else if (output == "ddraw") {
		sdl.desktop.want_type=SCREEN_SURFACE_DDRAW;
#endif
	} else if (output == "overlay") {
		sdl.desktop.want_type=SCREEN_OVERLAY;
#endif	// !SDL_VERSION_ATLEAST(2,0,0)
#if C_OPENGL
	} else if (output == "opengl") {
		sdl.desktop.want_type=SCREEN_OPENGL;
		sdl.opengl.bilinear=true;
	} else if (output == "openglnb") {
		sdl.desktop.want_type=SCREEN_OPENGL;
		sdl.opengl.bilinear=false;
#endif
	} else {
		LOG_MSG("SDL:Unsupported output device %s, switching back to surface",output.c_str());
		sdl.desktop.want_type=SCREEN_SURFACE;//SHOULDN'T BE POSSIBLE anymore
	}

#if SDL_VERSION_ATLEAST(2,0,0)
	sdl.texture.texture=0;
	sdl.texture.pixelFormat=0;
	sdl.window=0;
	sdl.renderer=0;
	sdl.rendererDriver = section->Get_string("renderer");
#else
	sdl.overlay=0;
#endif

#if (defined C_OPENGL) && (defined ANDROID) // OpenGL ES
	static const GLfloat vertCoords[] = {
	-1, -1, //  lower left
	1, -1, // lower right
	1, 1, // upper right
	-1, 1 // upper left
	};
	memcpy(sdl.opengl.vertCoords, vertCoords, sizeof(vertCoords));
#endif

#if C_OPENGL
   if(sdl.desktop.want_type==SCREEN_OPENGL){ /* OPENGL is requested */
#if SDL_VERSION_ATLEAST(2,0,0)
	if (!GFX_SetSDLOpenGLWindow(640,400)) {
		LOG_MSG("Could not create OpenGL window, switching back to surface");
		sdl.desktop.want_type=SCREEN_SURFACE;
	} else {
		sdl.opengl.context = SDL_GL_CreateContext(sdl.window);
		if (sdl.opengl.context == 0) {
			LOG_MSG("Could not create OpenGL context, switching back to surface");
			sdl.desktop.want_type=SCREEN_SURFACE;
		}
	}
	if (sdl.desktop.want_type==SCREEN_OPENGL) {
#else	// Same story but for SDL 1.2
	sdl.surface=SDL_SetVideoMode(640,400,0,SDL_OPENGL);
	if (sdl.surface == NULL) {
		LOG_MSG("Could not initialize OpenGL, switching back to surface");
		sdl.desktop.want_type=SCREEN_SURFACE;
	} else {
#endif	// End of SDL specific video mode check. Let's assume it has passed...
	sdl.opengl.buffer=0;
	sdl.opengl.framebuf=0;
	sdl.opengl.texture=0;
#ifndef ANDROID
	sdl.opengl.displaylist=0;
#endif

	glGetIntegerv (GL_MAX_TEXTURE_SIZE, &sdl.opengl.max_texsize);
#ifndef ANDROID
	glGenBuffersARB = (PFNGLGENBUFFERSARBPROC)SDL_GL_GetProcAddress("glGenBuffersARB");
	glBindBufferARB = (PFNGLBINDBUFFERARBPROC)SDL_GL_GetProcAddress("glBindBufferARB");
	glDeleteBuffersARB = (PFNGLDELETEBUFFERSARBPROC)SDL_GL_GetProcAddress("glDeleteBuffersARB");
	glBufferDataARB = (PFNGLBUFFERDATAARBPROC)SDL_GL_GetProcAddress("glBufferDataARB");
	glMapBufferARB = (PFNGLMAPBUFFERARBPROC)SDL_GL_GetProcAddress("glMapBufferARB");
	glUnmapBufferARB = (PFNGLUNMAPBUFFERARBPROC)SDL_GL_GetProcAddress("glUnmapBufferARB");
	const char * gl_ext = (const char *)glGetString (GL_EXTENSIONS);
	if(gl_ext && *gl_ext){
		sdl.opengl.packed_pixel=(strstr(gl_ext,"EXT_packed_pixels") > 0);
		sdl.opengl.paletted_texture=(strstr(gl_ext,"EXT_paletted_texture") > 0);
		sdl.opengl.pixel_buffer_object=(strstr(gl_ext,"GL_ARB_pixel_buffer_object") >0 ) &&
		    glGenBuffersARB && glBindBufferARB && glDeleteBuffersARB && glBufferDataARB &&
		    glMapBufferARB && glUnmapBufferARB;
    	} else {
		sdl.opengl.packed_pixel=sdl.opengl.paletted_texture=false;
	}
#endif	// ifndef ANDROID
	}
	} /* OPENGL is requested end */

#endif	//OPENGL
	/* Initialize screen for first time */
#if SDL_VERSION_ATLEAST(2,0,0)
	if (!GFX_SetSDLSurfaceWindow(640,400))
		E_Exit("Could not initialize video: %s",SDL_GetError());
	sdl.surface = SDL_GetWindowSurface(sdl.window);
#if 0
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, sdl.rendererDriver); 
	sdl.renderer = SDL_CreateRenderer(sdl.window, -1, SDL_RENDERER_SOFTWARE);
	if (!sdl.renderer)
		E_Exit("Could not create software renderer: %s",SDL_GetError());
	/* SDL_PIXELFORMAT_RGBA8888 is currently
	supported by the software renderer driver. */
	sdl.texture.texture = SDL_CreateTexture(sdl.renderer, SDL_PIXELFORMAT_RGBA8888,
	                                        SDL_TEXTUREACCESS_STREAMING, 640, 400);
	if (!sdl.texture.texture)
		E_Exit("Could not create texture: %s",SDL_GetError());
#endif
	sdl.desktop.sdl2pixelFormat = SDL_GetWindowPixelFormat(sdl.window);
	LOG_MSG("SDL:Current window pixel format: %s", SDL_GetPixelFormatName(sdl.desktop.sdl2pixelFormat));
	/* Do NOT use SDL_BITSPERPIXEL here - It returns 24 for
	SDL_PIXELFORMAT_RGB888, while SDL_BYTESPERPIXEL returns 4.
	To compare, with SDL 1.2 the detected desktop color depth is 32 bpp. */
	sdl.desktop.bpp=8*SDL_BYTESPERPIXEL(sdl.desktop.sdl2pixelFormat);
#else
	sdl.surface=SDL_SetVideoMode(640,400,0,0);
	if (sdl.surface == NULL) E_Exit("Could not initialize video: %s",SDL_GetError());
	sdl.desktop.bpp=sdl.surface->format->BitsPerPixel;
#endif
	if (sdl.desktop.bpp==24) {
		LOG_MSG("SDL:You are running in 24 bpp mode, this will slow down things!");
	}
	GFX_Stop();
#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_SetWindowTitle(sdl.window,"DOSBox"); // VERSION is gone...
#else
	SDL_WM_SetCaption("DOSBox",VERSION);
#endif

/* The endian part is intentionally disabled as somehow it produces correct results without according to rhoenie*/
//#if SDL_BYTEORDER == SDL_BIG_ENDIAN
//    Bit32u rmask = 0xff000000;
//    Bit32u gmask = 0x00ff0000;
//    Bit32u bmask = 0x0000ff00;
//#else
    Bit32u rmask = 0x000000ff;
    Bit32u gmask = 0x0000ff00;
    Bit32u bmask = 0x00ff0000;
//#endif

/* Please leave the Splash screen stuff in working order in DOSBox. We spend a lot of time making DOSBox. */
	SDL_Surface* splash_surf = SDL_CreateRGBSurface(SDL_SWSURFACE, 640, 400, 32, rmask, gmask, bmask, 0);
	if (0) {
#if SDL_VERSION_ATLEAST(2,0,0)
		SDL_SetSurfaceBlendMode(splash_surf, SDL_BLENDMODE_BLEND);
#endif
		SDL_FillRect(splash_surf, NULL, SDL_MapRGB(splash_surf->format, 0, 0, 0));

		Bit8u* tmpbufp = new Bit8u[640*400*3];
		GIMP_IMAGE_RUN_LENGTH_DECODE(tmpbufp,gimp_image.rle_pixel_data,640*400,3);
		for (Bitu y=0; y<400; y++) {

			Bit8u* tmpbuf = tmpbufp + y*640*3;
			Bit32u * draw=(Bit32u*)(((Bit8u *)splash_surf->pixels)+((y)*splash_surf->pitch));
			for (Bitu x=0; x<640; x++) {
//#if SDL_BYTEORDER == SDL_BIG_ENDIAN
//				*draw++ = tmpbuf[x*3+2]+tmpbuf[x*3+1]*0x100+tmpbuf[x*3+0]*0x10000+0x00000000;
//#else
				*draw++ = tmpbuf[x*3+0]+tmpbuf[x*3+1]*0x100+tmpbuf[x*3+2]*0x10000+0x00000000;
//#endif
			}
		}

		bool exit_splash = false;

		static Bitu max_splash_loop = 600;
		static Bitu splash_fade = 100;
		static bool use_fadeout = true;

		for (Bit32u ct = 0,startticks = GetTicks();ct < max_splash_loop;ct = GetTicks()-startticks) {
			SDL_Event evt;
			while (SDL_PollEvent(&evt)) {
				if (evt.type == SDL_QUIT) {
					exit_splash = true;
					break;
				}
			}
			if (exit_splash) break;

			if (ct<1) {
				SDL_FillRect(sdl.surface, NULL, SDL_MapRGB(sdl.surface->format, 0, 0, 0));
#if SDL_VERSION_ATLEAST(2,0,0)
				SDL_SetSurfaceAlphaMod(splash_surf, 255);
#else
				SDL_SetAlpha(splash_surf, SDL_SRCALPHA,255);
#endif
				SDL_BlitSurface(splash_surf, NULL, sdl.surface, NULL);
#if SDL_VERSION_ATLEAST(2,0,0)
				SDL_UpdateWindowSurface(sdl.window);
#else
				SDL_Flip(sdl.surface);
#endif
			} else if (ct>=max_splash_loop-splash_fade) {
				if (use_fadeout) {
					SDL_FillRect(sdl.surface, NULL, SDL_MapRGB(sdl.surface->format, 0, 0, 0));
#if SDL_VERSION_ATLEAST(2,0,0)
					SDL_SetSurfaceAlphaMod(splash_surf, (Bit8u)((max_splash_loop-1-ct)*255/(splash_fade-1)));
#else
					SDL_SetAlpha(splash_surf, SDL_SRCALPHA, (Bit8u)((max_splash_loop-1-ct)*255/(splash_fade-1)));
#endif
					SDL_BlitSurface(splash_surf, NULL, sdl.surface, NULL);
#if SDL_VERSION_ATLEAST(2,0,0)
					SDL_UpdateWindowSurface(sdl.window);
#else
					SDL_Flip(sdl.surface);
#endif
				}
			} else { // Fix a possible glitch
#if SDL_VERSION_ATLEAST(2,0,0)
				SDL_UpdateWindowSurface(sdl.window);
#else
				SDL_Flip(sdl.surface);
#endif
			}

		}

		if (use_fadeout) {
			SDL_FillRect(sdl.surface, NULL, SDL_MapRGB(sdl.surface->format, 0, 0, 0));
#if SDL_VERSION_ATLEAST(2,0,0)
			SDL_UpdateWindowSurface(sdl.window);
#else
			SDL_Flip(sdl.surface);
#endif
		}
		SDL_FreeSurface(splash_surf);
		delete [] tmpbufp;

	}

	/* Get some Event handlers */
	MAPPER_AddHandler(KillSwitch,MK_f9,MMOD1,"shutdown","ShutDown");
	MAPPER_AddHandler(CaptureMouse,MK_f10,MMOD1,"capmouse","Cap Mouse");
	MAPPER_AddHandler(SwitchFullScreen,MK_return,MMOD2,"fullscr","Fullscreen");
	MAPPER_AddHandler(Restart,MK_home,MMOD1|MMOD2,"restart","Restart");
#if C_DEBUG
	/* Pause binds with activate-debugger */
#else
	MAPPER_AddHandler(&PauseDOSBox, MK_pause, MMOD2, "pause", "Pause");
#endif
	/* Get Keyboard state of numlock and capslock */
#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_Keymod keystate = SDL_GetModState();
#else
	SDLMod keystate = SDL_GetModState();
#endif
	if(keystate&KMOD_NUM) startup_state_numlock = true;
	if(keystate&KMOD_CAPS) startup_state_capslock = true;
}

void Mouse_AutoLock(bool enable) {
	sdl.mouse.autolock=enable;
	if (sdl.mouse.autoenable) sdl.mouse.requestlock=enable;
	else {
		SDL_ShowCursor(enable?SDL_DISABLE:SDL_ENABLE);
		sdl.mouse.requestlock=false;
	}
}

#if defined(ANDROID)
/* The way mouse emulation is done here is based on the
following (horizontal) partitioning of the touchscreen:

/-----------------------------------------------------------------------\
|    Left   | (H)Escape |   Motion  |  Motion   |   Middle  |   Right   |
\-----------------------------------------------------------------------/

Note that the simulated Escape key is the host one
(not the emulator's), so it can be re-mapped.
Furthermore, that should be considered a hack for now. */
static void HandleTouchFinger(SDL_TouchFingerEvent * tfinger) {
	void MAPPER_CheckEvent(SDL_Event * event);
	switch (tfinger->type) {
		case SDL_FINGERDOWN:
			if (tfinger->x >= 0.83f) { // Right button
				sdl.mouse.rightMouseFingerID = tfinger->fingerId;
				if (!sdl.mouse.isRightMouseFingerUsed) {
					Mouse_ButtonPressed(1);
					sdl.mouse.isRightMouseFingerUsed = true;
				}
			} else if (tfinger->x >= 0.67f) { // Middle button
				sdl.mouse.middleMouseFingerID = tfinger->fingerId;
				if (!sdl.mouse.isMiddleMouseFingerUsed) {
					Mouse_ButtonPressed(2);
					sdl.mouse.isMiddleMouseFingerUsed = true;
				}
			} else if (tfinger->x >= 0.33f) { // Motion
				sdl.mouse.mouseMotionFingerID = tfinger->fingerId;
			} else if (tfinger->x >= 0.17f) { // (Host) Escape key
				sdl.mouse.escKeyFingerID = tfinger->fingerId;
				if (!sdl.mouse.isEscKeyFingerUsed) {
					SDL_Event event;
					event.type = SDL_KEYDOWN;
					event.key.keysym.scancode = SDL_SCANCODE_ESCAPE;
					MAPPER_CheckEvent(&event);
					sdl.mouse.isEscKeyFingerUsed = true;
				}
			} else { // Left button
				sdl.mouse.leftMouseFingerID = tfinger->fingerId;
				if (!sdl.mouse.isLeftMouseFingerUsed) {
					Mouse_ButtonPressed(0);
					sdl.mouse.isLeftMouseFingerUsed = true;
				}
			}
			break;
		case SDL_FINGERUP:
			if ((sdl.mouse.leftMouseFingerID == tfinger->fingerId) && sdl.mouse.isLeftMouseFingerUsed) {
				Mouse_ButtonReleased(0);
				sdl.mouse.isLeftMouseFingerUsed = false;
			} else if ((sdl.mouse.rightMouseFingerID == tfinger->fingerId) && sdl.mouse.isRightMouseFingerUsed) {
				Mouse_ButtonReleased(1);
				sdl.mouse.isRightMouseFingerUsed = false;
			} else if ((sdl.mouse.middleMouseFingerID == tfinger->fingerId) && sdl.mouse.isMiddleMouseFingerUsed) {
				Mouse_ButtonReleased(2);
				sdl.mouse.isMiddleMouseFingerUsed = false;
			} else if ((sdl.mouse.escKeyFingerID == tfinger->fingerId) && sdl.mouse.isEscKeyFingerUsed) {
				SDL_Event event;
				event.type = SDL_KEYUP;
				event.key.keysym.scancode = SDL_SCANCODE_ESCAPE;
				MAPPER_CheckEvent(&event);
				sdl.mouse.isEscKeyFingerUsed = false;
			}
			break;
		case SDL_FINGERMOTION:
			/* We basically IGNORE the absolute coordinates, since
			we emulate relative mouse movement all the time.    */
			if (sdl.mouse.mouseMotionFingerID == tfinger->fingerId) {
				Mouse_CursorMoved((float)tfinger->dx*sdl.clip.w*sdl.mouse.sensitivity/100.0f,
				                  (float)tfinger->dy*sdl.clip.h*sdl.mouse.sensitivity/100.0f,
				                  0, 0, true);
			}
			break;
	}
}

static void HandleMouseMotion(SDL_MouseMotionEvent * motion) {
//	if (sdl.mouse.locked || !sdl.mouse.autoenable)
		Mouse_CursorMoved((float)motion->xrel*sdl.mouse.sensitivity/100.0f,
						  (float)motion->yrel*sdl.mouse.sensitivity/100.0f,
						  (float)(motion->x-sdl.clip.x)/(sdl.clip.w-1)*sdl.mouse.sensitivity/100.0f,
						  (float)(motion->y-sdl.clip.y)/(sdl.clip.h-1)*sdl.mouse.sensitivity/100.0f, 0);
}
#else						  
static void HandleMouseMotion(SDL_MouseMotionEvent * motion) {
	if (sdl.mouse.locked || !sdl.mouse.autoenable)
		Mouse_CursorMoved((float)motion->xrel*sdl.mouse.sensitivity/100.0f,
						  (float)motion->yrel*sdl.mouse.sensitivity/100.0f,
						  (float)(motion->x-sdl.clip.x)/(sdl.clip.w-1)*sdl.mouse.sensitivity/100.0f,
						  (float)(motion->y-sdl.clip.y)/(sdl.clip.h-1)*sdl.mouse.sensitivity/100.0f,
						  sdl.mouse.locked);
}
#endif	// End of touch/mouse differentiation


static void HandleMouseButton(SDL_MouseButtonEvent * button) {
	switch (button->state) {
	case SDL_PRESSED:
		if (sdl.mouse.requestlock && !sdl.mouse.locked) {
			GFX_CaptureMouse();
			// Dont pass klick to mouse handler
			break;
		}
		if (!sdl.mouse.autoenable && sdl.mouse.autolock && button->button == SDL_BUTTON_MIDDLE) {
			GFX_CaptureMouse();
			break;
		}
		switch (button->button) {
		case SDL_BUTTON_LEFT:
			Mouse_ButtonPressed(0);
			break;
		case SDL_BUTTON_RIGHT:
			Mouse_ButtonPressed(1);
			break;
		case SDL_BUTTON_MIDDLE:
			Mouse_ButtonPressed(2);
			break;
		}
		break;
	case SDL_RELEASED:
		switch (button->button) {
		case SDL_BUTTON_LEFT:
			Mouse_ButtonReleased(0);
			break;
		case SDL_BUTTON_RIGHT:
			Mouse_ButtonReleased(1);
			break;
		case SDL_BUTTON_MIDDLE:
			Mouse_ButtonReleased(2);
			break;
		}
		break;
	}
}

void GFX_LosingFocus(void) {
	sdl.laltstate=SDL_KEYUP;
	sdl.raltstate=SDL_KEYUP;
	MAPPER_LosingFocus();
}

bool GFX_IsFullscreen(void) {
	return sdl.desktop.fullscreen;
}

#if SDL_VERSION_ATLEAST(2,0,0)
void GFX_HandleVideoResize(int width, int height) {
	/* Maybe a screen rotation has just occurred, so we simply resize.
	There may be a different cause for a forced resized, though.    */
	if (sdl.desktop.full.display_res && sdl.desktop.fullscreen) {
		/* Note: We should not use GFX_ObtainDisplayDimensions
		(SDL_GetDisplayBounds) on Android after a screen rotation:
		The older values from application startup are returned. */
		sdl.desktop.full.width = width;
		sdl.desktop.full.height = height;
	}
	/* Even if the new window's dimensions are actually the desired ones
	 * we may still need to re-obtain a new window surface or do
	 * a different thing. So we basically call GFX_SetSize, but without
	 * touching the window itself (or else we may end in an infinite loop).
	 *
	 * Furthermore, if the new dimensions are *not* the desired ones, we
	 * don't fight it. Rather than attempting to resize it back, we simply
	 * keep the window as-is and disable screen updates. This is done
	 * in SDL_SetSDLWindowSurface by setting sdl.update_display_contents
	 * to false.
	 */
	sdl.update_window = false;
	GFX_ResetScreen();
	sdl.update_window = true;
}
#endif

void GFX_Events() {
	SDL_Event event;
#if defined (REDUCE_JOYSTICK_POLLING)
	static int poll_delay=0;
	int time=GetTicks();
	if (time-poll_delay>20) {
		poll_delay=time;
		if (sdl.num_joysticks>0) SDL_JoystickUpdate();
		MAPPER_UpdateJoysticks();
	}
#endif
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
#if SDL_VERSION_ATLEAST(2,0,0)
		case SDL_WINDOWEVENT:
			switch (event.window.event) {
				case SDL_WINDOWEVENT_RESTORED:
					/* We may need to re-create a texture
					 * and more on Android. Another case:
					 * Update surface while using X11.
					 */
					GFX_ResetScreen();
					continue;
				case SDL_WINDOWEVENT_RESIZED:
					GFX_HandleVideoResize(event.window.data1, event.window.data2);
					continue;
				case SDL_WINDOWEVENT_EXPOSED:
					if (sdl.draw.callback) sdl.draw.callback( GFX_CallBackRedraw );
					continue;
				case SDL_WINDOWEVENT_FOCUS_GAINED:
					if (sdl.desktop.fullscreen && !sdl.mouse.locked)
						GFX_CaptureMouse();
					SetPriority(sdl.priority.focus);
					CPU_Disable_SkipAutoAdjust();
					break;
				case SDL_WINDOWEVENT_FOCUS_LOST:
					if (sdl.mouse.locked) {
#ifdef WIN32
						if (sdl.desktop.fullscreen) {
							VGA_KillDrawing();
							GFX_ForceFullscreenExit();
						}
#endif
						GFX_CaptureMouse();
					}
					SetPriority(sdl.priority.nofocus);
					GFX_LosingFocus();
					CPU_Enable_SkipAutoAdjust();
					break;
				default: ;
			}

			/* Non-focus priority is set to pause; check to see if we've lost window or input focus
			 * i.e. has the window been minimised or made inactive?
			 */
			if (sdl.priority.nofocus == PRIORITY_LEVEL_PAUSE) {
				if ((event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) || (event.window.event == SDL_WINDOWEVENT_MINIMIZED)) {
					/* Window has lost focus, pause the emulator.
					 * This is similar to what PauseDOSBox() does, but the exit criteria is different.
					 * Instead of waiting for the user to hit Alt-Break, we wait for the window to
					 * regain window or input focus.
					 */
					bool paused = true;
					SDL_Event ev;

					GFX_SetTitle(-1,-1,true);
					KEYBOARD_ClrBuffer();
//					SDL_Delay(500);
//					while (SDL_PollEvent(&ev)) {
						// flush event queue.
//					}

					while (paused) {
						// WaitEvent waits for an event rather than polling, so CPU usage drops to zero
						SDL_WaitEvent(&ev);

						switch (ev.type) {
						case SDL_QUIT: throw(0); break; // a bit redundant at linux at least as the active events gets before the quit event.
						case SDL_WINDOWEVENT:     // wait until we get window focus back
							if ((ev.window.event == SDL_WINDOWEVENT_FOCUS_LOST) || (ev.window.event == SDL_WINDOWEVENT_MINIMIZED) || (ev.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) || (ev.window.event == SDL_WINDOWEVENT_RESTORED) || (ev.window.event == SDL_WINDOWEVENT_EXPOSED)) {
								// We've got focus back, so unpause and break out of the loop
								if ((ev.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) || (ev.window.event == SDL_WINDOWEVENT_RESTORED) || (ev.window.event == SDL_WINDOWEVENT_EXPOSED)) {
									paused = false;
									GFX_SetTitle(-1,-1,false);
								}

								/* Now poke a "release ALT" command into the keyboard buffer
								 * we have to do this, otherwise ALT will 'stick' and cause
								 * problems with the app running in the DOSBox.
								 */
								KEYBOARD_AddKey(KBD_leftalt, false);
								KEYBOARD_AddKey(KBD_rightalt, false);
								if (ev.window.event == SDL_WINDOWEVENT_RESTORED) {
									// We may need to re-create a texture and more
									GFX_ResetScreen();
								}
							}
							break;
						}
					}
				}
			}
			break;
#endif	// SDL_VERSION_ATLEAST(2,0,0)
#if !SDL_VERSION_ATLEAST(2,0,0)
		case SDL_ACTIVEEVENT:
			if (event.active.state & SDL_APPINPUTFOCUS) {
				if (event.active.gain) {
					if (sdl.desktop.fullscreen && !sdl.mouse.locked)
						GFX_CaptureMouse();
					SetPriority(sdl.priority.focus);
					CPU_Disable_SkipAutoAdjust();
				} else {
					if (sdl.mouse.locked) {
#ifdef WIN32
						if (sdl.desktop.fullscreen) {
							VGA_KillDrawing();
							GFX_ForceFullscreenExit();
						}
#endif
						GFX_CaptureMouse();
					}
					SetPriority(sdl.priority.nofocus);
					GFX_LosingFocus();
					CPU_Enable_SkipAutoAdjust();
				}
			}

			/* Non-focus priority is set to pause; check to see if we've lost window or input focus
			 * i.e. has the window been minimised or made inactive?
			 */
			if (sdl.priority.nofocus == PRIORITY_LEVEL_PAUSE) {
				if ((event.active.state & (SDL_APPINPUTFOCUS | SDL_APPACTIVE)) && (!event.active.gain)) {
					/* Window has lost focus, pause the emulator.
					 * This is similar to what PauseDOSBox() does, but the exit criteria is different.
					 * Instead of waiting for the user to hit Alt-Break, we wait for the window to
					 * regain window or input focus.
					 */
					bool paused = true;
					SDL_Event ev;

					GFX_SetTitle(-1,-1,true);
					KEYBOARD_ClrBuffer();
//					SDL_Delay(500);
//					while (SDL_PollEvent(&ev)) {
						// flush event queue.
//					}

					while (paused) {
						// WaitEvent waits for an event rather than polling, so CPU usage drops to zero
						SDL_WaitEvent(&ev);

						switch (ev.type) {
						case SDL_QUIT: throw(0); break; // a bit redundant at linux at least as the active events gets before the quit event.
						case SDL_ACTIVEEVENT:     // wait until we get window focus back
							if (ev.active.state & (SDL_APPINPUTFOCUS | SDL_APPACTIVE)) {
								// We've got focus back, so unpause and break out of the loop
								if (ev.active.gain) {
									paused = false;
									GFX_SetTitle(-1,-1,false);
								}

								/* Now poke a "release ALT" command into the keyboard buffer
								 * we have to do this, otherwise ALT will 'stick' and cause
								 * problems with the app running in the DOSBox.
								 */
								KEYBOARD_AddKey(KBD_leftalt, false);
								KEYBOARD_AddKey(KBD_rightalt, false);
							}
							break;
						}
					}
				}
			}
			break;
#endif	// !SDL_VERSION_ATLEAST(2,0,0)
#if defined(ANDROID)
		case SDL_FINGERDOWN:
		case SDL_FINGERUP:
		case SDL_FINGERMOTION:
			HandleTouchFinger(&event.tfinger);
			break;
#endif
		case SDL_MOUSEMOTION:
			HandleMouseMotion(&event.motion);
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			HandleMouseButton(&event.button);
			break;
#if !SDL_VERSION_ATLEAST(2,0,0)
		case SDL_VIDEORESIZE:
			//GFX_HandleVideoResize(event.resize.w, event.resize.h);
			break;
#endif
		case SDL_QUIT:
			throw(0);
			break;
#if !SDL_VERSION_ATLEAST(2,0,0)
		case SDL_VIDEOEXPOSE:
			if (sdl.draw.callback) sdl.draw.callback( GFX_CallBackRedraw );
			break;
#endif
#ifdef WIN32
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			// ignore event alt+tab
			if (event.key.keysym.sym==SDLK_LALT) sdl.laltstate = event.key.type;
			if (event.key.keysym.sym==SDLK_RALT) sdl.raltstate = event.key.type;
			if (((event.key.keysym.sym==SDLK_TAB)) &&
				((sdl.laltstate==SDL_KEYDOWN) || (sdl.raltstate==SDL_KEYDOWN))) break;
#endif
#if defined (MACOSX)			
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			/* On macs CMD-Q is the default key to close an application */
			if (event.key.keysym.sym == SDLK_q && (event.key.keysym.mod == KMOD_RMETA || event.key.keysym.mod == KMOD_LMETA) ) {
				KillSwitch(true);
				break;
			} 
#endif
		default:
			void MAPPER_CheckEvent(SDL_Event * event);
			MAPPER_CheckEvent(&event);
		}
	}
}

#if defined (WIN32)
static BOOL WINAPI ConsoleEventHandler(DWORD event) {
	switch (event) {
	case CTRL_SHUTDOWN_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_BREAK_EVENT:
		raise(SIGTERM);
		return TRUE;
	case CTRL_C_EVENT:
	default: //pass to the next handler
		return FALSE;
	}
}
#endif


/* static variable to show wether there is not a valid stdout.
 * Fixes some bugs when -noconsole is used in a read only directory */
static bool no_stdout = false;
void GFX_ShowMsg(char const* format,...) {
	char buf[512];
	va_list msg;
	va_start(msg,format);
	vsprintf(buf,format,msg);
        strcat(buf,"\n");
	va_end(msg);
#ifdef ANDROID
	__android_log_print(ANDROID_LOG_DEBUG, "DOSBox", "%s", buf);
#else
	if(!no_stdout) printf("%s",buf); //Else buf is parsed again.
#endif
}


void Config_Add_SDL() {
	Section_prop * sdl_sec=control->AddSection_prop("sdl",&GUI_StartUp);
	sdl_sec->AddInitFunction(&MAPPER_StartUp);
	Prop_bool* Pbool;
	Prop_string* Pstring;
	Prop_int* Pint;
	Prop_multival* Pmulti;

#ifndef ANDROID
	Pbool = sdl_sec->Add_bool("fullscreen",Property::Changeable::Always,false);
	Pbool->Set_help("Start dosbox directly in fullscreen. (Press ALT-Enter to go back)");
#endif
     
	Pbool = sdl_sec->Add_bool("vsync",Property::Changeable::Always,false);
	Pbool->Set_help("Sync to Vblank IF supported by the output device and renderer (if relevant).\n"
	                "It can reduce screen flickering, but it can also result in a slow DOSBox.");

	Pstring = sdl_sec->Add_string("fullresolution",Property::Changeable::Always,"0x0");
	Pstring->Set_help("What resolution to use for fullscreen: original, desktop or a fixed size (e.g. 1024x768).\n"
	                  "  Using your monitor's native resolution with aspect=true might give the best results.\n"
			  "  If you end up with small window on a large screen, try an output different from surface.");

#ifndef ANDROID
	Pstring = sdl_sec->Add_string("windowresolution",Property::Changeable::Always,"original");
	Pstring->Set_help("Scale the window to this size IF the output device supports hardware scaling.\n"
	                  "  (output=surface does not!)");
#endif

	const char* outputs[] = {
		"surface",
#if SDL_VERSION_ATLEAST(2, 0, 0)
		"texture",
		"texturenb",
#else	// !SDL_VERSION_ATLEAST(2, 0, 0)
		"overlay",
#if (HAVE_DDRAW_H) && defined(WIN32)
		"ddraw",
#endif
#endif	// !SDL_VERSION_ATLEAST(2, 0, 0)
#if C_OPENGL
		"opengl", "openglnb",
#endif
		0 };
#if SDL_VERSION_ATLEAST(2, 0, 0)
	Pstring = sdl_sec->Add_string("output",Property::Changeable::Always,"texture");
#else
	Pstring = sdl_sec->Add_string("output",Property::Changeable::Always,"surface");
#endif
	Pstring->Set_help("What video system to use for output.");
	Pstring->Set_values(outputs);

#if SDL_VERSION_ATLEAST(2,0,0)
	const char* renderers[] = {
		"auto",
#ifdef WIN32
		"direct3d",
#endif
#ifdef ANDROID
		"opengles2",
		"opengles",
#else	// On any platform other than Android
		"opengl",
#endif
		"software",
		0 };
	Pstring = sdl_sec->Add_string("renderer",Property::Changeable::Always,"auto");
	Pstring->Set_help("Choose a renderer driver if output=texture or output=texturenb. Use output=auto for an automatic choice.");
	Pstring->Set_values(renderers);
#endif	// SDL_VERSION_ATLEAST(2,0,0)

	Pbool = sdl_sec->Add_bool("autolock",Property::Changeable::Always,true);
	Pbool->Set_help("Mouse will automatically lock, if you click on the screen. (Press CTRL-F10 to unlock)");

	Pint = sdl_sec->Add_int("sensitivity",Property::Changeable::Always,100);
	Pint->SetMinMax(1,1000);
	Pint->Set_help("Mouse sensitivity.");

	Pbool = sdl_sec->Add_bool("waitonerror",Property::Changeable::Always, true);
	Pbool->Set_help("Wait before closing the console if dosbox has an error.");

	Pmulti = sdl_sec->Add_multi("priority", Property::Changeable::Always, ",");
	Pmulti->SetValue("higher,normal");
	Pmulti->Set_help("Priority levels for dosbox. Second entry behind the comma is for when dosbox is not focused/minimized.\n"
	                 "  pause is only valid for the second entry.");

	const char* actt[] = { "lowest", "lower", "normal", "higher", "highest", "pause", 0};
	Pstring = Pmulti->GetSection()->Add_string("active",Property::Changeable::Always,"higher");
	Pstring->Set_values(actt);

	const char* inactt[] = { "lowest", "lower", "normal", "higher", "highest", "pause", 0};
	Pstring = Pmulti->GetSection()->Add_string("inactive",Property::Changeable::Always,"normal");
	Pstring->Set_values(inactt);

	Pstring = sdl_sec->Add_path("mapperfile",Property::Changeable::Always,MAPPERFILE);
	Pstring->Set_help("File used to load/save the key/event mappings from. Resetmapper only works with the default value.");

#if !SDL_VERSION_ATLEAST(2,0,0)
	Pbool = sdl_sec->Add_bool("usescancodes",Property::Changeable::Always,true);
	Pbool->Set_help("Avoid usage of symkeys, might not work on all operating systems.");
#endif
}

static void show_warning(char const * const message) {
	bool textonly = true;
#ifdef WIN32
	textonly = false;
	if ( !sdl.inited && SDL_Init(SDL_INIT_VIDEO|SDL_INIT_NOPARACHUTE) < 0 ) textonly = true;
	sdl.inited = true;
#endif
	printf("%s", message);
	if(textonly) return;
#if SDL_VERSION_ATLEAST(2, 0, 0)
	if (!sdl.window)
		if (!GFX_SetSDLSurfaceWindow(640,400)) return;
	sdl.surface = SDL_GetWindowSurface(sdl.window);
#else
	if(!sdl.surface) sdl.surface = SDL_SetVideoMode(640,400,0,0);
#endif
	if(!sdl.surface) return;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	Bit32u rmask = 0xff000000;
	Bit32u gmask = 0x00ff0000;
	Bit32u bmask = 0x0000ff00;
#else
	Bit32u rmask = 0x000000ff;
	Bit32u gmask = 0x0000ff00;                    
	Bit32u bmask = 0x00ff0000;
#endif
	SDL_Surface* splash_surf = SDL_CreateRGBSurface(SDL_SWSURFACE, 640, 400, 32, rmask, gmask, bmask, 0);
	if (!splash_surf) return;

	int x = 120,y = 20;
	std::string m(message),m2;
	std::string::size_type a,b,c,d;
   
	while(m.size()) { //Max 50 characters. break on space before or on a newline
		c = m.find('\n');
		d = m.rfind(' ',50);
		if(c>d) a=b=d; else a=b=c;
		if( a != std::string::npos) b++; 
		m2 = m.substr(0,a); m.erase(0,b);
		OutputString(x,y,m2.c_str(),0xffffffff,0,splash_surf);
		y += 20;
	}
   
	SDL_BlitSurface(splash_surf, NULL, sdl.surface, NULL);
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_UpdateWindowSurface(sdl.window);
#else
	SDL_Flip(sdl.surface);
#endif
	SDL_Delay(12000);
}
   
static void launcheditor() {
	std::string path,file;
	Cross::CreatePlatformConfigDir(path);
	Cross::GetPlatformConfigName(file);
	path += file;
	FILE* f = fopen(path.c_str(),"r");
	if(!f && !control->PrintConfig(path.c_str())) {
		printf("tried creating %s. but failed.\n",path.c_str());
		exit(1);
	}
	if(f) fclose(f);
/*	if(edit.empty()) {
		printf("no editor specified.\n");
		exit(1);
	}*/
	std::string edit;
	while(control->cmdline->FindString("-editconf",edit,true)) //Loop until one succeeds
		execlp(edit.c_str(),edit.c_str(),path.c_str(),(char*) 0);
	//if you get here the launching failed!
	printf("can't find editor(s) specified at the command line.\n");
	exit(1);
}
#if C_DEBUG
extern void DEBUG_ShutDown(Section * /*sec*/);
#endif

void restart_program(std::vector<std::string> & parameters) {
	char** newargs = new char* [parameters.size()+1];
	// parameter 0 is the executable path
	// contents of the vector follow
	// last one is NULL
	for(Bitu i = 0; i < parameters.size(); i++) newargs[i]=(char*)parameters[i].c_str();
	newargs[parameters.size()] = NULL;
	SDL_CloseAudio();
	SDL_Delay(50);
	SDL_Quit();
#if C_DEBUG
	// shutdown curses
	DEBUG_ShutDown(NULL);
#endif

	execvp(newargs[0], newargs);
	free(newargs);
}
void Restart(bool pressed) { // mapper handler
	restart_program(control->startup_params);
}

static void launchcaptures(std::string const& edit) {
	std::string path,file;
	Section* t = control->GetSection("dosbox");
	if(t) file = t->GetPropValue("captures");
	if(!t || file == NO_SUCH_PROPERTY) {
		printf("Config system messed up.\n");
		exit(1);
	}
	Cross::CreatePlatformConfigDir(path);
	path += file;
	Cross::CreateDir(path);
	struct stat cstat;
	if(stat(path.c_str(),&cstat) || (cstat.st_mode & S_IFDIR) == 0) {
		printf("%s doesn't exists or isn't a directory.\n",path.c_str());
		exit(1);
	}
/*	if(edit.empty()) {
		printf("no editor specified.\n");
		exit(1);
	}*/

	execlp(edit.c_str(),edit.c_str(),path.c_str(),(char*) 0);
	//if you get here the launching failed!
	printf("can't find filemanager %s\n",edit.c_str());
	exit(1);
}

static void printconfiglocation() {
	std::string path,file;
	Cross::CreatePlatformConfigDir(path);
	Cross::GetPlatformConfigName(file);
	path += file;
     
	FILE* f = fopen(path.c_str(),"r");
	if(!f && !control->PrintConfig(path.c_str())) {
		printf("tried creating %s. but failed",path.c_str());
		exit(1);
	}
	if(f) fclose(f);
	printf("%s\n",path.c_str());
	exit(0);
}

static void eraseconfigfile() {
	FILE* f = fopen("dosbox.conf","r");
	if(f) {
		fclose(f);
		show_warning("Warning: dosbox.conf exists in current working directory.\nThis will override the configuration file at runtime.\n");
	}
	std::string path,file;
	Cross::GetPlatformConfigDir(path);
	Cross::GetPlatformConfigName(file);
	path += file;
	f = fopen(path.c_str(),"r");
	if(!f) exit(0);
	fclose(f);
	unlink(path.c_str());
	exit(0);
}

static void erasemapperfile() {
	FILE* g = fopen("dosbox.conf","r");
	if(g) {
		fclose(g);
		show_warning("Warning: dosbox.conf exists in current working directory.\nKeymapping might not be properly reset.\n"
		             "Please reset configuration as well and delete the dosbox.conf.\n");
	}

	std::string path,file=MAPPERFILE;
	Cross::GetPlatformConfigDir(path);
	path += file;
	FILE* f = fopen(path.c_str(),"r");
	if(!f) exit(0);
	fclose(f);
	unlink(path.c_str());
	exit(0);
}


//extern void UI_Init(void);
int main(int argc, char* argv[]) {
	try {
		CommandLine com_line(argc,argv);
		Config myconf(&com_line);
		control=&myconf;
		/* Init the configuration system and add default values */
		Config_Add_SDL();
		DOSBOX_Init();

		std::string editor;
		if(control->cmdline->FindString("-editconf",editor,false)) launcheditor();
		if(control->cmdline->FindString("-opencaptures",editor,true)) launchcaptures(editor);
		if(control->cmdline->FindExist("-eraseconf")) eraseconfigfile();
		if(control->cmdline->FindExist("-resetconf")) eraseconfigfile();
		if(control->cmdline->FindExist("-erasemapper")) erasemapperfile();
		if(control->cmdline->FindExist("-resetmapper")) erasemapperfile();
		
		/* Can't disable the console with debugger enabled */
#if defined(WIN32) && !(C_DEBUG)
		if (control->cmdline->FindExist("-noconsole")) {
			FreeConsole();
			/* Redirect standard input and standard output */
			if(freopen(STDOUT_FILE, "w", stdout) == NULL)
				no_stdout = true; // No stdout so don't write messages
			freopen(STDERR_FILE, "w", stderr);
			setvbuf(stdout, NULL, _IOLBF, BUFSIZ);	/* Line buffered */
			setbuf(stderr, NULL);					/* No buffering */
		} else {
			if (AllocConsole()) {
				fclose(stdin);
				fclose(stdout);
				fclose(stderr);
				freopen("CONIN$","r",stdin);
				freopen("CONOUT$","w",stdout);
				freopen("CONOUT$","w",stderr);
			}
			SetConsoleTitle("DOSBox Status Window");
		}
#endif  //defined(WIN32) && !(C_DEBUG)
		if (control->cmdline->FindExist("-version") ||
		    control->cmdline->FindExist("--version") ) {
			printf("\nDOSBox version %s, copyright 2002-2013 DOSBox Team.\n\n",VERSION);
			printf("DOSBox is written by the DOSBox Team (See AUTHORS file))\n");
			printf("DOSBox comes with ABSOLUTELY NO WARRANTY.  This is free software,\n");
			printf("and you are welcome to redistribute it under certain conditions;\n");
			printf("please read the COPYING file thoroughly before doing so.\n\n");
			return 0;
		}
		if(control->cmdline->FindExist("-printconf")) printconfiglocation();

#if C_DEBUG
		DEBUG_SetupConsole();
#endif

#if defined(WIN32)
	SetConsoleCtrlHandler((PHANDLER_ROUTINE) ConsoleEventHandler,TRUE);
#endif

#ifdef OS2
        PPIB pib;
        PTIB tib;
        DosGetInfoBlocks(&tib, &pib);
        if (pib->pib_ultype == 2) pib->pib_ultype = 3;
        setbuf(stdout, NULL);
        setbuf(stderr, NULL);
#endif

	/* Display Welcometext in the console */
	LOG_MSG("DOSBox version %s",VERSION);
	LOG_MSG("Copyright 2002-2013 DOSBox Team, published under GNU GPL.");
	LOG_MSG("---");

	/* Init SDL */
#if SDL_VERSION_ATLEAST(1, 2, 14)
	/* Or debian/ubuntu with older libsdl version as they have done this themselves, but then differently.
	 * with this variable they will work correctly. I've only tested the 1.2.14 behaviour against the windows version
	 * of libsdl
	 */
	putenv(const_cast<char*>("SDL_DISABLE_LOCK_KEYS=1"));
#endif
	if ( SDL_Init( SDL_INIT_AUDIO|SDL_INIT_VIDEO|SDL_INIT_TIMER
#if !SDL_VERSION_ATLEAST(2,0,0)
		|SDL_INIT_CDROM
#endif
		|SDL_INIT_NOPARACHUTE
		) < 0 ) E_Exit("Can't init SDL %s",SDL_GetError());
	sdl.inited = true;

#ifndef DISABLE_JOYSTICK
	//Initialise Joystick seperately. This way we can warn when it fails instead
	//of exiting the application
	if( SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0 ) LOG_MSG("Failed to init joystick support");
#endif

	sdl.laltstate = SDL_KEYUP;
	sdl.raltstate = SDL_KEYUP;

#if !SDL_VERSION_ATLEAST(2, 0, 0) // Not relevant for SDL 2.0 at the moment
#if defined (WIN32)
#if SDL_VERSION_ATLEAST(1, 2, 10)
		sdl.using_windib=true;
#else
		sdl.using_windib=false;
#endif
		char sdl_drv_name[128];
		if (getenv("SDL_VIDEODRIVER")==NULL) {
			if (SDL_VideoDriverName(sdl_drv_name,128)!=NULL) {
				sdl.using_windib=false;
				if (strcmp(sdl_drv_name,"directx")!=0) {
					SDL_QuitSubSystem(SDL_INIT_VIDEO);
					putenv("SDL_VIDEODRIVER=directx");
					if (SDL_InitSubSystem(SDL_INIT_VIDEO)<0) {
						putenv("SDL_VIDEODRIVER=windib");
						if (SDL_InitSubSystem(SDL_INIT_VIDEO)<0) E_Exit("Can't init SDL Video %s",SDL_GetError());
						sdl.using_windib=true;
					}
				}
			}
		} else {
			char* sdl_videodrv = getenv("SDL_VIDEODRIVER");
			if (strcmp(sdl_videodrv,"directx")==0) sdl.using_windib = false;
			else if (strcmp(sdl_videodrv,"windib")==0) sdl.using_windib = true;
		}
		if (SDL_VideoDriverName(sdl_drv_name,128)!=NULL) {
			if (strcmp(sdl_drv_name,"windib")==0) LOG_MSG("SDL_Init: Starting up with SDL windib video driver.\n          Try to update your video card and directx drivers!");
		}
#endif	// WIN32
#endif	// !SDL_VERSION_ATLEAST(2, 0, 0)
	sdl.num_joysticks=SDL_NumJoysticks();

	/* Parse configuration files */
	std::string config_file,config_path;
	Cross::GetPlatformConfigDir(config_path);
	
	//First parse -userconf
	if(control->cmdline->FindExist("-userconf",true)){
		config_file.clear();
		Cross::GetPlatformConfigDir(config_path);
		Cross::GetPlatformConfigName(config_file);
		config_path += config_file;
		control->ParseConfigFile(config_path.c_str());
		if(!control->configfiles.size()) {
			//Try to create the userlevel configfile.
			config_file.clear();
			Cross::CreatePlatformConfigDir(config_path);
			Cross::GetPlatformConfigName(config_file);
			config_path += config_file;
			if(control->PrintConfig(config_path.c_str())) {
				LOG_MSG("CONFIG: Generating default configuration.\nWriting it to %s",config_path.c_str());
				//Load them as well. Makes relative paths much easier
				control->ParseConfigFile(config_path.c_str());
			}
		}
	}

	//Second parse -conf switches
	while(control->cmdline->FindString("-conf",config_file,true)) {
		if(!control->ParseConfigFile(config_file.c_str())) {
			// try to load it from the user directory
			control->ParseConfigFile((config_path + config_file).c_str());
		}
	}
	// if none found => parse localdir conf
	if(!control->configfiles.size()) control->ParseConfigFile("dosbox.conf");

	// if none found => parse userlevel conf
	if(!control->configfiles.size()) {
		config_file.clear();
		Cross::GetPlatformConfigName(config_file);
		control->ParseConfigFile((config_path + config_file).c_str());
	}

	if(!control->configfiles.size()) {
		//Try to create the userlevel configfile.
		config_file.clear();
		Cross::CreatePlatformConfigDir(config_path);
		Cross::GetPlatformConfigName(config_file);
		config_path += config_file;
		if(control->PrintConfig(config_path.c_str())) {
			LOG_MSG("CONFIG: Generating default configuration.\nWriting it to %s",config_path.c_str());
			//Load them as well. Makes relative paths much easier
			control->ParseConfigFile(config_path.c_str());
		} else {
			LOG_MSG("CONFIG: Using default settings. Create a configfile to change them");
		}
	}


#if (ENVIRON_LINKED)
		control->ParseEnv(environ);
#endif
//		UI_Init();
//		if (control->cmdline->FindExist("-startui")) UI_Run(false);
		/* Init all the sections */
		control->Init();
		/* Some extra SDL Functions */
		Section_prop * sdl_sec=static_cast<Section_prop *>(control->GetSection("sdl"));

		if (control->cmdline->FindExist("-fullscreen") || sdl_sec->Get_bool("fullscreen")) {
			if(!sdl.desktop.fullscreen) { //only switch if not already in fullscreen
				GFX_SwitchFullScreen();
			}
		}

		/* Init the keyMapper */
		MAPPER_Init();
		if (control->cmdline->FindExist("-startmapper")) MAPPER_RunInternal();
		/* Start up main machine */
		control->StartUp();
		/* Shutdown everything */
	} catch (char * error) {
#if defined (WIN32)
		sticky_keys(true);
#endif
		GFX_ShowMsg("Exit to error: %s",error);
		fflush(NULL);
		if(sdl.wait_on_error) {
			//TODO Maybe look for some way to show message in linux?
#if (C_DEBUG)
			GFX_ShowMsg("Press enter to continue");
			fflush(NULL);
			fgetc(stdin);
#elif defined(WIN32)
			Sleep(5000);
#endif
		}

	}
	catch (int){
		;//nothing pressed killswitch
	}
	catch(...){
#if defined (WIN32)
		sticky_keys(true);
#endif
		//Force visible mouse to end user. Somehow this sometimes doesn't happen
#if SDL_VERSION_ATLEAST(2,0,0)
		SDL_SetRelativeMouseMode(SDL_FALSE);
#else
		SDL_WM_GrabInput(SDL_GRAB_OFF);
#endif
		SDL_ShowCursor(SDL_ENABLE);
		throw;//dunno what happened. rethrow for sdl to catch
	}
#if defined (WIN32)
	sticky_keys(true); //Might not be needed if the shutdown function switches to windowed mode, but it doesn't hurt
#endif 
	//Force visible mouse to end user. Somehow this sometimes doesn't happen
#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_SetRelativeMouseMode(SDL_FALSE);
#else
	SDL_WM_GrabInput(SDL_GRAB_OFF);
#endif
	SDL_ShowCursor(SDL_ENABLE);

	SDL_Quit();//Let's hope sdl will quit as well when it catches an exception
#ifdef ANDROID
	exit(0); // Actually quits application... and hopefully(?) removes static values
#endif
	return 0;
}

void GFX_GetSize(int &width, int &height, bool &fullscreen) {
	width = sdl.draw.width;
	height = sdl.draw.height;
	fullscreen = sdl.desktop.fullscreen;
}
