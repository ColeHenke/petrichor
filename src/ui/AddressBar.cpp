#include "ui/AddressBar.h"

AddressBar::AddressBar(QWidget* parent) : QLineEdit(parent) {
  connect(this, &QLineEdit::returnPressed, this, &AddressBar::OnReturnPressed);
}

void AddressBar::SetUrl(const QString& url) {
  setText(url);
}

void AddressBar::OnReturnPressed() {
  QString url = this->text();

  if (!url.contains("://")) {
    url.prepend("https://");
  }
  printf("URL submitted: %s\n", url.toStdString().c_str());
  emit urlSubmitted(url);
}
