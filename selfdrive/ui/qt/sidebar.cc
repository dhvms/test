#include "selfdrive/ui/qt/sidebar.h"

#include <QMouseEvent>

#include "selfdrive/common/util.h"
#include "selfdrive/hardware/hw.h"
#include "selfdrive/ui/qt/util.h"

void Sidebar::drawMetric(QPainter &p, const QString &label, QColor c, int y) {
  const QRect rect = {30, y, 240, label.contains("\n") ? 124 : 100};

  p.setPen(Qt::NoPen);
  p.setBrush(QBrush(c));
  p.setClipRect(rect.x() + 6, rect.y(), 18, rect.height(), Qt::ClipOperation::ReplaceClip);
  p.drawRoundedRect(QRect(rect.x() + 6, rect.y() + 6, 100, rect.height() - 12), 10, 10);
  p.setClipping(false);

  QPen pen = QPen(QColor(0xff, 0xff, 0xff, 0x55));
  pen.setWidth(2);
  p.setPen(pen);
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(rect, 20, 20);

  p.setPen(QColor(0xff, 0xff, 0xff));
  configFont(p, "Open Sans", 35, "Bold");
  const QRect r = QRect(rect.x() + 30, rect.y(), rect.width() - 40, rect.height());
  p.drawText(r, Qt::AlignCenter, label);
}

Sidebar::Sidebar(QWidget *parent) : QFrame(parent) {
  home_img = loadPixmap("../assets/images/button_home.png", {180, 180});
  settings_img = loadPixmap("../assets/images/button_settings.png", settings_btn.size(), Qt::IgnoreAspectRatio);

  connect(this, &Sidebar::valueChanged, [=] { update(); });

  setAttribute(Qt::WA_OpaquePaintEvent);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  setFixedWidth(300);

  QObject::connect(uiState(), &UIState::uiUpdate, this, &Sidebar::updateState);
}

void Sidebar::mouseReleaseEvent(QMouseEvent *event) {
  if (settings_btn.contains(event->pos())) {
    emit openSettings();
  }
}

void Sidebar::updateState(const UIState &s) {
  if (!isVisible()) return;

  auto &sm = *(s.sm);

  auto deviceState = sm["deviceState"].getDeviceState();
  setProperty("netType", network_type[deviceState.getNetworkType()]);
  int strength = (int)deviceState.getNetworkStrength();
  setProperty("netStrength", strength > 0 ? strength + 1 : 0);
  setProperty("wifiAddr", deviceState.getWifiIpAddress().cStr());

  int batteryPercent = deviceState.getBatteryPercent();
  QColor batteryColor = good_color;
  if(batteryPercent < 30)
    batteryColor = warning_color;
  else if(batteryPercent < 10)
    batteryColor = danger_color;

  QString batteryDesc;
  batteryDesc.sprintf("배터리\n%d%%", batteryPercent);
  setProperty("batteryPercent", QVariant::fromValue(ItemStatus{batteryDesc, batteryColor}));

  //ItemStatus tempStatus = {"TEMP\nHIGH", danger_color};
  ItemStatus tempStatus = {"장치온도\n높음", danger_color};
  auto ts = deviceState.getThermalStatus();
  if (ts == cereal::DeviceState::ThermalStatus::GREEN) {
    //tempStatus = {"TEMP\nGOOD", good_color};
    tempStatus = {"장치온도\n좋음", good_color};
  } else if (ts == cereal::DeviceState::ThermalStatus::YELLOW) {
    //tempStatus = {"TEMP\nnWARNING", warning_color};
    tempStatus = {"장치온도\n경고", warning_color};
  }
  setProperty("tempStatus", QVariant::fromValue(tempStatus));

  //ItemStatus pandaStatus = {"VEHICLE\nONLINE", good_color};
  ItemStatus pandaStatus = {"판다\n연결됨", good_color};
  if (s.scene.pandaType == cereal::PandaState::PandaType::UNKNOWN) {
    //pandaStatus = {"NO\nPANDA", danger_color};
    pandaStatus = {"판다\n연결안됨", danger_color};
  } /*else if (s.scene.started && !sm["liveLocationKalman"].getLiveLocationKalman().getGpsOK()) {
    pandaStatus = {"GPS\nSEARCH", warning_color};
  }*/
  setProperty("pandaStatus", QVariant::fromValue(pandaStatus));
}

void Sidebar::paintEvent(QPaintEvent *event) {
  QPainter p(this);
  p.setPen(Qt::NoPen);
  p.setRenderHint(QPainter::Antialiasing);

  p.fillRect(rect(), QColor(57, 57, 57));

  // static imgs
  p.setOpacity(0.65);
  p.drawPixmap(settings_btn.x(), settings_btn.y(), settings_img);
  p.setOpacity(1.0);
  p.drawPixmap(60, 1080 - 180 - 40, home_img);

  // network
  int x = 58;
  const QColor gray(0x54, 0x54, 0x54);
  for (int i = 0; i < 5; ++i) {
    p.setBrush(i < net_strength ? Qt::white : gray);
    p.drawEllipse(x, 196, 27, 27);
    x += 37;
  }

  configFont(p, "Open Sans", 30, "Regular");
  p.setPen(QColor(0xff, 0xff, 0xff));

  const QRect r = QRect(0, 247, event->rect().width(), 50);

  if(net_type == network_type[cereal::DeviceState::NetworkType::WIFI])
    p.drawText(r, Qt::AlignCenter, wifi_addr);
  else
    p.drawText(r, Qt::AlignCenter, net_type);

  // metrics
  configFont(p, "Open Sans", 35, "Regular");
  drawMetric(p, temp_status.first, temp_status.second, 338);
  drawMetric(p, panda_status.first, panda_status.second, 496);
  drawMetric(p, battery_percent.first, battery_percent.second, 654);
}