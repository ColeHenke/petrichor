#ifndef BROWSER_HANDLER_H_
#define BROWSER_HANDLER_H_

#include <list>

#include "include/cef_client.h"
#include "include/internal/cef_types_wrappers.h"

class MainWindow;

// BrowserHandler implements CEF client callbacks for the browser process.
// It manages the lifecycle of all browser windows and handles load events,
// title changes, and shutdown sequencing.
//
// We always use Alloy runtime style, which means:
// - CEF gives us a bare native window with no Chrome UI
// - We're responsible for the shell (Qt handles this)
// - Platform-specific methods (PlatformTitleChange etc.) are always active
class BrowserHandler : public CefClient,
                       public CefDisplayHandler,
                       public CefLifeSpanHandler,
                       public CefLoadHandler {
 public:
  BrowserHandler();
  ~BrowserHandler() override;
  typedef std::list<CefRefPtr<CefBrowser>> BrowserList;

  // Returns the single global instance.
  static BrowserHandler* GetInstance();

  // CefClient methods:
  CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
  CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
  CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }

  // CefDisplayHandler methods:
  void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) override;
  void OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& url) override;

  // CefLifeSpanHandler methods:
  void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
  bool DoClose(CefRefPtr<CefBrowser> browser) override;
  void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

  // CefLoadHandler methods:
  void OnLoadError(CefRefPtr<CefBrowser> browser,
                   CefRefPtr<CefFrame> frame,
                   ErrorCode errorCode,
                   const CefString& errorText,
                   const CefString& failedUrl) override;

  // Show the first browser window (called when app is re-opened via dock).
  void ShowMainWindow();

  void SetMainWindow(MainWindow* main_window);

  // Request all browser windows to close.
  void CloseAllBrowsers(bool force_close);

  bool IsClosing() const { return is_closing_; }

  BrowserList GetBrowserList() const { return browser_list_; }

  void ResizeBrowser(CefRefPtr<CefBrowser> browser, const CefRect& rect);

  void OnLoadEnd(CefRefPtr<CefBrowser> browser,
               CefRefPtr<CefFrame> frame,
               int httpStatusCode) override;

 private:
  // Implemented in BrowserHandler_mac.mm
  void PlatformTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title);
  void PlatformShowWindow(CefRefPtr<CefBrowser> browser);
  void PlatformResizeView(CefRefPtr<CefBrowser> browser, const CefRect& rect);

  MainWindow* main_window_ = nullptr;

  BrowserList browser_list_;

  bool is_closing_ = false;

  IMPLEMENT_REFCOUNTING(BrowserHandler);
};

#endif  // BROWSER_HANDLER_H_
