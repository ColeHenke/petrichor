// Copyright (c) 2013 The Chromium Embedded Framework Authors.
// Portions copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "include/cef_application_mac.h"
#include "include/cef_command_line.h"
#include "include/wrapper/cef_helpers.h"
#include "include/wrapper/cef_library_loader.h"

#include "app/PetrichorApp.h"
#include "browser/BrowserHandler.h"
#include "ui/MainWindow.h"

#include <vector>

#include <QApplication>

// Receives notifications from the application.
@interface BrowserAppDelegate : NSObject <NSApplicationDelegate>
- (void)createApplication:(id)object;
- (void)tryToTerminateApplication:(NSApplication*)app;
@end

// Provide the CefAppProtocol implementation required by CEF.
// CEF on Mac requires that NSApplication conforms to CefAppProtocol
// so it can correctly interleave its message handling with Cocoa's.
@interface BrowserApplication : NSApplication <CefAppProtocol> {
 @private
  BOOL handlingSendEvent_;
}
@end

@implementation BrowserApplication

- (BOOL)isHandlingSendEvent {
  return handlingSendEvent_;
}

- (void)setHandlingSendEvent:(BOOL)handlingSendEvent {
  handlingSendEvent_ = handlingSendEvent;
}

- (void)sendEvent:(NSEvent*)event {
  CefScopedSendingEvent sendingEventScoper;
  [super sendEvent:event];
}

// Override terminate: so CEF gets a chance to shut down cleanly
// before the process exits. See cefsimple for full explanation.
- (void)terminate:(id)sender {
  BrowserAppDelegate* delegate =
      static_cast<BrowserAppDelegate*>([NSApp delegate]);
  [delegate tryToTerminateApplication:self];
}
@end

@implementation BrowserAppDelegate

- (void)createApplication:(id)object {
  [[NSApplication sharedApplication] setDelegate:self];
}

- (void)tryToTerminateApplication:(NSApplication*)app {
  BrowserHandler* handler = BrowserHandler::GetInstance();
  if (handler && !handler->IsClosing()) {
    handler->CloseAllBrowsers(false);
  }
}

- (NSApplicationTerminateReply)applicationShouldTerminate:
    (NSApplication*)sender {
  return NSTerminateNow;
}

- (BOOL)applicationShouldHandleReopen:(NSApplication*)theApplication
                    hasVisibleWindows:(BOOL)flag {
  BrowserHandler* handler = BrowserHandler::GetInstance();
  if (handler && !handler->IsClosing()) {
    handler->ShowMainWindow();
  }
  return NO;
}

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication*)app {
  return YES;
}
@end

// Entry point for the browser process.
// NOTE: Qt will eventually take over the event loop here.
// For now we use CefRunMessageLoop() directly. Once we integrate Qt,
// this will change to run both loops together.
int main(int argc, char* argv[]) {
  // Load CEF framework at runtime (required by macOS sandbox).
  CefScopedLibraryLoader library_loader;
  if (!library_loader.LoadInMain()) {
    return 1;
  }

  CefMainArgs main_args(argc, argv);
  int result = 0;

  @autoreleasepool {
    [BrowserApplication sharedApplication];
    CHECK([NSApp isKindOfClass:[BrowserApplication class]]);

    CefRefPtr<CefCommandLine> command_line =
        CefCommandLine::CreateCommandLine();
    command_line->InitFromArgv(argc, argv);

    CefSettings settings;

#if !defined(CEF_USE_SANDBOX)
    settings.no_sandbox = true;
#endif

    // CEF won't block/pump its own loop; instead it calls
    // PetrichorApp::OnScheduleMessagePumpWork() to ask us to pump it via
    // CefDoMessageLoopWork() on our own schedule (see PetrichorApp.cpp).
    // This is what makes it possible for Qt to own the actual event loop.
    settings.external_message_pump = true;

    // QApplication mutates the argc/argv it's given (stripping Qt's own
    // recognized flags). CefMainArgs above already captured the original
    // argc/argv by pointer, not by deep copy, so handing Qt the same array
    // could leave main_args looking at a mutated/inconsistent view. Give
    // QApplication its own independent copy instead. This vector must
    // outlive qt_app, so it's declared here in main()'s scope rather than
    // in a smaller nested block.
    std::vector<char*> qt_argv(argv, argv + argc);
    int qt_argc = argc;
    QApplication qt_app(qt_argc, qt_argv.data());

    // PetrichorApp owns a Qt timer for the external CEF message pump, so Qt
    // must already exist before PetrichorApp is constructed.
    CefRefPtr<PetrichorApp> cef_app(new PetrichorApp);

    // Must exist and be registered with cef_app before CefInitialize(),
    // since OnContextInitialized() (which asks main_window for its native
    // handle/rect) can fire synchronously from inside that call.
    MainWindow main_window;
    cef_app->SetMainWindow(&main_window);

    if (!CefInitialize(main_args, settings, cef_app.get(), nullptr)) {
      return CefGetExitCode();
    }

    BrowserAppDelegate* delegate = [[BrowserAppDelegate alloc] init];
    NSApp.delegate = delegate;

    [delegate performSelectorOnMainThread:@selector(createApplication:)
                               withObject:nil
                            waitUntilDone:NO];

    main_window.show();

    // Qt now owns the event loop. CEF gets pumped on the side whenever
    // OnScheduleMessagePumpWork() schedules a QTimer to call
    // CefDoMessageLoopWork().
    result = qt_app.exec();

    CefShutdown();

#if !__has_feature(objc_arc)
    [delegate release];
#endif
    delegate = nil;
  }

  return result;
}
