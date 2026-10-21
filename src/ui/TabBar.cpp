#include "ui/TabBar.h"

#include <QVariant>

TabBar::TabBar(QWidget* parent) : QTabBar(parent) {
  add_tab_button_ = new QToolButton(this);
  add_tab_button_->setText("+");
  add_tab_button_->setAutoRaise(true);
  setTabsClosable(true);

  connect(add_tab_button_, &QToolButton::clicked, this, &TabBar::NewTabRequested);
}

TabBar::~TabBar() = default;

void TabBar::AddTab(int browser_id, const QString& title) {
  const int index = addTab(title);
  setTabData(index, browser_id);
  setTabButton(index - 1, RightSide, nullptr);
  setTabButton(index, RightSide, add_tab_button_);
}

bool TabBar::RemoveTab(int browser_id) {
  for (int index = 0; index < count(); ++index) {
    if (GetBrowserIdAt(index) == browser_id) {
      removeTab(index);
      return true;
    }
  }
  return false;
}

bool TabBar::SetActiveTab(int browser_id) {
  for (int index = 0; index < count(); ++index) {
    if (GetBrowserIdAt(index) == browser_id) {
      setCurrentIndex(index);
      return true;
    }
  }
  return false;
}

int TabBar::GetBrowserIdAt(int index) const {
  if (index < 0 || index >= count()) {
    return -1;
  }

  bool ok = false;
  const int browser_id = tabData(index).toInt(&ok);
  return ok ? browser_id : -1;
}
