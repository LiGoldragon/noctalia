#include "dbus/portal/portal_appearance_service.h"

#include "config/config_service.h"
#include "config/config_types.h"
#include "core/log.h"
#include "dbus/session_bus.h"
#include "theme/external_appearance.h"
#include "theme/theme_service.h"

#include <sdbus-c++/IProxy.h>
#include <sdbus-c++/Types.h>

namespace noctalia::dbus {

  namespace {
    constexpr Logger kLog("portal_appearance");
    const sdbus::ServiceName kPortalBusName{"org.freedesktop.portal.Desktop"};
    const sdbus::ObjectPath kPortalObjectPath{"/org/freedesktop/portal/desktop"};
    constexpr auto kSettingsInterface = "org.freedesktop.portal.Settings";
    constexpr auto kAppearanceNamespace = "org.freedesktop.appearance";
    constexpr auto kColorSchemeKey = "color-scheme";
  } // namespace

  PortalAppearanceService::PortalAppearanceService(SessionBus& bus, ConfigService& config, theme::ThemeService& theme)
      : m_config(config), m_theme(theme),
        m_externalAppearance(
            [&config]() -> std::optional<bool> {
              const auto persisted = config.stateString("theme", "external_mode");
              if (!persisted.has_value()) {
                return std::nullopt;
              }
              return *persisted == "light";
            }(),
            [this](bool isLight) {
              (void)m_config.setStateString("theme", "external_mode", isLight ? "light" : "dark");
            },
            [this](bool isLight) { m_theme.setExternalMode(isLight); }
        ),
        m_proxy(sdbus::createProxy(bus.connection(), kPortalBusName, kPortalObjectPath)) {
    m_proxy->uponSignal("SettingChanged")
        .onInterface(kSettingsInterface)
        .call([this](const std::string& nameSpace, const std::string& key, const sdbus::Variant& value) {
          if (nameSpace != kAppearanceNamespace || key != kColorSchemeKey) {
            return;
          }
          try {
            observe(value.get<std::uint32_t>());
          } catch (const sdbus::Error&) {
            kLog.warn("portal color-scheme signal carried an invalid value");
          }
        });
    refresh();
  }

  PortalAppearanceService::~PortalAppearanceService() = default;

  void PortalAppearanceService::refresh() {
    try {
      sdbus::Variant value;
      m_proxy->callMethod("Read")
          .onInterface(kSettingsInterface)
          .withArguments(std::string{kAppearanceNamespace}, std::string{kColorSchemeKey})
          .storeResultsTo(value);
      observe(value.get<std::uint32_t>());
    } catch (const sdbus::Error& error) {
      // The portal is optional. The ThemeService retains its persisted mode,
      // or Dark on the first run, until a real authoritative value arrives.
      kLog.debug("portal color-scheme unavailable: {}", error.getName().c_str());
    }
  }

  void PortalAppearanceService::onConfigReload() {
    if (m_config.config().theme.mode == ThemeMode::External) {
      refresh();
    }
  }

  void PortalAppearanceService::observe(std::uint32_t value) {
    (void)m_externalAppearance.observePortalValue(value);
  }

} // namespace noctalia::dbus
