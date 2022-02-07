/*************************************************************************/
/*  app_delegate.mm                                                      */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#import "app_delegate.h"

#include "core/project_settings.h"
#include "drivers/coreaudio/audio_driver_coreaudio.h"
#import "godot_view.h"
#include "main/main.h"
#include "os_appletv.h"
#import "view_controller.h"

#import <AudioToolbox/AudioServices.h>

#define kRenderingFrequency 60

extern int gargc;
extern char **gargv;

extern int appletv_main(int, char **, String, String);
extern void appletv_finish();

@implementation AppDelegate

static ViewController *mainViewController = nil;

+ (ViewController *)viewController {
	return mainViewController;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
	// Create a full-screen window
	CGRect windowBounds = [[UIScreen mainScreen] bounds];
	self.window = [[UIWindow alloc] initWithFrame:windowBounds];

	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, //TODO CasheDir, Document access denied!
			NSUserDomainMask, YES);
	NSString *documentsDirectory = [paths objectAtIndex:0];

	paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory,
			NSUserDomainMask, YES);
	NSString *cashDirectory = [paths objectAtIndex:0];

	int err = appletv_main(gargc, gargv, String::utf8([documentsDirectory UTF8String]), String::utf8([cashDirectory UTF8String]));
	if (err != 0) {
		// bail, things did not go very well for us, should probably output a message on screen with our error code...
		exit(0);
		return FALSE;
	}

	// WARNING: We must *always* create the GodotView after we have constructed the
	// OS with appletv_main. This allows the GodotView to access project settings so
	// it can properly initialize the OpenGL context

	ViewController *viewController = [[ViewController alloc] init];
	viewController.godotView.useCADisplayLink = bool(GLOBAL_DEF("display.tvOS/use_cadisplaylink", true)) ? YES : NO;
	viewController.godotView.renderingInterval = 1.0 / kRenderingFrequency;

	self.window.rootViewController = viewController;

	// Show the window
	[self.window makeKeyAndVisible];

	mainViewController = viewController;

	// prevent to stop music in another background app
	[[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryAmbient error:nil];

	bool keep_screen_on = bool(GLOBAL_DEF("display/window/energy_saving/keep_screen_on", true));
	OSAppleTV::get_singleton()->set_keep_screen_on(keep_screen_on);

	return TRUE;
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application {
	if (OS::get_singleton()->get_main_loop()) {
		OS::get_singleton()->get_main_loop()->notification(
				MainLoop::NOTIFICATION_OS_MEMORY_WARNING);
	}
}

- (void)applicationWillTerminate:(UIApplication *)application {
	appletv_finish();
}

// When application goes to background (e.g. user switches to another app or presses Home),
// then applicationWillResignActive -> applicationDidEnterBackground are called.
// When user opens the inactive app again,
// applicationWillEnterForeground -> applicationDidBecomeActive are called.

// There are cases when applicationWillResignActive -> applicationDidBecomeActive
// sequence is called without the app going to background. For example, that happens
// if you open the app list without switching to another app or open/close the
// notification panel by swiping from the upper part of the screen.

- (void)applicationDidEnterBackground:(UIApplication *)application {
	print("*** applicationDidEnterBackground()\n");
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
	print("*** applicationWillEnterForeground()\n");
}

- (void)applicationWillResignActive:(UIApplication *)application {
	OSAppleTV::get_singleton()->on_focus_out();
	print("*** focus out\n");
	print("*** applicationWillResignActive()\n");
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
	print("*** applicationDidBecomeActive()\n");
	print("*** focus in\n");
	OSAppleTV::get_singleton()->on_focus_in();
}

- (void)dealloc {
	self.window = nil;
}

@end
