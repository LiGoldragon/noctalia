#include "theme/external_appearance.h"

#include <print>
#include <string_view>

namespace {

  int failures = 0;

  void expect(bool condition, std::string_view message) {
    if (!condition) {
      std::println(stderr, "external_appearance_test: FAIL: {}", message);
      ++failures;
    }
  }

  void checkInitialReadAndSignalUpdate() {
    bool persisted = false;
    bool applied = false;
    noctalia::theme::ExternalAppearanceController controller(
        std::nullopt, [&](bool isLight) { persisted = isLight; }, [&](bool isLight) { applied = isLight; }
    );
    expect(!controller.isLight(), "first run falls back to dark before the portal answers");
    expect(controller.observePortalValue(2), "initial portal Read accepts prefer-light");
    expect(controller.isLight() && persisted && applied, "initial Read persists and applies light");
    applied = true;
    expect(controller.observePortalValue(1), "SettingChanged accepts prefer-dark");
    expect(!controller.isLight() && !persisted && !applied, "signal update persists and applies dark");
  }

  void checkPersistenceAndNoPreference() {
    bool wrote = false;
    noctalia::theme::ExternalAppearanceController controller(
        true, [&](bool) { wrote = true; }, [&](bool) { failures += 100; }
    );
    expect(controller.isLight(), "persisted external observation survives restart");
    expect(!controller.observePortalValue(0), "portal no-preference is not authoritative");
    expect(controller.isLight() && !wrote, "no-preference retains persisted mode without a write");
    expect(!noctalia::theme::externalAppearanceModeFromPortalValue(99).has_value(), "unknown portal value is ignored");
    expect(
        !noctalia::theme::shouldSyncGlobalColorScheme(ThemeMode::External),
        "external mode never writes the global GNOME color-scheme"
    );
    expect(
        noctalia::theme::shouldSyncGlobalColorScheme(ThemeMode::Dark),
        "non-external modes retain their existing global-color behavior"
    );
  }

} // namespace

int main() {
  checkInitialReadAndSignalUpdate();
  checkPersistenceAndNoPreference();
  return failures == 0 ? 0 : 1;
}
