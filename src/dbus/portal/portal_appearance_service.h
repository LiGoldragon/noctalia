#pragma once

#include "theme/external_appearance.h"

#include <cstdint>
#include <memory>

class ConfigService;
class SessionBus;

namespace sdbus {
  class IProxy;
}

namespace noctalia::theme {
  class ThemeService;
}

namespace noctalia::dbus {

  // Bridges the portal-owned desktop preference into Noctalia's external
  // appearance mode. It never writes desktop settings.
  class PortalAppearanceService {
  public:
    PortalAppearanceService(SessionBus& bus, ConfigService& config, theme::ThemeService& theme);
    ~PortalAppearanceService();

    void refresh();
    void onConfigReload();

  private:
    void observe(std::uint32_t value);

    ConfigService& m_config;
    theme::ThemeService& m_theme;
    theme::ExternalAppearanceController m_externalAppearance;
    std::unique_ptr<sdbus::IProxy> m_proxy;
  };

} // namespace noctalia::dbus
