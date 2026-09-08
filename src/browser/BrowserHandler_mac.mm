// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "browser/BrowserHandler.h"

#import <Cocoa/Cocoa.h>

#include "include/cef_browser.h"

namespace {

NSWindow* GetNSWindowForBrowser(CefRefPtr<CefBrowser> browser) {
  NSView* view =
      CAST_CEF_WINDOW_HANDLE_TO_NSVIEW(browser->GetHost()->GetWindowHandle());
  return [view window];
}

}  // namespace

void BrowserHandler::PlatformTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) {
  NSWindow* window = GetNSWindowForBrowser(browser);
  std::string titleStr(title);
  NSString* str = [NSString stringWithUTF8String:titleStr.c_str()];
  [window setTitle:str];
}

void BrowserHandler::PlatformShowWindow(CefRefPtr<CefBrowser> browser) {
  NSWindow* window = GetNSWindowForBrowser(browser);
  [window makeKeyAndOrderFront:window];
}

void BrowserHandler::PlatformResizeView(CefRefPtr<CefBrowser> browser, const CefRect& rect) {
  NSView* view = CAST_CEF_WINDOW_HANDLE_TO_NSVIEW(browser->GetHost()->GetWindowHandle());

  if (view == nil) {
    return;
  }

  [view setFrame:NSMakeRect(rect.x, rect.y, rect.width, rect.height)];
}
