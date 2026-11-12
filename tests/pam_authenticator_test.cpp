#include "auth/pam_authenticator.h"
#include "test_check.h"

#include <array>
#include <cstdlib>
#include <security/pam_appl.h>
#include <string>
#include <string_view>
#include <unistd.h>

namespace {
  int observationFd = -1;

  std::string observedService(PamAuthenticator& authenticator) {
    TEST_CHECK(::ftruncate(observationFd, 0) == 0);
    TEST_CHECK(::lseek(observationFd, 0, SEEK_SET) == 0);

    TEST_CHECK(!authenticator.authenticateCurrentUser("").success);
    TEST_CHECK(::lseek(observationFd, 0, SEEK_SET) == 0);

    std::array<char, 128> buffer{};
    const ssize_t size = ::read(observationFd, buffer.data(), buffer.size());
    TEST_CHECK(size >= 0);
    return {buffer.data(), static_cast<std::size_t>(size)};
  }
} // namespace

extern "C" int pam_start(
    const char* serviceName, const char* /*user*/, const struct pam_conv* /*pamConversation*/, pam_handle_t** pamHandle
) {
  if (serviceName != nullptr) {
    const std::string_view service(serviceName);
    if (::write(observationFd, service.data(), service.size()) < 0) {
      return PAM_SYSTEM_ERR;
    }
  }
  *pamHandle = nullptr;
  return PAM_SERVICE_ERR;
}

extern "C" int pam_authenticate(pam_handle_t* /*pamHandle*/, int /*flags*/) {
  constexpr std::string_view unexpectedCall = "pam_authenticate called";
  const ssize_t written = ::write(observationFd, unexpectedCall.data(), unexpectedCall.size());
  (void)written;
  return PAM_AUTH_ERR;
}

extern "C" int pam_acct_mgmt(pam_handle_t* /*pamHandle*/, int /*flags*/) { return PAM_SUCCESS; }

extern "C" int pam_end(pam_handle_t* /*pamHandle*/, int /*status*/) { return PAM_SUCCESS; }

extern "C" const char* pam_strerror(pam_handle_t* /*pamHandle*/, int /*status*/) { return "mock PAM"; }

int main() {
  char observationPath[] = "/tmp/noctalia-pam-service-test-XXXXXX";
  observationFd = ::mkstemp(observationPath);
  TEST_CHECK(observationFd >= 0);
  TEST_CHECK(::unlink(observationPath) == 0);

  PamAuthenticator authenticator;

  TEST_CHECK(::unsetenv("NOCTALIA_PAM_SERVICE") == 0);
  TEST_CHECK(observedService(authenticator) == "login");

  TEST_CHECK(::setenv("NOCTALIA_PAM_SERVICE", "noctalia-test", 1) == 0);
  TEST_CHECK(observedService(authenticator) == "noctalia-test");

  TEST_CHECK(::setenv("NOCTALIA_PAM_SERVICE", "", 1) == 0);
  TEST_CHECK(observedService(authenticator) == "login");

  TEST_CHECK(::close(observationFd) == 0);
  return EXIT_SUCCESS;
}
