#pragma once

#include "config/config_types.h"

#include <cstdint>
#include <functional>
#include <optional>

namespace noctalia::theme {

  // org.freedesktop.appearance color-scheme values from xdg-desktop-portal.
  constexpr std::optional<bool> externalAppearanceModeFromPortalValue(std::uint32_t value) {
    switch (value) {
    case 1:
      return false; // prefer-dark
    case 2:
      return true; // prefer-light
    default:
      return std::nullopt; // no-preference or an extension we do not own
    }
  }

  [[nodiscard]] constexpr bool shouldSyncGlobalColorScheme(ThemeMode configuredMode) {
    return configuredMode != ThemeMode::External;
  }

  // Keeps the last authoritative portal preference. This deliberately has no
  // timer: the portal is the authority and SettingChanged is its update path.
  class ExternalAppearanceController {
  public:
    using Persist = std::function<void(bool)>;
    using Apply = std::function<void(bool)>;

    ExternalAppearanceController(std::optional<bool> persisted, Persist persist, Apply apply)
        : m_isLight(persisted.value_or(false)), m_persist(std::move(persist)), m_apply(std::move(apply)) {}

    [[nodiscard]] bool isLight() const noexcept { return m_isLight; }

    // Used for both the startup Read result and SettingChanged. No-preference
    // is not an observation, so it preserves the last observed (or Dark) mode.
    bool observePortalValue(std::uint32_t value) {
      const auto mode = externalAppearanceModeFromPortalValue(value);
      if (!mode.has_value()) {
        return false;
      }
      const bool changed = m_isLight != *mode;
      m_isLight = *mode;
      if (m_persist) {
        m_persist(m_isLight);
      }
      if (changed && m_apply) {
        m_apply(m_isLight);
      }
      return changed;
    }

  private:
    bool m_isLight = false;
    Persist m_persist;
    Apply m_apply;
  };

} // namespace noctalia::theme
